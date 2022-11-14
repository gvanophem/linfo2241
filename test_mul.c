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


ARRAY_TYPE* mat_mul(ARRAY_TYPE* file, ARRAY_TYPE* key, int nbytes, int keysz){
    int nr = nbytes/keysz;
    ARRAY_TYPE* crypt = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
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
    }return crypt;
}

ARRAY_TYPE* line_mul(ARRAY_TYPE* file, ARRAY_TYPE* key, int nbytes, int keysz){
    int nr = nbytes/keysz;
    ARRAY_TYPE* crypt = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
    // for(int i = 0; i < keysz; i++){
    //     for(int k = 0; k < keysz; k++){
    //         ARRAY_TYPE r = key[i*keysz + k];
    //         for(int j = 0; j < keysz; j++){
    //             crypt[i * keysz + j] += r * file[k*keysz + j];
    //         }
    //     }
    // }return crypt;
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
    }return crypt;
}

int main(int argc, char const *argv[])
{
    int keysz = 128;
    int filesz = 4*1024;
    ARRAY_TYPE* key = malloc(sizeof(ARRAY_TYPE) * keysz*keysz);
    // for(int i = 0; i < keysz*keysz; i++){
    //     key[i] = i % 13;
    //     printf("%d ", key[i]);
    // }printf("\n");
    ARRAY_TYPE* file = malloc(sizeof(ARRAY_TYPE) * filesz * filesz);
    for(int j = 0; j < filesz * filesz; j++){
        file[j] = j * 2;
    }clock_t begin, end;
    double diff;
    begin = clock();
    ARRAY_TYPE* crypted = mat_mul(file, key, filesz, keysz);
    end = clock();
    diff = (double) (end-begin)/CLOCKS_PER_SEC;
    printf("Execution time of mat_mul : %f\n", diff);
    // for(int i = 0; i < filesz*filesz; i++){
    //     printf("%d ", crypted[i]);
    // }printf("\n");
    time_t beg = clock();
    ARRAY_TYPE* allez = line_mul(file, key, filesz, keysz);
    time_t finish = clock();
    double line;
    line = (double)(finish- beg)/CLOCKS_PER_SEC;
    printf("Execution time of line_mul : %f\n", line);
    // for(int i = 0; i < filesz*filesz; i++){
    //     printf("%d ", allez[i]);
    // }printf("\n");
    return 0;
}
