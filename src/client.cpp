#include "client.h"
#include "messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

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
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		printf("Error creating client socket\n");
		exit(EXIT_FAILURE);
	}

	// Prepare the server address (protocol, ip address, port)
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip.c_str());
	server_address.sin_port = htons(port);

	// Encode an auth message as a vector of bytes
	AuthMessage message;
	message.username = username;
	message.password = password;
	auto message_data = encodeAuthMessage(message);

	// DEBUG 1
	ReadRequestMessage message1;
	message1.session = 1;
	message1.filename = "a";
	auto message_data1 = encodeReadRequestMessage(message1);

	// DEBUG 2
	WriteRequestMessage message2;
	message2.session = 2;
	message2.filename = "b";
	auto message_data2 = encodeWriteRequestMessage(message2);

	// DEBUG 3
	DataMessage message3;
	message3.session = 1;
	message3.block = 2;
	message3.segment = 3;
	message3.data = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	auto message_data3 = encodeDataMessage(message3);

	// DEBUG 4
	AckMessage message4;
	message4.session = 7;
	message4.block = 8;
	message4.segment = 9;
	auto message_data4 = encodeAckMessage(message4);

	// DEBUG 5
	ErrorMessage message5;
	message5.message = "c";
	auto message_data5 = encodeErrorMessage(message5);

	// Send the message over UDP from a randomly assigned port
	auto bytes_sent = sendto(sockfd, message_data.data(), message_data.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	std::cout << "Bytes sent: " << bytes_sent << std::endl;
	if (bytes_sent < 0)
	{
		std::cerr << "Failed to send message\n";
		exit(EXIT_FAILURE);
	}

	// Wait for a packet to arrive
	std::vector<std::uint8_t> buffer(1031); // Allocate a buffer to hold the incoming packet
	socklen_t len = sizeof(server_address);
	ssize_t bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&server_address, &len);
	if (bytes_received < 0)
	{
		std::cerr << "Failed to receive packet" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Decode message
	Opcode opcode = decodeOpcode(buffer);
	if (opcode == Opcode::ERROR)
	{
		ErrorMessage error = decodeErrorMessage(buffer);
		std::cout << "Received error message: " << error.message << std::endl;
	}
	else if (opcode == Opcode::ACK)
	{
		AckMessage ack = decodeAckMessage(buffer);
		std::cout << "Received ack message with session: " << ack.session << ", block: " << ack.block << ", segment: " << +ack.segment << std::endl;
	}
	else
	{
		std::cout << "Received message with invalid opcode: " << (std::uint16_t)opcode << std::endl;
	}

	// // Debug
	// bytes_sent = sendto(sockfd, message_data1.data(), message_data1.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	// std::cout << "Bytes sent: " << bytes_sent << std::endl;

	// bytes_sent = sendto(sockfd, message_data2.data(), message_data2.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	// std::cout << "Bytes sent: " << bytes_sent << std::endl;

	// bytes_sent = sendto(sockfd, message_data3.data(), message_data3.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	// std::cout << "Bytes sent: " << bytes_sent << std::endl;

	// bytes_sent = sendto(sockfd, message_data4.data(), message_data4.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	// std::cout << "Bytes sent: " << bytes_sent << std::endl;

	// bytes_sent = sendto(sockfd, message_data5.data(), message_data5.size(), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	// std::cout << "Bytes sent: " << bytes_sent << std::endl;

	// Clean up
	close(sockfd);
	return EXIT_SUCCESS;
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