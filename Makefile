all: client/client server/server

client/client: client/client.cpp
		g++ -o client/client client/client.cpp

server/server: server/server.cpp
		g++ -o server/server server/server.cpp

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile
