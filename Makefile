client : client.c
	gcc client.c -o client -lpthread -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2

server : server.c
	gcc server.c -o server -lpthread -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2

clean :
	rm server
	rm client

all : client.c server.c
	gcc client.c -o client -lpthread -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2
	gcc server.c -o server-optim -lpthread -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2

rs : 
	./server -j 1 -s 8 -p 2241 -d 1

rc : 
	./client -r 2 -t 2 -k 4 127.0.0.1:2241 

server-optim :
	gcc server.c -o server-optim -lpthread -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2