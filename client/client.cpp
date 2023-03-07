#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#define SERVER_IP "127.0.0.1"
#define MAX_LEN 1024

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	int port = atoi(argv[1]);
	int client_socket, bytes_sent, bytes_received;
	struct sockaddr_in server_address;
	char buffer[1024];

	// Create a TCP connection with the server
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket < 0)
	{
		printf("Error creating client socket\n");
		exit(EXIT_FAILURE);
	}

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_address.sin_port = htons(port);


	// Send hello
	int n;
	socklen_t len;
	auto message = "hello from the client";
	sendto(client_socket, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &server_address, sizeof(server_address));
	std::cout << "Hello message sent" << std::endl;

	// Receive hello	
	n = recvfrom(client_socket, (char *)buffer, MAX_LEN, MSG_WAITALL, (struct sockaddr *) &server_address, &len);
	buffer[n] = '\0';
	std::cout << "Server: " << buffer << std::endl;
	close(client_socket);
	
	return EXIT_SUCCESS;
}
