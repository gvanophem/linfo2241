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

void send_message(){
    printf("message\n");
}

void func(int sockfd, int max_time, int rate)
{
    int count = 0;
    time_t begin = time(NULL);
    double beg = begin;
    printf("beg : %ld\n", beg);
    time_t maximum = max_time;
    time_t sec;
    sec = time(NULL);
    time_t test;
    test = time(NULL);
    int clac = 0;
    int i = 0;
    printf("maximum : %ld\n", maximum);
    while(begin - time(NULL) < maximum){
        time_t temp = difftime(time(NULL), begin);
        printf("difference : %ld\n", temp);
        printf("begin : %ld\n", begin);
        printf("time actuel : %ld\n", time(NULL));
        if(clac == 1){
            while(count < rate){
                //send_message();
                count++;
            }clac = 0;
        }if(sec - time(NULL) != 0){
            clac = 1;
            count = 0;
        }i++;
        if(i == 1000) break;
    }
    // char buff[MAX];
    // int n;
    // // infinite loop for chat
    // for (;;) {
    //     bzero(buff, MAX);
   
    //     // read the message from client and copy it in buffer
    //     read(connfd, buff, sizeof(buff));
    //     // print buffer which contains the client contents
    //     printf("From client: %s\t To client : ", buff);
    //     bzero(buff, MAX);
    //     n = 0;
    //     // copy server message in the buffer
    //     while ((buff[n++] = getchar()) != '\n')
    //         ;
   
    //     // and send that buffer to client
    //     write(connfd, buff, sizeof(buff));
   
    //     // if msg contains "Exit" then server exit and chat ended.
    //     if (strncmp("exit", buff, 4) == 0) {
    //         printf("Server Exit...\n");
    //         break;
    //     }
    // }
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

    

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    func(sockfd, time, rate);
 
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
    
 
    // close the socket
    close(sockfd);

    free(str);
}