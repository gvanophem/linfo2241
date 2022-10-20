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
#define MAX 80
#define PORT 8080
//#define SA struct sockaddr

#define SERVERPORT 8080
#define BUFSIZE 50//todo
#define THREAD_POOL_SIZE 20
#define SERVER_BACKLOG 100
 
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
 
void* handle_connection(void* p_client_socket);
int check(int exp, const char *msg);
void* thread_func(void* arg);

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

matrix* encrypt(char*buff, matrix** files, int size){
    matrix* mat = allocate_matrix(size,size);
    return mat;
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
   
// Function designed for chat between client and server.
void func(int connfd, int size, matrix** files)
{
    char buff[size];
    int n;
    // infinite loop for chat
    for (int i = 0; i < 20; i++) {
        bzero(buff, size);
   
        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("From client: %s\n", buff);

        matrix* encrypted = encrypt(buff, files, size);

        char* msg = (char*)malloc(sizeof(char)*size+5);
        int index = 0;
        msg[index] = '0';
        index++;
        int tm = size+5;

        char* sent_size = (char*)&tm;
        memcpy(&msg[index], sent_size, 4);
        index += 4;
        memcpy(&msg[index], (char*)encrypted->data, size);

        write(connfd, msg, size+5);

        // bzero(buff, MAX);
        // n = 0;
        // // copy server message in the buffer
        // while ((buff[n++] = getchar()) != '\n')
        //     ;
   
        // // and send that buffer to client
        // write(connfd, buff, sizeof(buff));
   
        // // if msg contains "Exit" then server exit and chat ended.
        // if (strncmp("exit", buff, 4) == 0) {
        //     printf("Server Exit...\n");
        //     break;
        // }
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
    while (true)
    {
        int* client;
        pthread_mutex_lock(&mutex);
        client = dequeue();
        pthread_mutex_unlock(&mutex);
 
        if(client != NULL){
            handle_connection(client); 
        }
    }
 
}
 
void* handle_connection(void* client_socket){
    char buffer[BUFSIZE];
    size_t bytes_read;
    int msgsize = 0;
    while((bytes_read = read((int*)client_socket, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0){
        msgsize+= bytes_read;
        if(msgsize > BUFSIZE -1 || buffer[msgsize-1] == '\n'){
            break;
        }
    }check(bytes_read, "recv error");
    buffer[msgsize-1] = 0;
    printf("Request : %s\n", buffer);


    return 0;
}


// Driver function
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
            //size = (int)*optarg;
            break;
        case 's':
            size = atoi(optarg);
            //rate = (int)*optarg;
            break;
        case 'p':
            port = atoi(optarg);
            //time = (int)*optarg;
            //sscanf(optarg, "%d", time);
            break;
        default:
            printf("erreur\n");
            break;
        }

    }

    // generate files (only needs command line arguments, to do at the beginning, when server is launched)
    matrix** files = malloc(1000*sizeof(matrix)); 
    for (int i = 0; i < 1000; i++)
    {
        files[i] = allocate_matrix(size,size);
        for (int j = 0; j < size*size; j++)
        {
            files[i]->data[j] = rand()%256;
        }
    }

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_func, NULL);
    }
   
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    int client_socket, addr_size;

    while (true)
    {
        printf("Waiting for connections...\n");
 
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(sockfd, (SA*)&client_addr, (socklen_t*)&addr_size),"accept failed");
        printf("Client connected\n");
 
        int* client = malloc(sizeof(int));
        *client = client_socket;
        pthread_mutex_lock(&mutex);
        enqueue(client);
        pthread_mutex_unlock(&mutex);
 
    }
   
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");
   
    // Function for chatting between client and server
    //func(connfd, size, files);
   
    // After chatting close the socket
    close(sockfd);
}