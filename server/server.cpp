#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#define MAX_LEN 1024

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	int port = atoi(argv[1]);
	int server_socket, client_socket, client_len, bytes_received, read_size;
	struct sockaddr_in server_address, client_address;
	char buffer[MAX_LEN];

	// Create server socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket < 0)
	{
		printf("Error creating server socket\n");
		exit(EXIT_FAILURE);
	}

	// Bind server socket to the given port
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	// Bind server socket with the server address
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		printf("Error binding server socket\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// Receive hello
		socklen_t len = sizeof(client_address); ;
		int n = recvfrom(server_socket, (char *)buffer, MAX_LEN, MSG_WAITALL, ( struct sockaddr *) &client_address, &len);
    buffer[n] = '\0';
		std::cout << "Client: " << buffer << std::endl;

		// Send hello
		auto message = "hello from the server";
    sendto(server_socket, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &client_address, len);
    std::cout << "Hello response sent" << std::endl; 
	}
}
