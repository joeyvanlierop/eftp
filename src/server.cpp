#include "server.h"
#include "messages.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <signal.h>
#include <map>

// Server socket descriptor
int sockfd;

// Session state management
// 'sessions' maps a session id to a port
int current_session = 0;
std::map<int, int> sessions;

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
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Error creating server socket" << std::endl;
		exit(EXIT_FAILURE);
	}

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
		std::vector<std::uint8_t> buffer(1031);
		struct sockaddr_in client_address;
		socklen_t len = sizeof(client_address);
		ssize_t bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&client_address, &len);
		if (bytes_received < 0)
		{
			std::cerr << "Failed to receive message" << std::endl;
			exit(EXIT_FAILURE);
		}

		// Process message
		// Only auth messages should be sent to the public port
		std::thread session_thread(session, buffer, client_address, username, password);
		session_thread.detach();
	}
}

void session(std::vector<std::uint8_t> buffer, sockaddr_in client_address, std::string username, std::string password)
{
	// Validate opcode
	Opcode opcode = decodeOpcode(buffer);
	if (opcode != Opcode::AUTH)
	{
		std::cout << "Expected auth message, received: " << (std::uint16_t)opcode << std::endl;
		return;
	}

	// Decode message
	AuthMessage message = decodeAuthMessage(buffer);
	std::cout << "Received auth message with username: " << message.username << ", password: " << message.password << std::endl;

	// Register session
	int session = current_session++;
	sessions[session] = client_address.sin_port;

	// Bind the server socket to the server address
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Error creating server socket" << std::endl;
		return;
	}

	// Validate credentials
	if (username.compare(message.username) != 0 || password.compare(message.password) != 0)
	{
		// Send the error message and exit
		ErrorMessage error;
		error.message = "Invalid credentials";
		auto error_buffer = encodeErrorMessage(error);
		sendto(sockfd, error_buffer.data(), error_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
		return;
	}

	// Send an ack message from a randomly assigned port
	AckMessage ack;
	ack.session = session;
	ack.block = 0;
	ack.segment = 0;
	auto ack_buffer = encodeAckMessage(ack);
	auto bytes_sent = sendto(sockfd, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
	if (bytes_sent < 0)
	{
		std::cerr << "Failed to send ack message" << std::endl;
		return;
	}

	// Session
	while (1)
	{
		// Wait for a message to arrive
		std::vector<std::uint8_t> buffer(1031);
		struct sockaddr_in client_address;
		socklen_t len = sizeof(client_address);
		ssize_t bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&client_address, &len);
		if (bytes_received < 0)
		{
			std::cerr << "Failed to receive message" << std::endl;
			exit(EXIT_FAILURE);
		}

		// Process message
		std::thread process(process_message, buffer, bytes_received);
		process.detach();
	}
}

void process_message(std::vector<std::uint8_t> buffer, ssize_t bytes_received)
{
	Opcode opcode = decodeOpcode(buffer);
	if (opcode == Opcode::AUTH)
	{
		std::cout << "Received unexpected auth message" << std::endl;
	}
	else if (opcode == Opcode::RRQ)
	{
		ReadRequestMessage message = decodeReadRequestMessage(buffer);
		std::cout << "Received read request message with session: " << message.session << ", filename: " << message.filename << std::endl;
	}
	else if (opcode == Opcode::WRQ)
	{
		WriteRequestMessage message = decodeWriteRequestMessage(buffer);
		std::cout << "Received write request message with session: " << message.session << ", filename: " << message.filename << std::endl;
	}
	else if (opcode == Opcode::DATA)
	{
		DataMessage message = decodeDataMessage(buffer);
		std::cout << "Received data request message with session: " << message.session << ", block: " << message.block << ", segment: " << +message.segment << ", data: { ";
		for (auto i = 0; i < bytes_received - DATA_HEADER_SIZE; i++)
		{
			std::cout << +message.data[i] << ", ";
		}
		std::cout << "}" << std::endl;
	}
	else if (opcode == Opcode::ACK)
	{
		AckMessage message = decodeAckMessage(buffer);
		std::cout << "Received ack message with session: " << message.session << ", block: " << message.block << ", segment: " << +message.segment << std::endl;
	}
	else if (opcode == Opcode::ERROR)
	{
		ErrorMessage message = decodeErrorMessage(buffer);
		std::cout << "Received error message with message: " << message.message << std::endl;
	}
	else
	{
		std::cout << "Received message with invalid opcode: " << (std::uint16_t)opcode << std::endl;
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