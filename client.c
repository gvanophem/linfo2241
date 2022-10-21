#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <ctype.h>
#include <time.h>
#define MAX 80
#define PORT 8080
#define SA struct sockaddr
#define BUFSIZE 11

void send_message(int sockfd){
    write(sockfd, "Hello\n", 6);
}

void func(int sock, int max_time, int rate, struct sockaddr_in serveraddr)
{
    int count = 0;
    time_t begin = time(NULL);
    double beg = begin;
    time_t sec;
    sec = time(NULL);
    int clac = 1;
    int i = 0;

    char buffer[BUFSIZE];
    size_t bytes_read;
    int msgsize = 0;

    while(difftime(time(NULL), begin) < max_time){
        if(clac == 1){
            while(count < rate){
                int sockfd;
                if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != 0){
                    printf("socket creation failed\n");
                    exit(0);
                }if(connect(sockfd, (SA*)&serveraddr, sizeof(serveraddr)) != 0){
                    printf("failed to connect\n");
                    exit(0);
                }
                send_message(sockfd);
                count++;
                i++;
                while((bytes_read = read(sockfd, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0){
                    msgsize+= bytes_read;
                    if(msgsize > BUFSIZE -1 || buffer[msgsize-1] == '\n'){
                        break;
                    }
                }buffer[msgsize-1] = 0;
                printf("Request : %s\n", buffer);
                printf("msg size : %d\n", msgsize);
                msgsize = 0;
                bytes_read= 0;
                close(sockfd);
                
            }clac = 0;
        }if(sec - time(NULL) != 0){
            clac = 1;
            count = 0;
            sec = time(NULL);
        }
    }printf("%d message sent\n", i);
}
 
void my_func(int sockfd){
    send_message(sockfd);
    char buff[BUFSIZE];
    bzero(buff, BUFSIZE);
    read(sockfd, buff, BUFSIZE);
    printf("From Server : %s\n", buff);
}

int main(int argc, char** argv)
{
    int opt;

    int size;
    int rate;
    int max_time;

    while ((opt = getopt(argc, argv, ":k:r:t:?")) != -1) {
        switch (opt) {
        case 'k':
            size = atoi(optarg);
            break;
        case 'r':
            rate = atoi(optarg);
            break;
        case 't':
            max_time = atoi(optarg);
            break;
        default:
            break;
        }

    }

    char* str = malloc(strlen(argv[argc-1]));
    strcpy(str, argv[argc - 1]);
    
    char* token = strtok(str, ":");
    char* address = malloc(sizeof(char)*strlen(token));
    memcpy(address, token, strlen(token));
    token = strtok(NULL, ":");
    char* port = malloc(strlen(token));
    memcpy(port, token, strlen(token));

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(atoi(port));
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    // function for chat

    func(sockfd, max_time, rate, servaddr);
 
    // close the socket
    close(sockfd);

    

    free(str);
}