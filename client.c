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
#define SA struct sockaddr
#define ARRAY_TYPE uint32_t
#define BILLION 1000000000
 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
int64_t *receive_times; // en ns

int64_t getts(){
    struct timespec ts;
    int err = clock_gettime(CLOCK_MONOTONIC, &ts);
    if(err == -1){
        printf("getts failed\n");
        exit(0);
    }
    int64_t tns = ts.tv_sec*1000000000 + ts.tv_nsec;
    return tns;
}
 
typedef struct args{
    struct sockaddr_in server_addr;
    int max_time;
    int size;
    clock_t start;
    int rate;
    char* port;
    int npages;
    int keysz;
    int *t;
}args_t;
 
void print_matrix(ARRAY_TYPE* mat, int dim){
    for(int i = 0; i < dim; i++){
        for(int j = 0; j < dim; j++){
        printf("%4d ", mat[i*dim+j]);
        }
        printf("\n");
    }
}
int check(int err, const char *msg){
    if(err == -1){
        perror(msg);
        exit(1);
    }    
    return err;
}

char modulo(int random, int mod){
    unsigned char c = random%mod;
    if((int)c < 0){
        c = (int)c + mod;
    }
    return c;
}
 
void* rcv(void* r) {
    args_t* arguments = (args_t*)r;
    struct sockaddr_in servaddr = arguments->server_addr;
    char* port = arguments->port;
    int npages = arguments->npages;
    int keysz = arguments->keysz;
    int nbytes = arguments->size;
    int *t = arguments->t;
    int rate = arguments->rate;
    int max_time = arguments->max_time;
 
    int ret;
    int sockfd;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    check(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)), "connection failed...\n");
    //Send file id
    unsigned fileindex = htonl(rand() % npages);
    ret = send(sockfd, &fileindex, 4, 0);
    //Send key size
    //printf("key size : %d\n", keysz);
    int revkey = htonl(keysz);
    ret = send(sockfd, &revkey, 4, 0);
    //Send key
    ARRAY_TYPE* key = malloc(keysz*keysz * sizeof(ARRAY_TYPE));
    srand(time(0));
    for (int i = 0; i < nbytes*nbytes; i++)
    {
        int random = rand();
        key[i] = (ARRAY_TYPE)modulo(random, 100000);
        //printf("%d ", key[i]);
    }//printf("\n");
    ret = send(sockfd, key, sizeof(ARRAY_TYPE) * keysz*keysz, 0);
    unsigned char error;
    recv(sockfd, &error, 1, 0);
    //printf("error code : %d\n", error);
    unsigned filesz;
    recv(sockfd, &filesz, 4, 0);
    filesz = ntohl(filesz);
    //printf("file size : %d\n", (int)(filesz/sizeof(ARRAY_TYPE)));
    if (filesz > 0) {
        ARRAY_TYPE buffer[filesz/sizeof(ARRAY_TYPE)];
        unsigned tot = filesz;
        unsigned done = 0;
        int tread;
        while( done < tot){
            if((tread = recv(sockfd, buffer, tot-done, 0)) == -1) return NULL;
            done+= tread;
        }
    }
    printf("reception done for the %dth time\n", *t);
    receive_times[((int)(*t))%(max_time*rate)] = getts();
    close(sockfd);
}

void print_times(int64_t *times, int l){
    for(int i = 0; i < l; i++){
        printf("%lu ", (times[i]));
    }printf("\n");
}
 
int main(int argc, char** argv)
{
    int opt;
 
    int size = 4;
    int rate = 1;
    int max_time = 1;
 
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

    printf("file size : %d\n", size);
 
    int npages = 1000;

    char* str = malloc(strlen(argv[argc-1]));
    strcpy(str, argv[argc - 1]);
 
    char* token = strtok(str, ":");
    char* address = malloc(sizeof(char)*strlen(token));
    memcpy(address, token, strlen(token));
    token = strtok(NULL, ":");
    char* port = malloc(strlen(token));
    memcpy(port, token, strlen(token));
 
 
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(atoi(port));
 
 
    args_t* arguments = malloc(sizeof(args_t));
    arguments->server_addr = servaddr;
    arguments->max_time = max_time;
    arguments->size = size;
    arguments->rate = rate;
    arguments->port = port;
    arguments->npages = npages;
    arguments->keysz = size;
    arguments->t = malloc(sizeof(int));
    
    int64_t * sent_times = malloc(sizeof(int64_t) * rate * max_time);
    if(sent_times == NULL) return -1;
    receive_times = malloc(sizeof(int64_t) * rate * max_time);
    int64_t diffrate = 1000000000 / rate;
    int64_t start = getts();
    int64_t next = 0;
    int i = 0;
    while (getts() - start < (long unsigned)1000000000 * max_time) {
        next += diffrate;
        while ((int)(getts() - next) < 0) {
            usleep((int)(next - getts()) / 1000);
        }
        pthread_t thread;
        arguments->t = realloc(arguments->t, sizeof(int));
        pthread_mutex_lock(&mutex);
        int mine = i;
        sent_times[i%(rate*max_time)] = getts();
        i++;
        *(arguments->t) = mine;
        pthread_mutex_unlock(&mutex);
        pthread_create( &thread, NULL, rcv, (void*)arguments); 
        pthread_join(thread, NULL);       
    }

    printf("---------Sent times[ns]---------\n");
    print_times(sent_times, rate * max_time);
    printf("--------Receive times[ns]-------\n");
    print_times(receive_times, rate*max_time);
    int64_t *computation_times = malloc(sizeof(int64_t) * rate * max_time);
    for(int i = 0; i < max_time*rate; i++){
        computation_times[i] = receive_times[i] - sent_times[i];
    }
    printf("------Computation times[ns]-----\n");
    print_times(computation_times, rate*max_time);

    // free(receive_times);
    // free(sent_times);
    // free(computation_times);

    return 0;
}