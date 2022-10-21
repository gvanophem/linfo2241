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

typedef struct {
int m, n; // dimensions de la matrice
	int *data; // tableau 1D de taille m*n contenant les entrées de la matrice
	int **a; // tableau 1D de m pointeurs vers chaque ligne, pour pouvoir appeler a[i][j]
} matrix;

matrix* allocate_matrix(int m, int n) {
	matrix *mat = (matrix*) malloc(sizeof(matrix));
	mat->m = m, mat->n = n;
	mat->data = (int*)malloc(m*n*sizeof(int));
	if(mat->data == NULL) return NULL;
	mat->a = (int**)malloc(m*sizeof(int*));
	if (mat->a == NULL) return NULL;
	for (int i = 0; i < m; i++)
		mat->a[i] = mat->data+i*n;
	return mat;
}
 
void free_matrix(matrix *mat) {
	if(mat == NULL) return;
	free(mat->a);
	free(mat->data);       
}

char* substr(char* src, int start, int len){
    printf("fdp\n");
    printf("%d\n",sizeof(char)*(len+1));
    char* sub = malloc(sizeof(char)*(len+1));
    printf("bug\n");
    memcpy(sub, &src[start], len);
    sub[len] = '\0';
    return sub;
}

void send_message(int sockfd, int size){
    // generate key
    matrix* key = allocate_matrix(size,size);
    for (int i = 0; i < size*size; i++)
    {
        key->data[i] = rand()%256;
    }
    char* msg = malloc(size * size + 8);
    int written_bytes = 0;
    int ind = rand()%1000;
    char index[4];
    sprintf(index, "%d", ind);
    memcpy(&msg[written_bytes], index, 4); written_bytes+= 4;
    char length[4];
    sprintf(length, "%d", size);
    memcpy(&msg[written_bytes], length, 4); written_bytes += 4;
    char* val = malloc(sizeof(int));
    for(int i = 0; i < size * size; i++){
        sprintf(val, "%d", key->data[i]);
        memcpy(&msg[written_bytes], val, 4);
        written_bytes += 4;
    }
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            printf("%d ", key->data[i*size + j]);
        }printf("\n");
    }printf("strlen(msg) : %d\n", strlen(msg));
    for(int i = 0; i < size*size+2; i+=1){
        char sub[5];
        memcpy(sub, &msg[i*4], 4);
        printf("%d ", (uint32_t)atoi(sub));
    }
    printf("ind : %d\n",ind);

    printf("%s",msg); printf("\n");
    //write(sockfd, msg, size*size + 8);
    send(sockfd, msg, size * size + 8, 0);
    printf("%s",msg); printf("\n");

}

void func(int sockfd, int max_time, int rate, struct sockaddr_in serveraddr, int size)
{
    int count = 0;
    time_t begin = time(NULL);
    double beg = begin;
    time_t sec;
    sec = time(NULL);
    int clac = 1;
    int i = 0;

    char buffer[size*size + 5];
    size_t bytes_read;
    int msgsize = 0;

    while(difftime(time(NULL), begin) < max_time){
        if(clac == 1){
            while(count < rate){
                // int sockfd;
                // if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != 0){
                //     printf("socket creation failed\n");
                //     exit(0);
                // }if(connect(sockfd, (SA*)&serveraddr, sizeof(serveraddr)) != 0){
                //     printf("failed to connect\n");
                //     exit(0);
                // }
                send_message(sockfd,size);
                count++;
                i++;
                while((bytes_read = read(sockfd, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0){
                    msgsize+= bytes_read;
                    if(msgsize > size * size + 5 -1 || buffer[msgsize-1] == '\n'){
                        break;
                    }
                }buffer[msgsize-1] = 0;
                printf("Request : %s\n", buffer);
                printf("msg size : %d\n", msgsize);
                msgsize = 0;
                bytes_read = 0;
                close(sockfd);
                
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

    func(sockfd, max_time, rate, servaddr, size);
 
    // close the socket
    close(sockfd);

    

    free(str);
}