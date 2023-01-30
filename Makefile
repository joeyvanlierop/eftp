all: client/client server/server

client/client: client/client.c
		gcc -o client/client client/client.c

server/server: server/server.c
		gcc -o server/server server/server.c

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile
