.PHONY: clean

all: eftpclient eftpserver

eftpclient: client.o file.o socket.o messages.o errors.o
	g++ -g client.o file.o socket.o messages.o errors.o -o eftpclient

eftpserver: server.o session.o file.o socket.o messages.o errors.o
	g++ -g server.o session.o file.o socket.o messages.o errors.o -o eftpserver

client.o: blocking/client.cpp blocking/client.h
	g++ -c blocking/client.cpp

server.o: blocking/server.cpp blocking/server.h
	g++ -c blocking/server.cpp

session.o: blocking/session.cpp blocking/session.h
	g++ -c blocking/session.cpp

socket.o: blocking/socket.cpp blocking/socket.h
	g++ -c blocking/socket.cpp

file.o: blocking/file.cpp blocking/file.h
	g++ -c blocking/file.cpp

messages.o: blocking/messages.cpp blocking/messages.h
	g++ -c blocking/messages.cpp

errors.o: blocking/errors.cpp blocking/errors.h
	g++ -c blocking/errors.cpp

clean:
	-rm *.o eftpclient eftpserver

zip: eftpclient eftpserver
	zip -r JosephVanLierop.zip client/ server/ blocking/ test/ eftpserver eftpclient Makefile runclient.sh runserver.sh