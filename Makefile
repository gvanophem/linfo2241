

client : client.c
	gcc client.c -o client -lpthread

server : server.c
	gcc server.c -o server -lpthread

clean :
	rm server
	rm client

all : client.c server.c
	gcc client.c -o client -lpthread
	gcc server.c -o server -lpthread