// Asynchronous input

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1
#define BUF_SIZE 256

volatile int STOP = FALSE;

void signal_handler_IO (int status);  // Definition of signal handler
int wait_flag = TRUE;                 // TRUE while no signal received

int main(int argc, char** argv)
{
    // Program usage: Uses either COM1 or COM2
    if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) ))
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0], argv[0]);
        exit(1);
    }

    // Open the device to be non-blocking (read will return immediately)
    int fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
    // int fd = open(argv[1], O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror(argv[1]);
        exit(-1);
    }


    // To receive the SIGIO we need to perform three steps:
    // 1. Establish a signal handler for the signal - sigaction()
    // 2. Set the pid or gid to receive the signal for the descriptor
    // 3. Enable asynchronous I/O on the descriptor

    // Install the signal handler before making the device asynchronous
    struct sigaction saio;               // Definition of signal action.
    saio.sa_handler = signal_handler_IO; // Signal handler address or SI_IGN or SIG_DFL
    saio.sa_flags = 0;                   // gives a mask of signals which should
                                         // be blocked during execution of the
                                         // signal handler. In addition, the signal
                                         // which triggered the handler will be blocked,
                                         // unless the SA_NODEFER or SA_NOMASK flags
                                         // are used.
    saio.sa_restorer = NULL;             // Obsolete
    sigaction(SIGIO, &saio, NULL);

    // Allow the process to receive SIGIO
    // set the process ID or process group ID to receive
    // the SIGIO signal. A positive arg specifies the pid.
    // A negative arg implies a process group ID equal to
    // absolute value of arg
    fcntl(fd, F_SETOWN, getpid());
    // Make the file descriptor asynchronous (the manual page says only
    // O_APPEND and O_NONBLOCK, will work with F_SETFL...)
    fcntl(fd, F_SETFL, FASYNC);

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    // Set new port settings for canonical input processing
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_oflag = 0;
    newtio.c_lflag = ICANON;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    // Loop while waiting for input.
    // Normally we would do something useful here.
    while (STOP == FALSE) {
        // Keep printing "." to the console to show the program is running
        write(1, ".", 1);
        usleep(100000);

        // after receiving SIGIO, wait_flag = FALSE, input is available
        // and can be read
        char buf[BUF_SIZE];

        if (wait_flag == FALSE) {
            int bytes = read(fd, buf, BUF_SIZE - 1); // -1: Save space for the final '\0' char
            buf[bytes] = '\0';                       // Set end of string to '\0', so we can printf
            printf("%s:%d", buf, bytes);

            if (bytes == 1)
                STOP = TRUE;   // stop loop if only a CR was input

            wait_flag = TRUE;  // wait for new input
        }
    }

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

/***************************************************************************
* Signal handler. Sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/

void signal_handler_IO (int status)
{
    printf("received SIGIO signal\n");
    wait_flag = FALSE;
}
