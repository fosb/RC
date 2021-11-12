#include "linklayer.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BAUDRATE 9600
#define NUM_RETRANSMISSIONS 3
#define TIMEOUT 3

/*
 * $1 /dev/ttySxx
 * $2 tx | rx
 * $3 filename
 */

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s /dev/ttySxx tx|rx filename\n", argv[0]);
        exit(1);
    }

    char *serialPort = argv[1];
    char *role = argv[2];
    char *filename = argv[3];

    printf("%s %s %s\n", serialPort, role, filename);
    fflush(stdout);

    if (strcmp(role, "tx") == 0)
    {
        // ***********
        // tx mode
        printf("tx mode\n");

        // open connection
        struct linkLayer ll;
        sprintf(ll.serialPort, "%s", serialPort);
        ll.role = TRANSMITTER;
        ll.baudRate = BAUDRATE;
        ll.numTries = NUM_RETRANSMISSIONS;
        ll.timeOut = TIMEOUT;

        if(llopen(ll) == -1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }

        printf("connection opened\n");
        fflush(stdout);
        fflush(stderr);

        // open file to read
        int file_desc = open(filename, O_RDONLY);
        if(file_desc < 0) {
            fprintf(stderr, "Error opening file: %s\n", filename);
            exit(1);
        }

        // cycle through
        const int buf_size = MAX_PAYLOAD_SIZE - 1;
        char buffer[buf_size + 1];
        int write_result = 0;
        int bytes_read = 1;

        while (bytes_read > 0)
        {
            bytes_read = read(file_desc, buffer + 1, buf_size);
            if(bytes_read < 0) {
                fprintf(stderr, "Error receiving from link layer\n");
                break;
            }
            else if (bytes_read > 0) {
                // continue sending data
                buffer[0] = 1;
                write_result = llwrite(buffer, bytes_read + 1);
                if(write_result < 0) {
                    fprintf(stderr, "Error sending data to link layer\n");
                    break;
                }
                printf("read from file -> write to link layer, %d\n", bytes_read);
            }
            else if (bytes_read == 0) {
                // stop receiver
                buffer[0] = 0;
                llwrite(buffer, 1);
                printf("App layer: done reading and sending file\n");
                break;
            }

            sleep(1);
        }
        // close file
        close(file_desc);
    }
    else
    {
        // ***************
        // rx mode
        printf("rx mode\n");

        struct linkLayer ll;
        sprintf(ll.serialPort, "%s", serialPort);
        ll.role = RECEIVER;
        ll.baudRate = BAUDRATE;
        ll.numTries = NUM_RETRANSMISSIONS;
        ll.timeOut = TIMEOUT;

        if(llopen(ll) == -1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }

        int file_desc = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(file_desc < 0) {
            fprintf(stderr, "Error opening file: %s\n", filename);
            exit(1);
        }

        int bytes_read = 0;
        int write_result = 0;
        const int buf_size = MAX_PAYLOAD_SIZE;
        char buffer[buf_size];
        int total_bytes = 0;

        while (bytes_read >= 0)
        {
            bytes_read = llread(buffer);
            if(bytes_read < 0) {
                fprintf(stderr, "Error receiving from link layer\n");
                break;
            }
            else if (bytes_read > 0) {
                if (buffer[0] == 1) {
                    write_result = write(file_desc, buffer + 1, bytes_read - 1);
                    if(write_result < 0) {
                        fprintf(stderr, "Error writing to file\n");
                        break;
                    }
                    total_bytes = total_bytes + write_result;
                    printf("read from file -> write to link layer, %d %d %d\n", bytes_read, write_result, total_bytes);
                }
                else if (buffer[0] == 0) {
                    printf("App layer: done receiving file\n");
                    break;
                }
            }
        }

        // Close file
        close(file_desc);
    }

    // Close connection
    llclose(TRUE);
    return 0;
}
