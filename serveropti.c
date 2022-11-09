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
 
typedef struct {
int m, n; // dimensions de la matrice
	unsigned char *data; // tableau 1D de taille m*n contenant les entrées de la matrice
	unsigned char **a; // tableau 1D de m pointeurs vers chaque ligne, pour pouvoir appeler a[i][j]
} matrix;
 
typedef struct args{
    int size;
    ARRAY_TYPE ** files;
}args_t;
 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
 
int check(int exp, const char *msg);
void* thread_func(void *arg);
 
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
 
 
// iCoord and jCoord are the coordinates of the first element of the submatrix
int mult_sub_matrix(matrix* key, matrix* file, matrix* encrypted, int N, int iCoord, int jCoord) {
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            int sum = 0;
            for (int pos = 0; pos < N; pos++)
            {
                sum += (int)key->a[i][pos] * (int)file->a[pos+iCoord][j+jCoord];
            }
            encrypted->a[i+iCoord][j+jCoord] = (unsigned char) sum%256;
        }
    }
}
 
char* substr(char* src, int start, int len){
    char* sub = (char*)malloc(len+1);
    if(sub==NULL) printf("bug\n");
    memcpy(sub, &src[start], len);
    sub[len] = '\0';
    return sub;
}
 
matrix* encrypt(unsigned char* buf, matrix** files, int size){
    int index = atoi(substr(buf,0,4));
    int N  = atoi(substr(buf,4,4));
    if(size%N != 0){
        printf("The key size must divide the file size, up to the file size itself.\n");
        exit(0);
    }
 
    matrix* key = allocate_matrix(N,N);
    for (int i = 0; i < N*N; i++)
    {
        key->data[i] = substr(buf,i+8,1)[0];
    }
 
    //print_matrix(key,N);
 
    matrix* encrypted = allocate_matrix(size,size);
    for (int i = 0; i < size; i+=N){
        for (int j = 0; j < size; j+=N){ 
            mult_sub_matrix(key, files[index],encrypted,N,i,j);
        }
    }
    return encrypted;
}
 
struct node {
    struct node* next;
    int *client_socket;
};
typedef struct node node_t;
 
node_t* head = NULL;
node_t* tail = NULL;
 
void enqueue(int* client_socket){
    node_t* new_node = malloc(sizeof(node_t));
    new_node->client_socket = client_socket;
    new_node->next = NULL;
    if(tail == NULL){
        head = new_node;
    }else{
        tail->next = new_node;
    }tail = new_node;
}
 
int* dequeue(){
    if(head == NULL) return NULL;
    else{
        int* result = head->client_socket;
        node_t* temp = head;
        head = head->next;
        if(head == NULL) tail = NULL;
        free(temp);
        return result;
    }
}
 
 
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
 
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
 
int connection_handler(int sockfd, int nbytes, ARRAY_TYPE **pages, int npages){
    ARRAY_TYPE fileid;
    ARRAY_TYPE keysz;
    int8_t err = 0;
    int tread;
    if((tread = recv(sockfd, &fileid, 4, 0)) == -1) return -1;
    if((tread = recv(sockfd, &keysz, 4, 0))  == -1) return -1;
    keysz = ntohl(keysz);
    printf("key size : %d\n", keysz);
    ARRAY_TYPE key[keysz*keysz];
    unsigned tot = keysz*keysz * sizeof(ARRAY_TYPE);
    unsigned done = 0;
    while( done < tot){
        if((tread = recv(sockfd, key, tot-done, 0)) == -1) return -1;
        done+= tread;
    }
    for(int i = 0; i < keysz*keysz; i++){
        printf("%d ", key[i]);
    }printf("\n");
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
                    crypted[aline + col] = (ARRAY_TYPE)tot;
                }
            }
        }
    }

    int check = send(sockfd, &err, 1,MSG_NOSIGNAL );
    if(check == -1) return -1;
    unsigned sz = htonl(nbytes*nbytes * sizeof(ARRAY_TYPE));
    printf("sent key bytes : %d\n", (int)(nbytes*nbytes*sizeof(ARRAY_TYPE)));
    check = send(sockfd, &sz, 4, MSG_NOSIGNAL);
    if(check == -1) return -1;
    int i;
    for(i = 0; i < nbytes*nbytes; i++){
        printf("%d ", crypted[i]);
    }printf("\n%d\n", i);
    check = send(sockfd, crypted, nbytes*nbytes * sizeof(ARRAY_TYPE),MSG_NOSIGNAL );
    if(check == -1) return -1;
    free(crypted);
    return 0;
}
 
int main(int argc, char** argv)
{
 
    int opt;
 
    int num_th = 1;
    int nbytes = 8;
    int port = 2241;
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
 
    printf("nbytes : %d\n", nbytes);
 
    int npages = 1000;
    ARRAY_TYPE** pages;
    pages = (ARRAY_TYPE**)malloc(sizeof(ARRAY_TYPE*) * npages);
    if(pages == NULL){
        printf("pages creation failed\n");
        exit(0);
    }
    for(int i = 0; i < npages; i++){
        pages[i] = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
        if(pages[i] == NULL){
            printf("pages[i] creation failed\n");
            exit(0);
        }for(int j = 0; j < nbytes*nbytes; j++){
            pages[i][j] = i + j;
        }
    }
 
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
        printf("connection entered...\n");
        check(connection_handler(client_socket, nbytes, pages, npages), "Failed to handle the connection\n");
    }
    // pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
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