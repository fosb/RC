// Waiting for input from multiple sources

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

int open_input_source(char *);
void handle_input_from_source(int);
int max(int, int);

int main()
{
    // open_input_source opens a device, sets the port correctly, and
    // returns a file descriptor
    char serialPort1[] = "/dev/ttyS1";
    char serialPort2[] = "/dev/ttyS2";

    int fd1 = open_input_source(serialPort1); // COM2
    if (fd1 < 0)
    {
        perror(serialPort1);
        exit(0);
    }

    int fd2 = open_input_source(serialPort2); // COM3
    if (fd2 < 0)
    {
        perror(serialPort2);
        exit(0);
    }

    int maxfd = max(fd1, fd2) + 1; // Maximum file descriptor to test

    // Loop while TRUE for input
    int loop = TRUE;
    fd_set readfs; // File descriptor set

    while (loop == TRUE) {
        FD_SET(fd1, &readfs);  // Set testing for source 1
        FD_SET(fd2, &readfs);  // Set testing for source 2

        // block until input becomes available
        // select will not load the CPU
        select(maxfd, &readfs, NULL, NULL, NULL);

        // input from source 1 available
        if (FD_ISSET(fd1, &readfs))
            handle_input_from_source(fd1);

        // input from source 2 available
        if (FD_ISSET(fd2, &readfs))
            handle_input_from_source(fd2);
    }

    close(fd1);
    close(fd2);

    return 0;
}

int open_input_source(char *port)
{
    // Open the device to be non-blocking (read will return immediately)
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
        return -1;
    else
        return fd;
}

void handle_input_from_source(int fd)
{
    char buf[BUF_SIZE];

    int bytes = read(fd, buf, BUF_SIZE - 1); // -1: Save space for the final '\0' char
    buf[bytes] = '\0';                       // Set end of string to '\0', so we can printf
    printf("%s:%d", buf, bytes);
}

int max(int i1, int i2)
{
    if (i1 > i2)
        return i1;
    else
        return i2;
}
