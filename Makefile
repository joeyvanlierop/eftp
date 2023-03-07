all: client/client server/server

client/client: client/client.cpp
		gcc -o client/client client/client.cpp

server/server: server/server.cpp
		gcc -o server/server server/server.cpp

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile
