#include "messages.h"
#include "socket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <iostream>

ssize_t send_data(int sockfd, sockaddr_in &address, std::vector<std::uint8_t> data_buffer)
{
	auto bytes_sent = sendto(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&address, sizeof(address));
	if (bytes_sent < 0)
	{
		throw std::runtime_error("Error sending data");
	}
	return bytes_sent;
}

ssize_t send_ack(int sockfd, sockaddr_in &address, std::uint16_t session, std::uint16_t block, std::uint8_t segment)
{
	// Create ack message
	AckMessage ack;
	ack.session = session;
	ack.block = block;
	ack.segment = segment;
	auto ack_buffer = encodeAckMessage(ack);

	// Send ack message
	auto bytes_received = send_data(sockfd, address, ack_buffer);
	std::cout << "Sent ack message with session: " << session << ", block: " << block << ", segment: " << +segment << std::endl;
	return bytes_received;
}

std::tuple<ssize_t, std::vector<std::uint8_t>> receive_data(int sockfd, sockaddr_in &address)
{
	// Wait for ack message to arrive
	std::vector<std::uint8_t> data_buffer(1031);
	socklen_t len = sizeof(address);
	ssize_t bytes_received = recvfrom(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&address, &len);
	if (bytes_received < 0)
	{
		throw std::runtime_error("Error receiving data");
	}
	return {bytes_received, data_buffer};
}

AckMessage receive_ack(int sockfd, sockaddr_in &address)
{
	// Receive data
	auto [_, data_buffer] = receive_data(sockfd, address);
	
	// Decode message
	Opcode opcode = decodeOpcode(data_buffer);
	if (opcode == Opcode::ERROR)
	{
		ErrorMessage error = decodeErrorMessage(data_buffer);
		std::cout << "Received error message: " << error.message << std::endl;
		throw std::runtime_error("Received error message");
	}
	else if (opcode != Opcode::ACK)
	{
		std::cout << "Received message with invalid opcode: " << (std::uint16_t)opcode << std::endl;
		throw std::runtime_error("Expected ack message");
	}

	// Parse ack
	AckMessage ack = decodeAckMessage(data_buffer);
	std::cout << "Received ack message with session: " << ack.session << ", block: " << ack.block << ", segment: " << +ack.segment << std::endl;
	return ack;
}
