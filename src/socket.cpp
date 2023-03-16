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

#include "messages.h"
#include "socket.h"
#include "errors.h"

#define DEFAULT_TIMEOUT 5

int create_socket(bool timeout)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		throw socket_error("error creating socket");

	// Add socket timeout
	if (timeout)
	{
		struct timeval timeout;
		timeout.tv_sec = DEFAULT_TIMEOUT;
		timeout.tv_usec = 0;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			throw socket_error("error setting socket timeout");
	}

	return sockfd;
}

ssize_t send_data(int sockfd, sockaddr_in &address, std::vector<std::uint8_t> data_buffer)
{
	auto bytes_sent = sendto(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&address, sizeof(address));
	if (bytes_sent < 0)
		throw send_error("error sending data");
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

	// Throw error when no retries remaining
	if (bytes_received < 0)
		throw receive_error("error receiving data");

	// Check if error message
	Opcode opcode = decodeOpcode(data_buffer);
	if (opcode == Opcode::ERROR)
	{
		ErrorMessage error = decodeErrorMessage(data_buffer);
		throw error_message("received error message: " + error.message);
	}

	return {bytes_received, data_buffer};
}

AckMessage receive_ack(int sockfd, sockaddr_in &address)
{
	// Receive data
	auto [_, data_buffer] = receive_data(sockfd, address);

	// Decode message
	Opcode opcode = decodeOpcode(data_buffer);
	if (opcode != Opcode::ACK)
		throw unexpected_message("expected ack, received opcode: " + (std::uint16_t)opcode);

	// Parse ack
	AckMessage ack = decodeAckMessage(data_buffer);
	std::cout << "Received ack message with session: " << ack.session << ", block: " << ack.block << ", segment: " << +ack.segment << std::endl;
	return ack;
}
