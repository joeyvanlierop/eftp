.PHONY: clean

all: eftpclient eftpserver

eftpclient: client.o messages.o 
	g++ -g client.o messages.o -o eftpclient

eftpserver: server.o messages.o 
	g++ -g server.o messages.o -o eftpserver

client.o: src/client.cpp src/client.h
	g++ -c src/client.cpp

server.o: src/server.cpp src/server.h
	g++ -c src/server.cpp

messages.o: src/messages.cpp src/messages.h
	g++ -c src/messages.cpp

clean:
	-rm *.o eftpclient eftpserver

zip: client server
	zip -r JosephVanLierop.zip client/ server/ Makefile