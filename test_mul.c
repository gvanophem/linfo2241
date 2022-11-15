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
 
 
void print_matrix(ARRAY_TYPE* mat, int dim){
    for(int i = 0; i < dim; i++){
        for(int j = 0; j < dim; j++){
        printf("%4d ", mat[i*dim+j]);
        }
        printf("\n");
    }
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
 
void line_mul(ARRAY_TYPE* file, ARRAY_TYPE* key, ARRAY_TYPE* crypt, int nbytes, int keysz){
    int nr = nbytes/keysz;
 
    for(int i = 0; i < nr; i++){
        int istart = i * keysz;
        for(int j = 0; j < nr; j++){
            int jstart = j * keysz;
            for(int ln = 0; ln < keysz; ln++){
                for(int col = 0; col < keysz; col++){
                    ARRAY_TYPE r = key[ln*keysz + col]; //peut etre utilisé file plutôt ici, file[(istart + ln) * nbytes + jstart + col]
                    for(int k = 0; k < keysz; k++){
                        //printf("matmul at key[%d,%d] and file[%d,%d]. Writing in crypt[%d,%d]\n", ln, col, istart+col, jstart + k, istart + ln, jstart + k);
                        crypt[(istart + ln) * nbytes + (jstart + k)] += r * file[(istart + col) * nbytes + (jstart + k)];
                    }
                }
            }
        }
    }
}
 
void line_mul_unrolled(ARRAY_TYPE* file, ARRAY_TYPE* key, ARRAY_TYPE* crypt, int nbytes, int keysz){
    int nr = nbytes/keysz;
    // ARRAY_TYPE* crypt = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
 
    for(int i = 0; i < nr; i++){
        int istart = i * keysz;
        for(int j = 0; j < nr; j++){
            int jstart = j * keysz;
            for(int ln = 0; ln < keysz; ln++){
                for(int col = 0; col < keysz; col++){
                    ARRAY_TYPE r = key[ln*keysz + col]; //peut etre utilisé file plutôt ici, file[(istart + ln) * nbytes + jstart + col]
                    for(int k = 0; k < keysz; k+=8){
                        //printf("matmul at key[%d,%d] and file[%d,%d]. Writing in crypt[%d,%d]\n", ln, col, istart+col, jstart + k, istart + ln, jstart + k);
                        crypt[(istart + ln) * nbytes + (jstart + k)] += r * file[(istart + col) * nbytes + (jstart + k)];
                        crypt[(istart + ln) * nbytes + (jstart + k+1)] += r * file[(istart + col) * nbytes + (jstart + k+1)];
                        crypt[(istart + ln) * nbytes + (jstart + k+2)] += r * file[(istart + col) * nbytes + (jstart + k+2)];
                        crypt[(istart + ln) * nbytes + (jstart + k+3)] += r * file[(istart + col) * nbytes + (jstart + k+3)];
                        crypt[(istart + ln) * nbytes + (jstart + k+4)] += r * file[(istart + col) * nbytes + (jstart + k+4)];
                        crypt[(istart + ln) * nbytes + (jstart + k+5)] += r * file[(istart + col) * nbytes + (jstart + k+5)];
                        crypt[(istart + ln) * nbytes + (jstart + k+6)] += r * file[(istart + col) * nbytes + (jstart + k+6)];
                        crypt[(istart + ln) * nbytes + (jstart + k+7)] += r * file[(istart + col) * nbytes + (jstart + k+7)];
                    }
                }
            }
        }
    }
}
 
 
 
 
 
 
 
 
void line_mul_line(ARRAY_TYPE* file, ARRAY_TYPE* key, ARRAY_TYPE* crypt, int nbytes, int keysz){
    //printf("line by line started \n");
    int nr = nbytes/keysz;
 
    //print_matrix(crypt,nbytes);
    //printf("line by line malloced \n");
 
    for(int i = 0; i < nr; i++){
        int istart = i * keysz;
        for(int ln = 0; ln < keysz; ln++){
            for(int col = 0; col < keysz; col++){
                ARRAY_TYPE r = key[ln*keysz + col]; //peut etre utilisé file plutôt ici, file[(istart + ln) * nbytes + jstart + col]
                for(int k = 0; k < nbytes; k++){
                    // printf("%d\n", crypt[(istart + ln) * nbytes + (k)]);
                    // printf("matmul at key[%d,%d] and file[%d,%d]. Writing in crypt[%d,%d]\n", ln, col, istart+col, k, istart + ln, k);
                    crypt[(istart + ln) * nbytes + (k)] += r * file[(istart + col) * nbytes + (k)];
                }
            }
        }
    }
}
 
void line_mul_line_unrolled(ARRAY_TYPE* file, ARRAY_TYPE* key, ARRAY_TYPE* crypt, int nbytes, int keysz){
    //printf("line by line started \n");
    int nr = nbytes/keysz;
 
    //print_matrix(crypt,nbytes);
    //printf("line by line malloced \n");
 
    for(int i = 0; i < nr; i++){
        int istart = i * keysz;
        for(int ln = 0; ln < keysz; ln++){
            for(int col = 0; col < keysz; col++){
                ARRAY_TYPE r = key[ln*keysz + col]; //peut etre utilisé file plutôt ici, file[(istart + ln) * nbytes + jstart + col]
                for(int k = 0; k < nbytes; k+=8){
                    // printf("%d\n", crypt[(istart + ln) * nbytes + (k)]);
                    // printf("matmul at key[%d,%d] and file[%d,%d]. Writing in crypt[%d,%d]\n", ln, col, istart+col, k, istart + ln, k);
                    crypt[(istart + ln) * nbytes + (k+0)] += r * file[(istart + col) * nbytes + (k+0)];
                    crypt[(istart + ln) * nbytes + (k+1)] += r * file[(istart + col) * nbytes + (k+1)];
                    crypt[(istart + ln) * nbytes + (k+2)] += r * file[(istart + col) * nbytes + (k+2)];
                    crypt[(istart + ln) * nbytes + (k+3)] += r * file[(istart + col) * nbytes + (k+3)];
                    crypt[(istart + ln) * nbytes + (k+4)] += r * file[(istart + col) * nbytes + (k+4)];
                    crypt[(istart + ln) * nbytes + (k+5)] += r * file[(istart + col) * nbytes + (k+5)];
                    crypt[(istart + ln) * nbytes + (k+6)] += r * file[(istart + col) * nbytes + (k+6)];
                    crypt[(istart + ln) * nbytes + (k+7)] += r * file[(istart + col) * nbytes + (k+7)]; 
                }
            }
        }
    }
}
 
 
void clear_crypt(ARRAY_TYPE* crypt, int len){
    for(int i = 0; i<len; i++){
        crypt[i] = 0;
    }
}
 
int main(int argc, char const *argv[])
{
    int keysz = 128;
    int filesz = 1024;
 
    // generate key
    ARRAY_TYPE* key = malloc(sizeof(ARRAY_TYPE) * keysz*keysz);
    for(int i = 0; i < keysz*keysz; i++){
        key[i] = i % 13;
    }
    //print_matrix(key,keysz);
 
    // generate file
    ARRAY_TYPE* file = malloc(sizeof(ARRAY_TYPE) * filesz * filesz);
    for(int j = 0; j < filesz * filesz; j++){
        file[j] = j * 2;
    }
    //print_matrix(file,filesz);
 
    // allocate crypt
    ARRAY_TYPE* crypt = calloc(filesz*filesz, sizeof(ARRAY_TYPE));
    if(crypt == NULL){
        printf("failed to alloc\n");
        exit(0);
    }
 
    // with mat mul
    clock_t begin, end;
    double diff;
    begin = clock();
    mat_mul(file, key, crypt, filesz, keysz);  
    end = clock();
    diff = (double) (end-begin)/CLOCKS_PER_SEC;
    printf("Execution time of mat_mul : %f\n", diff);
    //print_matrix(crypt,filesz);
 
    clear_crypt(crypt, filesz*filesz);
 
    // with line mul
    clock_t beg = clock();
    line_mul(file, key, crypt, filesz, keysz);
    clock_t finish = clock();
    double line;
    line = (double)(finish- beg)/CLOCKS_PER_SEC;
    printf("Execution time of line_mul : %f\n", line);
    //print_matrix(crypt,filesz);
 
    clear_crypt(crypt, filesz*filesz);
 
    // with line mul2
    clock_t beg2 = clock();
    line_mul_unrolled(file, key, crypt, filesz, keysz);
    clock_t finish2 = clock();
    double line2;
    line2 = (double)(finish2- beg2)/CLOCKS_PER_SEC;
    printf("Execution time of line_mul_unrolled : %f\n", line2);
    //print_matrix(crypt,filesz);
 
    clear_crypt(crypt, filesz*filesz);
 
    // with line mul line
    clock_t beg3 = clock();
    line_mul_line(file, key, crypt, filesz, keysz);
    clock_t finish3 = clock();
    double line3;
    line3 = (double)(finish3- beg3)/CLOCKS_PER_SEC;
    printf("Execution time of line_mul_line : %f\n", line3);
    //print_matrix(crypt,filesz);
 
    clear_crypt(crypt,filesz*filesz);
 
    // with line_mul_line_unrolled
    clock_t beg4 = clock();
    line_mul_line_unrolled(file, key, crypt, filesz, keysz);
    clock_t finish4 = clock();
    double line4;
    line4 = (double)(finish4- beg4)/CLOCKS_PER_SEC;
    printf("Execution time of line_mul_line_unrolled : %f\n", line4);
    //print_matrix(crypt,filesz);
 
 
    free(crypt);
 
 
    return 0;
}