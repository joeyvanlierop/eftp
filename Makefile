all: client/eftpclient server/eftpserver

client/eftpclient: client/client.cpp client/client.h
		g++ -o client/eftpclient client/client.cpp client/client.h

server/eftpserver: server/server.cpp server/server.h
		g++ -o server/eftpserver server/server.cpp server/server.h

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile
