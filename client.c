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

void send_message(int sockfd){
    write(sockfd, "Hello", 5);
}

void func(int sockfd, int max_time, int rate)
{
    int count = 0;
    time_t begin = time(NULL);
    double beg = begin;
    time_t sec;
    sec = time(NULL);
    int clac = 1;
    int i = 0;
    while(difftime(time(NULL), begin) < max_time){
        if(clac == 1){
            while(count < rate){
                send_message(sockfd);
                count++;
                i++;
            }clac = 0;
        }if(sec - time(NULL) != 0){
            clac = 1;
            count = 0;
            sec = time(NULL);
        }
    }printf("%d message sent\n", i);
}
 
int main(int argc, char** argv)
{
    //printf("opt");
    int opt;

    int size;
    int rate;
    int time;

    while ((opt = getopt(argc, argv, ":k:r:t:?")) != -1) {
        switch (opt) {
        case 'k':
            size = atoi(optarg);
            //size = (int)*optarg;
            break;
        case 'r':
            rate = atoi(optarg);
            //rate = (int)*optarg;
            break;
        case 't':
            time = atoi(optarg);
            //time = (int)*optarg;
            //sscanf(optarg, "%d", time);
            break;
        default:
            printf("erreur\n");
            break;
        }

    }

    char* str = malloc(strlen(argv[argc-1]));
    strcpy(str, argv[argc - 1]);
    printf("last arg is : %s\n", str);
    printf("the size is : %d, the rate is : %d, and the time is : %d\n", size, rate, time);

    char* token = strtok(str, ":");
    char* address = malloc(sizeof(char)*strlen(token));
    memcpy(address, token, strlen(token));
    token = strtok(NULL, ":");
    char* port = malloc(strlen(token));
    memcpy(port, token, strlen(token));
    printf("token : %s\n", address);
    printf("token[1] : %s\n", port);

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
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    // function for chat

    func(sockfd, time, rate);
 
    // close the socket
    close(sockfd);

    free(str);
}