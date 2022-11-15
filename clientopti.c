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
    char* sub = malloc(sizeof(char)*(len+1));
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
        key->data[i] = (unsigned char)modulo(random, 256);
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
    char* val = malloc(sizeof(char));
    for(int i = 0; i < size * size; i++){
        sprintf(val, "%c", key->data[i]);
        memcpy(&msg[written_bytes], val, 1);
        written_bytes += 1;
    }
    send(sockfd, msg, size * size + 8, 0);
 
}
 
int create_socket(struct sockaddr_in servaddr){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(sock, (SA*)&servaddr, sizeof(servaddr)) != 0){
        printf("bind failed\n");
        exit(0);
    }return sock;
}
 
void send_and_receive(int sockfd, int size){
    char* buffer = malloc(size*size);
    size_t bytes_read = 0;
    int msgsize = 0;
    char N2[4];
    send_message(sockfd,size);
    while((bytes_read = read(sockfd, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0){
        msgsize+= bytes_read;
        if(msgsize > size * size + 5 -1 || buffer[msgsize-1] == '\n'){
            break;
        }if(msgsize >= 5){
            memcpy(N2, &buffer[1], 4);
            buffer = realloc(buffer, 5+atoi(N2));
            break;
        }
    }recv(sockfd, &buffer[msgsize], atoi(N2) + 5 - msgsize,0);
}
 
void* thread_func(void* args){
    args_t* arguments = (args_t*)args;
    struct sockaddr_in servaddr = arguments->server_addr;
    time_t begin = time(NULL);
    time_t sec = time(NULL);
    int clac = 1;
    int max_time = arguments->max_time;
    int size = arguments->size;
    int rate = arguments->rate;
    clock_t start = arguments->start;
    int count = 0;
    double time_spent;
    double time_spent_since_start;
    while(difftime(time(NULL), begin) < max_time){
        if(count >= max_time) return NULL;
        if(clac == 1){
            int sockfd = create_socket(servaddr);
            printf("Request sending\n");
            clock_t begin = clock();
            send_and_receive(sockfd, size);
            clock_t end = clock();
            time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
            // time_spent_since_start = (double)(begin-start)/CLOCKS_PER_SEC;
            pthread_mutex_lock(&mutex);
            FILE* times = fopen("times_ref.txt", "a");
            fprintf(times, "%f\n", time_spent);
            fclose(times);
            pthread_mutex_unlock(&mutex);
            close(sockfd);
            clac = 0;
            count++;
        }else if(difftime(time(NULL), sec) != 0){
            clac = 1;
            sec = time(NULL);
        } 
    }//if pour terminer si un thread a perdu trop de temps.
    if(count < max_time){
        for(int i = 0; i < max_time-count; i++){
            int sockfd = create_socket(servaddr);
            printf("Request sending\n");
            send_and_receive(sockfd, size);
            close(sockfd);
        }
    }
}
 
int check(int err, const char *msg){
    if(err == -1){
        perror(msg);
        exit(1);
    }    
    return err;
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
    printf("index : %d\n", ((int)(*t))%(max_time*rate));
    receive_times[((int)(*t))%(max_time*rate)] = getts();
    close(sockfd);
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
        if(i == 100) break;
        while ((int)(getts() - next) < 0) {
            usleep((int)(next - getts()) / 1000);
        }
        pthread_t thread;
        arguments->t = realloc(arguments->t, sizeof(int));
        pthread_mutex_lock(&mutex);
        int mine = i;
        i++;
        *(arguments->t) = mine;
        pthread_mutex_unlock(&mutex);
        sent_times[i%(rate*max_time)] = getts();
        pthread_create( &thread, NULL, rcv, (void*)arguments);        
    }
}