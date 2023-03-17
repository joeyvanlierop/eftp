.PHONY: clean

all: eftpclient eftpserver

eftpclient: client.o file.o socket.o messages.o errors.o
	g++ -g client.o file.o socket.o messages.o errors.o -o eftpclient

eftpserver: server.o session.o file.o socket.o messages.o errors.o
	g++ -g server.o session.o file.o socket.o messages.o errors.o -o eftpserver

client.o: bonus/client.cpp bonus/client.h
	g++ -c bonus/client.cpp

server.o: bonus/server.cpp bonus/server.h
	g++ -c bonus/server.cpp

session.o: bonus/session.cpp bonus/session.h
	g++ -c bonus/session.cpp

socket.o: bonus/socket.cpp bonus/socket.h
	g++ -c bonus/socket.cpp

file.o: bonus/file.cpp bonus/file.h
	g++ -c bonus/file.cpp

messages.o: bonus/messages.cpp bonus/messages.h
	g++ -c bonus/messages.cpp

errors.o: bonus/errors.cpp bonus/errors.h
	g++ -c bonus/errors.cpp

clean:
	-rm *.o eftpclient eftpserver

zip: eftpclient eftpserver
	zip -r JosephVanLierop.zip client/ server/ bonus/ test/ eftpserver eftpclient Makefile runclient.sh runserver.sh