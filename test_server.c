#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
 
int main(int argc, char **argv){
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;
 
 
    // create the threads 
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_func, NULL);
    }
 
    check((server_socket = socket(AF_INET, SOCK_STREAM,0)), "Failed to create socket");
 
    // initialize the address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);
 
    check(bind(server_socket, (SA*)&server_addr, sizeof(server_addr)),"Bind failed!");
    check(listen(server_socket, SERVER_BACKLOG), "Listen failed!");
 
    while (true)
    {
        printf("Waiting for connections...\n");
 
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(server_socket, (SA*)&client_addr, (socklen_t*)&addr_size),"accept failed");
        printf("Client connected\n");
 
        int* client = malloc(sizeof(int));
        *client = client_socket;
        pthread_mutex_lock(&mutex);
        enqueue(client);
        pthread_mutex_unlock(&mutex);
 
    }
 
    return 0;
 
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
    while((bytes_read = read(client_socket, buffer+msgsize, sizeof(buffer)-msgsize-1)) > 0){
        msgsize+= bytes_read;
        if(msgsize > BUFSIZE -1 || buffer[msgsize-1] == '\n'){
            break;
        }
    }check(bytes_read, "recv error");
    buffer[msgsize-1] = 0;
    printf("Request : %s\n", buffer);


    return 0;
}