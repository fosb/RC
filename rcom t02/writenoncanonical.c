/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h> //adicionado
#include <string.h> //adicionado for strlen


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

//emissor que vai ler do stdin? /dev/ttyS10
//nos pcs da feup é /ttyS0 E /ttyS1

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255], msg[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || ((strcmp("/dev/ttyS10", argv[1])!=0) && (strcmp("/dev/ttyS11", argv[1])!=0) )) 
    //certifica-se que inseriste uma porta seria validas havia aqui um bug ? perguntar stor
    {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1X\n");
      exit(1);
    }
    
    printf("Hello, please enter a message to be sent: ");
    gets(msg); //le do stdin o input
    
    for(i = 0; i < strlen(msg); i++) //percorre a msg inserida e conta o n� de caracteres exeto ' '
    { 
    	if(msg[i] != ' ')
    	{	
    		sum++;
    	} 
    }
    
    printf("The message you wrote has %d characters\n", sum); 

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
    
    /* VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a leitura do(s) pr�ximo(s) caracter(es)*/

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) 
    {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    /* O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar o indicado no gui�o */
    
    for(i = 0; i < strlen(msg); i++) //percorre a msg lida e aloca esses caracters no buf
    { 
      buf[i] = msg[i];	
    }
    
    buf[strlen(buf)] = '\n'; // \0??
    
    res = write(fd,buf,strlen(buf));   
    printf("%d bytes written\n", res);
    
    printf("Waiting for confirmation...\n");
 
    while (STOP==FALSE) 
    {       /* loop for input */
      printf("Received confirmation\n");
      res = read(fd,buf,255);   /* returns after 5 chars have been input */
      buf[res]=0;               /* so we can printf... */
      printf(":%s:%d\n", buf, res);
      printf("End transmission\n");
      STOP=TRUE;
    }
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) 
    {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
