.PHONY: clean

all: eftpclient eftpserver

eftpclient: client.o packets.o 
	g++ -g client.o packets.o -o eftpclient

eftpserver: server.o packets.o 
	g++ -g server.o packets.o -o eftpserver

client.o: src/client.cpp src/client.h
	g++ -c src/client.cpp

server.o: src/server.cpp src/server.h
	g++ -c src/server.cpp

packets.o: src/packets.cpp src/packets.h
	g++ -c src/packets.cpp

clean:
	-rm *.o golf

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile