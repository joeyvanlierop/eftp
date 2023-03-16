#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#include "client.h"
#include "messages.h"
#include "file.h"
#include "socket.h"
#include "errors.h"

int main(int argc, char *argv[])
{
	// Extract username, password, ip, port, direction, and filename from command line arguments
	if (argc != 4)
	{
		printf("Usage: %s [username:password@ip:port] [\"upload\"|\"download\"] [filename]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	auto [username, password, ip, port] = parse_auth(argv[1]);
	bool upload = std::string(argv[2]) == "upload";
	std::string filename = argv[3];

	// Create the client socket
	int sockfd = create_socket(true);

	// Prepare the server address (protocol, ip address, port)
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip.c_str());
	server_address.sin_port = htons(port);

	// Prepare auth message
	AuthMessage message;
	message.username = username;
	message.password = password;
	auto auth_buffer = encodeAuthMessage(message);

	// Get session number from ack message
	AckMessage ack;
	try
	{
		std::cout << "Sending auth message with username: " << message.username << ", password: " << message.password << std::endl;
		ack = exchange_data(sockfd, server_address, auth_buffer);
	}
	catch (eftp_exception const &e)
	{
		std::cerr << e.what() << std::endl;
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	int session = ack.session;

	// File transfer
	if (upload)
		write_request(sockfd, server_address, session, filename);
	else
		read_request(sockfd, server_address, session, filename);

	// Clean up
	close(sockfd);
	return EXIT_SUCCESS;
}

void read_request(int sockfd, sockaddr_in server_address, int session, std::string filename)
{
	// Send read request
	ReadRequestMessage rrq;
	rrq.session = session;
	rrq.filename = filename;
	auto rrq_buffer = encodeReadRequestMessage(rrq);

	// Send rrq message and immediately start receiving file
	std::cout << "Sending rrq message with session: " << session << ", filename: " << filename << std::endl;
	send_data(sockfd, server_address, rrq_buffer);

	// Read incoming blocks
	try
	{
		receive_file(sockfd, server_address, session, filename, "./");
	}
	catch (eftp_exception const &e)
	{
		std::cerr << e.what() << std::endl;
	}

	// Close socket
	close(sockfd);
}

void write_request(int sockfd, sockaddr_in server_address, int session, std::string filename)
{
	// Send read request
	WriteRequestMessage wrq;
	wrq.session = session;
	wrq.filename = filename;
	auto wrq_buffer = encodeWriteRequestMessage(wrq);

	// Send wrq message and wait for ack
	std::cout << "Sending wrq message with session: " << session << ", filename: " << filename << std::endl;
	exchange_data(sockfd, server_address, wrq_buffer);

	// Read incoming blocks
	try
	{
		send_file(sockfd, server_address, session, filename, "./");
	}
	catch (eftp_exception const &e)
	{
		std::cerr << e.what() << std::endl;
	}

	// Close socket
	close(sockfd);
}

std::tuple<std::string, std::string, std::string, int> parse_auth(const std::string &input)
{
	// Find the position of the "@" separator
	size_t at_pos = input.find('@');
	if (at_pos == std::string::npos)
	{
		throw std::invalid_argument("Invalid input: no '@' separator found");
	}

	// Find the position of the ":" separator
	size_t colon_pos = input.find(':');
	if (colon_pos == std::string::npos || colon_pos > at_pos)
	{
		throw std::invalid_argument("Invalid auth: no ':' separator found in 'username:password'");
	}

	// Extract the username and password from the input string
	std::string username = input.substr(0, colon_pos);
	std::string password = input.substr(colon_pos + 1, at_pos - colon_pos - 1);

	// Find the position of the ":" separator in the "ip:port" part of the input
	size_t colon2_pos = input.find(':', at_pos + 1);
	if (colon2_pos == std::string::npos)
	{
		throw std::invalid_argument("Invalid input: no ':' separator found in 'ip:port'");
	}

	// Extract the ip and port from the input
	std::string ip = input.substr(at_pos + 1, colon2_pos - at_pos - 1);
	int port = std::stoi(input.substr(colon2_pos + 1));

	// Return the username and password as a tuple
	return {username, password, ip, port};
}