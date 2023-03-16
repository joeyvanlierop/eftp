#include "file.h"
#include "session.h"
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

void session(std::vector<std::uint8_t> buffer, sockaddr_in client_address, std::string username, std::string password, int session, std::string working_directory)
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

	// Create and bind the session to a random port
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Error creating session socket" << std::endl;
		return;
	}

	// Validate credentials
	if (username != message.username || password != message.password)
	{
		// Send the error message and exit
		ErrorMessage error;
		error.message = "Invalid credentials";
		auto error_buffer = encodeErrorMessage(error);
		sendto(sockfd, error_buffer.data(), error_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
		close(sockfd);
		return;
	}

	// Send an ack message
	AckMessage ack;
	ack.session = session;
	ack.block = 0;
	ack.segment = 0;
	auto ack_buffer = encodeAckMessage(ack);
	auto bytes_sent = sendto(sockfd, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
	if (bytes_sent < 0)
	{
		std::cerr << "Failed to send ack message" << std::endl;
		close(sockfd);
		return;
	}
		std::cout << "Sent ack message with session: " << ack.session << std::endl;

	// Wait for a read or write request to arrive
	std::vector<std::uint8_t> req_buffer(1031);
	socklen_t len = sizeof(client_address);
	ssize_t bytes_received = recvfrom(sockfd, req_buffer.data(), req_buffer.size(), 0, (struct sockaddr *)&client_address, &len);
	if (bytes_received < 0)
	{
		std::cerr << "Failed to receive message" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Process message
	// Should be a read or write request
	// void receive_file(int sockfd, sockaddr_in client_address, int session, std::string filename);
	opcode = decodeOpcode(req_buffer);
	if (opcode == Opcode::RRQ)
	{
		ReadRequestMessage rrq = decodeReadRequestMessage(req_buffer);
		std::cout << "Received read request message with session: " << rrq.session << ", filename: " << rrq.filename << std::endl;
		std::thread process(send_file, sockfd, client_address, session, rrq.filename, working_directory);
		process.detach();
	}
	else if (opcode == Opcode::WRQ)
	{
		WriteRequestMessage wrq = decodeWriteRequestMessage(req_buffer);
		std::cout << "Received write request message with session: " << wrq.session << ", filename: " << wrq.filename << std::endl;
		std::thread process(receive_file, sockfd, client_address, session, wrq.filename, working_directory);
		process.detach();
	}
	else
	{
		std::cout << "Expected read request message or write request message" << std::endl;
	}
}
