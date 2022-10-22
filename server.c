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

typedef struct {
int m, n; // dimensions de la matrice
	unsigned char *data; // tableau 1D de taille m*n contenant les entrées de la matrice
	unsigned char **a; // tableau 1D de m pointeurs vers chaque ligne, pour pouvoir appeler a[i][j]
} matrix;

typedef struct args{
    int size;
    matrix** files;
}args_t;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
 
void* handle_connection(void* client_socket, matrix** files, int size);
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
 
 
void* thread_func(void *arg) {
    args_t* arguments = (args_t*)arg;
    while (true)
    {
        int* client;
        pthread_mutex_lock(&mutex);
        client = dequeue();
        pthread_mutex_unlock(&mutex);
 
        if(client != NULL){
            printf("starting to handle the connection\n");
            handle_connection(client, arguments->files, arguments->size); 
        }
    }
}
 
void* handle_connection(void* client_socket, matrix** files, int size){
    char buffer[size * size + 8];
    recv(*((int*)client_socket), buffer, size*size + 8, 0);
    char idx[4];
    memcpy(idx, &buffer[0], 4);
    char l[4];
    memcpy(l, &buffer[4], 4);
    int N = atoi(l);
    for(int i = 0; i < N * N; i++){
        unsigned char sub;
        memcpy(&sub, &buffer[i + 8], 1);
    }

    //buffer contains the message. Now let's encrypt this message

    matrix* encrypted = encrypt(buffer, files, size);

    char* msg = (char*)malloc(sizeof(char)*size*size+5);
    int index = 0;
    msg[index] = '0';
    index++;
    int tm = size*size;

    char sent_size[4];
    sprintf(sent_size, "%d", tm);
    memcpy(&msg[index], sent_size, 4);
    index += 4;
    unsigned char val;
    for(int i = 0; i < size * size; i++){
        sprintf(&val, "%c",encrypted->data[i]);
        memcpy(&msg[index], &val, 1);
        index += 1;
    }
    send(*((int*)client_socket), msg, size*size+5, 0);

    return 0;
}

bool checksize(int size){
    int m = 1;
    for(int i = 0; i < 18; i++){
        m = m*2;
        if(m == size)
        return true;
    }return false;
}

int main(int argc, char** argv)
{

    int opt;

    int num_th;
    int size;
    int port;

    while ((opt = getopt(argc, argv, ":j:s:p:?")) != -1) {
        switch (opt) {
        case 'j':
            num_th = atoi(optarg);
            break;
        case 's':
            size = atoi(optarg);
            if(checksize(size) == false){
                printf("Argument in -s must be a power of 2 less or equal to 131072.\n");
                exit(0);
            }
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            printf("erreur\n");
            break;
        }

    }

    matrix** files = malloc(1000*sizeof(matrix)); 
    for (int i = 0; i < 1000; i++)
    {
        files[i] = allocate_matrix(size,size);
        for (int j = 0; j < size*size; j++)
        {
            files[i]->data[j] = (char) rand()%128;
        }
    }

    int sockfd, connfd, len;
    SA_IN servaddr, client_addr;

    pthread_t thread_pool[num_th];

    args_t* arguments = malloc(sizeof(args_t));
    arguments->files = files;
    arguments->size = size;

    for (int i = 0; i < num_th; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_func, (void*)arguments);
    }
   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
   
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    if ((listen(sockfd, SERVER_BACKLOG)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(client_addr);

    int client_socket, addr_size;

    while (true)
    {
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(sockfd, (SA*)&client_addr, (socklen_t*)&addr_size),"accept failed");
 
        int* client = malloc(sizeof(int));
        *client = client_socket;
        pthread_mutex_lock(&mutex);
        enqueue(client);
        pthread_mutex_unlock(&mutex);
 
    }
    close(sockfd);
}