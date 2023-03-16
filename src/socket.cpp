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
	std::cout << "Sending ack message with session: " << session << ", block: " << block << ", segment: " << +segment << std::endl;
	auto bytes_sent = send_data(sockfd, address, ack_buffer);
	return bytes_sent;
}

AckMessage exchange_data(int sockfd, sockaddr_in &address, std::vector<std::uint8_t> data_buffer)
{
	return exchange_data(sockfd, address, data_buffer, DEFAULT_RETRIES);
}

AckMessage exchange_data(int sockfd, sockaddr_in &address, std::vector<std::uint8_t> data_buffer, int retry_count)
{
	// Send data message
	send_data(sockfd, address, data_buffer);

	// Wait for ack to arrive
	AckMessage ack;
	try
	{
		// Verify correct ack
		ack = receive_ack(sockfd, address);
		return ack;
	}
	catch (receive_error const &e)
	{
		// Retry if receive timed out
		if (retry_count > 0)
		{
			return exchange_data(sockfd, address, data_buffer, retry_count - 1);
		}
		else
			throw timeout_error("transfer timed out after 3 retries");
	}
}

std::tuple<ssize_t, std::vector<std::uint8_t>> receive_data(int sockfd, sockaddr_in &address)
{
	// Wait for ack message to arrive
	std::vector<std::uint8_t> data_buffer(1031);
	socklen_t len = sizeof(address);
	ssize_t bytes_received = recvfrom(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&address, &len);

	// Throw error when no retries remaining
	if (bytes_received < 0)
		throw receive_error("error receiving message");

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
