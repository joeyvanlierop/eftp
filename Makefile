all: client server

client: client/client.c
		gcc -o client/client client/client.c

server: server/server.c
		gcc -o server/server server/server.c