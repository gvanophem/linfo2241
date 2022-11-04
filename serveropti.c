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
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>
#define SERVER_BACKLOG 100
#define ARRAY_TYPE uint32_t

typedef struct args{
    int size;
    ARRAY_TYPE ** files;
}args_t;
 
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int check(int err, const char *msg){
    if(err == -1){
        perror(msg);
        exit(1);
    }    
    return err;
}
 
bool checksize(int size){
    int m = 1;
    for(int i = 0; i < 18; i++){
        m = m*2;
        if(m == size)
        return true;
    }return false;
}

int connection_handler(int sockfd, int nbytes, ARRAY_TYPE **pages, int npages){
    ARRAY_TYPE fileid;
    ARRAY_TYPE keysz;
    int8_t err = 0;
    int tread;
    if((tread = recv(sockfd, &fileid, 4, 0)) == -1) return -1;
    if((tread = recv(sockfd, &keysz, 4, 0))  == -1) return -1;
    keysz = ntohl(keysz);
    ARRAY_TYPE key[keysz*keysz];
    unsigned tot = keysz*keysz * sizeof(ARRAY_TYPE);
    unsigned done = 0;
    while( done < tot){
        if((tread = recv(sockfd, key, tot-done, 0)) == -1) return -1;
        done+= tread;
    }
    int nr = nbytes/keysz;
    ARRAY_TYPE* file = pages[fileid % npages];
    ARRAY_TYPE* crypted = malloc(nbytes*nbytes * sizeof(ARRAY_TYPE));
    if(crypted == NULL) return -1;

    //Compute sub-matrices
    for (int i = 0; i < nr ; i ++) {
        int vstart = i * keysz;
        for (int j = 0; j < nr; j++) {
            int hstart = j * keysz;
            //Do the sub-matrix multiplication
            for (int ln = 0; ln < keysz; ln++) {
                int aline = (vstart + ln) * nbytes + hstart;
                for (int col = 0; col < keysz; col++) {
                    int tot = 0;
                    for (int k = 0; k < keysz; k++) {
                        int vline = (vstart + k) * nbytes + hstart;
                        tot += key[ln * keysz + k] * file[vline + col];
                    }
                    crypted[aline + col] = tot;
                }
            }
        }
    }
    int check = send(sockfd, &err, 1,MSG_NOSIGNAL );
    if(check == -1) return -1;
    unsigned sz = htonl(nbytes*nbytes * sizeof(ARRAY_TYPE));
    check = send(sockfd, &sz, 4, MSG_NOSIGNAL);
    if(check == -1) return -1;
    check = send(sockfd, crypted, nbytes*nbytes * sizeof(ARRAY_TYPE),MSG_NOSIGNAL );
    if(check == -1) return -1;
    free(crypted);
    return 0;
}

int main(int argc, char** argv)
{

    int opt;

    int num_th = 1;
    int nbytes;
    int port;
    int verbose = 0;

    while ((opt = getopt(argc, argv, ":j:s:p:v")) != -1) {
        switch (opt) {
        case 'j':
            num_th = atoi(optarg);
            break;
        case 's':
            nbytes = atoi(optarg);
            if(checksize(nbytes) == false){
                printf("Argument in -s must be a power of 2 less or equal to 131072.\n");
                exit(0);
            }
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-j threads] [-s bytes] [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

    }

    printf("getopt ok \n");

    int npages = 1000;
    ARRAY_TYPE** pages;
    pages = (ARRAY_TYPE**)malloc(sizeof(ARRAY_TYPE*) * npages);
    if(pages == NULL){
        printf("pages creation failed\n");
        exit(0);
    }
    printf("pages malloc ok\n");
    for(int i = 0; i < npages; i++){
        pages[i] = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
        if(pages[i] == NULL){
            printf("pages[i] creation failed\n");
            exit(0);
        }
    }

    printf("creation pages ok\n");

    // matrix** files = malloc(1000*sizeof(matrix)); 
    // for (int i = 0; i < 1000; i++)
    // {
    //     files[i] = allocate_matrix(nbytes,nbytes);
    //     for (int j = 0; j < nbytes*nbytes; j++)
    //     {
    //         files[i]->data[j] = (char) rand()%128;
    //     }
    // }

    int sockfd, connfd, len;
    SA_IN servaddr, client_addr;

    pthread_t thread_pool[num_th];

    args_t* arguments = malloc(sizeof(args_t));
    if(arguments == NULL){
        printf("argument creation failed\n");
        exit(0);
    }
    arguments->files = pages;
    arguments->size = nbytes;

    // for (int i = 0; i < num_th; i++)
    // {
    //     pthread_create(&thread_pool[i], NULL, thread_func, (void*)arguments);
    // }
   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    memset(&servaddr, 0, sizeof(servaddr));
   
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
   
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    if ((listen(sockfd, 128)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(client_addr);

    int client_socket, addr_size;
    while((client_socket = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len))){
        check(connection_handler(client_socket, nbytes, pages, npages), "Failed to handle the connection\n");
    }

    // while (true)
    // {
    //     addr_size = sizeof(SA_IN);
    //     check(client_socket = accept(sockfd, (SA*)&client_addr, (socklen_t*)&addr_size),"accept failed");
 
    //     int* client = malloc(sizeof(int));
    //     *client = client_socket;
    //     pthread_mutex_lock(&mutex);
    //     enqueue(client);
    //     pthread_mutex_unlock(&mutex);
 
    // }

    close(sockfd);
}