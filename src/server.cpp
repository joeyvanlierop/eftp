#include "server.h"
#include "packets.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#define MAX_LEN 1024

int main(int argc, char *argv[])
{
	// Command line stuff
	if (argc != 4)
	{
		printf("Usage: %s [username:password] [port] [working directory]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	auto [username, password] = parse_auth(argv[1]);
	int port = atoi(argv[2]);
	std::string working_directory = std::string(argv[3]);

	// Listen for auth packets
	auto server = new Server(username, password, port, working_directory);
	server->run();
}

Server::Server(const std::string& username, const std::string& password, int port, const std::string& working_dir)
	: username(username), password(password), port(port), working_dir(working_dir) {}

void Server::run() {
	// Create server socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket < 0)
	{
		std::cerr << "Error creating server socket\n";
		exit(EXIT_FAILURE);
	}

	// Prepare server struct
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	// Bind server socket with the server address
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		std::cerr << "Error binding server socket\n";
		exit(EXIT_FAILURE);
	}
	
	while (1)
	{
		// Wait for a packet to arrive
		std::vector<std::uint8_t> buffer(MAX_LEN); // Allocate a buffer to hold the incoming packet
		socklen_t len = sizeof(client_address);
		ssize_t bytes_received = recvfrom(server_socket, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client_address, &len);
		if (bytes_received < 0) {
				std::cerr << "Failed to receive packet\n";
				exit(EXIT_FAILURE);
		}

		// Decode the packet
		AuthPacket packet = decodeAuthPacket(buffer);

		// Print the contents of the packet
		std::cout << "Received auth packet with username: " << packet.username << ", password: " << packet.password << "\n";
	}
}

std::tuple<std::string, std::string> parse_auth(const std::string& input) {
	// Find the position of the ":" separator
	size_t colon_pos = input.find(':');
	if (colon_pos == std::string::npos) {
		throw std::invalid_argument("Invalid auth: no ':' separator found");
	}

	// Extract the username and password from the input string
	std::string username = input.substr(0, colon_pos);
	std::string password = input.substr(colon_pos + 1);

	// Return the username and password as a tuple
	return {username, password};
}