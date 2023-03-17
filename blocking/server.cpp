#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <signal.h>
#include <map>

#include "server.h"
#include "socket.h"
#include "session.h"
#include "errors.h"

// Server socket descriptor
int sockfd;

// Session
int current_session = 0;

int main(int argc, char *argv[])
{
	// Extract username, password, port, and working directory from command line arguments
	if (argc != 4)
	{
		printf("Usage: %s [username:password] [port] [working directory]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	auto [username, password] = parse_auth(argv[1]);
	int port = atoi(argv[2]);
	std::string working_directory = std::string(argv[3]);

	// Create the server socket
	sockfd = create_socket(false);

	// Prepare the server address (protocol, ip address, port)
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	// Bind the server socket to the server address
	if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		std::cerr << "Error binding server socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Server loop
	signal(SIGINT, sig_handler);
	while (1)
	{
		// Wait for a message to arrive
		struct sockaddr_in client_address;
		socklen_t len = sizeof(client_address);
		std::vector<std::uint8_t> buffer;
		try
		{
			std::tie(std::ignore, buffer) = receive_data(sockfd, client_address);
		}
		catch (eftp_exception const &e)
		{
			std::cout << e.what() << std::endl;
			continue;
		}

		// Process message
		// Only auth messages should be sent to the public port
		session(buffer, client_address, username, password, current_session++, working_directory);
	}
}

std::tuple<std::string, std::string> parse_auth(const std::string &input)
{
	// Find the position of the ":" separator
	size_t colon_pos = input.find(':');
	if (colon_pos == std::string::npos)
	{
		throw std::invalid_argument("Invalid auth: no ':' separator found");
	}

	// Extract the username and password from the input string
	std::string username = input.substr(0, colon_pos);
	std::string password = input.substr(colon_pos + 1);

	// Return the username and password as a tuple
	return {username, password};
}

void sig_handler(int signal_num)
{
	if (signal_num == SIGINT)
	{
		close(sockfd);
		exit(signal_num);
	}
}