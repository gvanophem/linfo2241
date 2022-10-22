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
	unsigned char *data; // tableau 1D de taille m*n contenant les entrées de la matrice
	unsigned char **a; // tableau 1D de m pointeurs vers chaque ligne, pour pouvoir appeler a[i][j]
} matrix;

matrix* allocate_matrix(int m, int n) {
	matrix *mat = (matrix*) malloc(sizeof(matrix));
	mat->m = m, mat->n = n;
	mat->data = (unsigned char*)malloc(m*n*sizeof(char));
	if(mat->data == NULL) return NULL;
	mat->a = (unsigned char**)malloc(m*sizeof(char*));
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

void print_matrix(matrix* m, int dim){
    for (int i = 0; i < dim; i++){
        for (int j = 0; j < dim; j++)
        {
            printf("%d  ",m->a[i][j]);
        }
        printf("\n");
    }
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

char modulo(int random, int mod){
    unsigned char c = random%mod;
    if((int)c < 0){
        c = (int)c + mod;
    }
    return c;
}

void send_message(int sockfd, int size){
    // generate key
    matrix* key = allocate_matrix(size,size);
    srand(time(0));
    for (int i = 0; i < size*size; i++)
    {
        int random = rand();
        //printf("%d ", random%256);
        key->data[i] = (unsigned char)modulo(random, 256);
        //printf("%d ", modulo(random, 256));
        //sprintf(&key->data[i], "%c", rand()%256);
    }print_matrix(key, size);
    char* msg = malloc(size * size + 8);
    int written_bytes = 0;
    int ind = rand()%1000;
    printf("index %d\n", ind);
    char index[4];
    sprintf(index, "%d", ind);
    memcpy(&msg[written_bytes], index, 4); written_bytes+= 4;
    char length[4];
    sprintf(length, "%d", size);
    memcpy(&msg[written_bytes], length, 4); written_bytes += 4;
    char* val = malloc(sizeof(char));
    for(int i = 0; i < size * size; i++){
        sprintf(val, "%c", key->data[i]);
        memcpy(&msg[written_bytes], val, 1);
        written_bytes += 1;
    }char* idx = malloc(4);
    memcpy(idx, &msg[0], 4);
    printf("index : %s\n", idx);
    char l[4];
    memcpy(l, &msg[4], 4);
    printf("length : %s\n", l);
    for(int i = 0; i < size*size; i++){
        unsigned char sub;
        memcpy(&sub, &msg[i + 8], 1);
        printf("%d ", sub);
    }printf("\n");

    //write(sockfd, msg, size*size + 8);
    send(sockfd, msg, size * size + 8, 0);

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

    char* buffer = malloc(size*size);  //TO MODIFY
    size_t bytes_read;
    int msgsize = 0;
    char N2[4];

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
                    }if(msgsize >= 5){
                        memcpy(N2, &buffer[1], 4);
                        buffer = realloc(buffer, 5+atoi(N2));
                        break;
                    }
                }//buffer[msgsize-1] = 0;
                recv(sockfd, &buffer[msgsize], atoi(N2) + 5 - msgsize,0);
                char err;
                memcpy(&err, &buffer[0], 1);
                memcpy(N2, &buffer[1], 4);
                printf("error code : %c\n", err);
                printf("N*N = %d\n", atoi(N2));
                for(int i = 0; i < atoi(N2); i++){
                    unsigned char sub;
                    memcpy(&sub, &buffer[i+5], 1);
                    printf("%d ", sub);
                }
                msgsize = 0;
                bytes_read = 0;
                close(sockfd);
                
            }clac = 0;
        }if(sec - time(NULL) != 0){
            clac = 1;
            count = 0;
            sec = time(NULL);
        }
    }//printf("%d message sent\n", i);
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