#include <stdio.h>
#include <stdlib.h>
#define ARRAY_TYPE uint32_t

int** mat_mul(int* file, int* key, int nbytes, int keysz){
    int nr = nbytes/keysz;
    ARRAY_TYPE* crypted = malloc(sizeof(ARRAY_TYPE)*nbytes*nbytes);
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
}


int main(int argc, char const *argv[])
{
    /* code */
    return 0;
}
