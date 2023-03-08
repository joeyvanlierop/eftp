#include "client.h"
#include "packets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main(int argc, char *argv[])
{
	// Command line stuff
	if (argc != 4)
	{
		printf("Usage: %s [username:password@ip:port] [\"upload\"|\"download\"] [filename]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	auto [username, password, ip, port] = parse_auth(argv[1]);
	bool upload = std::string(argv[2]) == "upload";
	std::string filename = argv[3];

	// Socket stuff
	int client_socket, bytes_received;
	struct sockaddr_in server_address;
	char buffer[1024];

	// Create client socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket < 0)
	{
		printf("Error creating client socket\n");
		exit(EXIT_FAILURE);
	}

	// Prepare server struct
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip.c_str());
	server_address.sin_port = htons(port);

	// Encode an auth packet as a vector of bytes
	AuthPacket packet;
	packet.username = username;
	packet.password = password;
	auto packet_data = encodeAuthPacket(packet);

	// Send the packet over UDP
	auto bytes_sent = sendto(client_socket, packet_data.data(), packet_data.size(), 0, (struct sockaddr*)&server_address, sizeof(server_address));
	if (bytes_sent < 0) {
			std::cerr << "Failed to send packet\n";
			exit(EXIT_FAILURE);
	}

	// Clean up
	close(client_socket);
	return EXIT_SUCCESS;
}

std::tuple<std::string, std::string, std::string, int> parse_auth(const std::string& input) {
	// Find the position of the "@" separator
	size_t at_pos = input.find('@');
	if (at_pos == std::string::npos) {
		throw std::invalid_argument("Invalid input: no '@' separator found");
	}

	// Find the position of the ":" separator
	size_t colon_pos = input.find(':');
	if (colon_pos == std::string::npos || colon_pos > at_pos) {
		throw std::invalid_argument("Invalid auth: no ':' separator found in 'username:password'");
	}

	// Extract the username and password from the input string
	std::string username = input.substr(0, colon_pos);
	std::string password = input.substr(colon_pos + 1, at_pos - colon_pos - 1);

	// Find the position of the ":" separator in the "ip:port" part of the input
	size_t colon2_pos = input.find(':', at_pos + 1);
	if (colon2_pos == std::string::npos) {
		throw std::invalid_argument("Invalid input: no ':' separator found in 'ip:port'");
	}

	// Extract the ip and port from the input
	std::string ip = input.substr(at_pos + 1, colon2_pos - at_pos - 1);
	int port = std::stoi(input.substr(colon2_pos + 1));

	// Return the username and password as a tuple
	return {username, password, ip, port};
}