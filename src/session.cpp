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

void session(std::vector<std::uint8_t> buffer, sockaddr_in client_address, std::string username, std::string password, int session)
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
	opcode = decodeOpcode(req_buffer);
	if (opcode == Opcode::RRQ)
	{
		std::thread process(read_request, req_buffer, bytes_received, client_address, session, sockfd);
		process.detach();
	}
	else if (opcode == Opcode::WRQ)
	{
		std::thread process(write_request, req_buffer, bytes_received, client_address, session, sockfd);
		process.detach();
	}
	else
	{
		std::cout << "Expected read request message or write request message" << std::endl;
	}
}

void read_request(std::vector<std::uint8_t> buffer, ssize_t bytes_received, sockaddr_in client_address, int session, int sockfd)
{
	// Decode rrq message
	ReadRequestMessage rrq = decodeReadRequestMessage(buffer);

	int i = 0;
	while (1)
	{
		// Send a data message
		DataMessage data;
		data.session = session;
		data.block = data.block;
		data.segment = i++;
		if (i == 10)
			data.data = {};
		else
			data.data = {1, 2, 3};
		auto data_buffer = encodeDataMessage(data);
		auto bytes_sent = sendto(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
		if (bytes_sent < 0)
		{
			std::cerr << "Failed to send data message" << std::endl;
			close(sockfd);
			return;
		}
		std::cout << "Sent data packet" << std::endl;

		// Wait for ack message to arrive
		std::vector<std::uint8_t> ack_buffer(1031);
		socklen_t len = sizeof(client_address);
		ssize_t bytes_received = recvfrom(sockfd, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&client_address, &len);
		if (bytes_received < 0)
		{
			std::cerr << "Failed to receive ack message" << std::endl;
			close(sockfd);
			return;
		}

		// Validate data message
		Opcode opcode = decodeOpcode(ack_buffer);
		if (opcode != Opcode::ACK)
		{
			std::cout << "Expected ack message" << std::endl;
			close(sockfd);
			return;
		}
		AckMessage ack = decodeAckMessage(ack_buffer);
		std::cout << "Received ack packet" << std::endl;

		// TODO
		if (i == 10)
			break;
	}

	std::cout << "Done sending " << rrq.filename << std::endl;
	close(sockfd);
}

void write_request(std::vector<std::uint8_t> buffer, ssize_t bytes_received, sockaddr_in client_address, int session, int sockfd)
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
