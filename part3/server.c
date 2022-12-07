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
#define ARRAY_TYPE uint32_t
 
typedef struct args{
    int size;
    ARRAY_TYPE ** files;
}args_t;
 
bool DOPTIM = true;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
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

void mat_mul(ARRAY_TYPE* file, ARRAY_TYPE* key, ARRAY_TYPE* crypt, int nbytes, int keysz){
    int nr = nbytes/keysz;
    // ARRAY_TYPE* crypt = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
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
                    crypt[aline + col] = tot;
                }
            }
        }
    }
}

int connection_handler(int sockfd, int nbytes, ARRAY_TYPE **pages, int npages){
    ARRAY_TYPE fileid;
    ARRAY_TYPE keysz;
    int8_t err = 0;
    int tread;
    if((tread = recv(sockfd, &fileid, 4, 0)) == -1) return -1;
    if((tread = recv(sockfd, &keysz, 4, 0))  == -1) return -1;
    keysz = ntohl(keysz);
    //printf("key size : %d\n", keysz);
    ARRAY_TYPE* key;
    if(DOPTIM) key = (ARRAY_TYPE*)aligned_alloc(64, keysz*keysz*sizeof(ARRAY_TYPE));
    else key = (ARRAY_TYPE*)malloc(keysz*keysz*sizeof(ARRAY_TYPE));
    unsigned tot = keysz*keysz * sizeof(ARRAY_TYPE);
    unsigned done = 0;
    while( done < tot){
        if((tread = recv(sockfd, key, tot-done, 0)) == -1) return -1;
        done+= tread;
    }
    // for(int i = 0; i < keysz*keysz; i++){
    //     printf("%d ", key[i]);
    // }printf("\n");
    int nr = nbytes/keysz;
    ARRAY_TYPE* file = pages[fileid % npages];
    ARRAY_TYPE* crypted;
    if(DOPTIM) crypted = aligned_alloc(64, nbytes*nbytes * sizeof(ARRAY_TYPE));
    else crypted = malloc(sizeof(ARRAY_TYPE) * nbytes * nbytes);
    if(crypted == NULL) return -1;

    if(DOPTIM){
        int nr = nbytes/keysz;
 
        for(int i = 0; i < nr; i++){
            int istart = i * keysz;
            for(int ln = 0; ln < keysz; ln++){
                for(int col = 0; col < keysz; col++){
                    ARRAY_TYPE r = key[ln*keysz + col]; //peut etre utilisé file plutôt ici, file[(istart + ln) * nbytes + jstart + col]
                    for(int k = 0; k < nbytes; k+=8){
                        // printf("%d\n", crypt[(istart + ln) * nbytes + (k)]);
                        // printf("matmul at key[%d,%d] and file[%d,%d]. Writing in crypt[%d,%d]\n", ln, col, istart+col, k, istart + ln, k);
                        crypted[(istart + ln) * nbytes + (k+0)] += r * file[(istart + col) * nbytes + (k+0)];
                        crypted[(istart + ln) * nbytes + (k+1)] += r * file[(istart + col) * nbytes + (k+1)];
                        crypted[(istart + ln) * nbytes + (k+2)] += r * file[(istart + col) * nbytes + (k+2)];
                        crypted[(istart + ln) * nbytes + (k+3)] += r * file[(istart + col) * nbytes + (k+3)];
                        crypted[(istart + ln) * nbytes + (k+4)] += r * file[(istart + col) * nbytes + (k+4)];
                        crypted[(istart + ln) * nbytes + (k+5)] += r * file[(istart + col) * nbytes + (k+5)];
                        crypted[(istart + ln) * nbytes + (k+6)] += r * file[(istart + col) * nbytes + (k+6)];
                        crypted[(istart + ln) * nbytes + (k+7)] += r * file[(istart + col) * nbytes + (k+7)]; 
                    }
                }
            }
        }
    }

    else{
        mat_mul(file, key, crypted, nbytes, keysz);
    }

    int check = send(sockfd, &err, 1,MSG_NOSIGNAL );
    if(check == -1) return -1;
    unsigned sz = htonl(nbytes*nbytes * sizeof(ARRAY_TYPE));
    //printf("sent key bytes : %d\n", (int)(nbytes*nbytes*sizeof(ARRAY_TYPE)));
    check = send(sockfd, &sz, 4, MSG_NOSIGNAL);
    if(check == -1) return -1;
    // int i;
    // for(i = 0; i < nbytes*nbytes; i++){
    //     printf("%d ", crypted[i]);
    // }printf("\n%d\n", i);
    check = send(sockfd, crypted, nbytes*nbytes * sizeof(ARRAY_TYPE),MSG_NOSIGNAL );
    if(check == -1) return -1;
    free(crypted);
    free(key);
    return 0;
}
 
int main(int argc, char** argv)
{
 
    int opt;
 
    int num_th = 1;
    int nbytes = 1024;
    int port = 2241;
    int verbose = 0;
 
    while ((opt = getopt(argc, argv, ":j:s:p:v:d:?")) != -1) {
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
        case 'd':
            DOPTIM = (bool)atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-j threads] [-s bytes] [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
 
    }
 
    int npages = 1000;
    ARRAY_TYPE** pages;
    if(DOPTIM) pages = (ARRAY_TYPE**)aligned_alloc(64, sizeof(ARRAY_TYPE*) * npages);
    else pages = (ARRAY_TYPE**)malloc(sizeof(ARRAY_TYPE*) * npages);
    if(pages == NULL){
        printf("pages creation failed\n");
        exit(0);
    }
    for(int i = 0; i < npages; i++){
        if(DOPTIM) pages[i] = aligned_alloc(64, sizeof(ARRAY_TYPE)*nbytes*nbytes);
        else pages[i] = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
        if(pages[i] == NULL){
            printf("pages[i] creation failed\n");
            exit(0);
        }
    }for(unsigned i = 0; i < nbytes*nbytes; i++){
        pages[0][i] = i;
    }
 
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
    memset(&servaddr, 0, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
 
    if ((listen(sockfd, 128)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    len = sizeof(client_addr);
 
    int i = 0;

    int client_socket, addr_size;
    while((client_socket = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len))){
        //printf("connection number %d entered...\n", i); i++;
        check(connection_handler(client_socket, nbytes, pages, npages), "Failed to handle the connection\n");
    }

    for(int i = 0; i < npages; i++){
        free(pages[i]);
    }free(pages);

    
 
    close(sockfd);
}