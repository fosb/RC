/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h> //adicionado
#include <string.h> //adicionado for strlen

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

//receptor /dev/ttyS11

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) ||  ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS52", argv[1])!=0) )) 
    {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

   /*Open serial port device for reading and writing and not as controlling tty because we don't want to get killed if linenoise sends CTRL-C.*/
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) 
    { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    
    /*VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a leitura do(s) pr�ximo(s) caracter(es) */

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) 
    {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    
    /* O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o */ 
    // falta ler um a um???????
    
    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,255);   /* returns after 5 chars have been input */
      //sleep(1);
      buf[res]=0;               /* so we can printf... */
      printf("Received a new message\n");
      printf(":%s:%d\n", buf, res);
      if (buf[res]==0) 
      {
        //buf[res] = 'Y'; para certificar que a msg � reenviada entre os dois programas e n�o � usada o valor antigo da variavel buf 
      	res = write(fd,buf,strlen(buf));   
      	printf("%d bytes resent\n", res);
      	printf("End reception\n");
      	STOP=TRUE;
      }
    }
   
    //entre o write e o tcsetattr usar se calhar um sleep(1);
    
    
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
