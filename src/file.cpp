#include "file.h"
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
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

void send_file(int sockfd, sockaddr_in client_address, int session, std::string filename, std::string working_directory)
{
	std::ifstream file(working_directory + filename, std::ios::in | std::ios::binary);

	int current_block = 1;
	while (1)
	{
		// Read block
		std::vector<std::uint8_t> block(BLOCK_SIZE);
		file.read(reinterpret_cast<char *>(block.data()), BLOCK_SIZE);

		// Find out how many characters were actually read.
		auto bytes_read = file.gcount();
		block.resize(bytes_read);

		// Send the block
		send_block(sockfd, client_address, session, block, current_block++, bytes_read);

		// Done reading file once bytes read is less than the block size
		if (bytes_read < BLOCK_SIZE)
			break;
	}

	std::cout << "Done sending " << filename << std::endl;
	close(sockfd);
}

void send_block(int sockfd, sockaddr_in client_address, int session, std::vector<std::uint8_t> block, int current_block, int block_size)
{
	// Split into segments
	int num_segments = std::max(1, (int)std::ceil((double)block_size / (double)SEGMENT_SIZE));
	int offset = 0;
	for (offset = 0; offset < num_segments; offset++)
	{
		auto start_index = block.begin() + offset * SEGMENT_SIZE;
		auto end_index = (offset + 1) * SEGMENT_SIZE > block_size ? block.end() : start_index + SEGMENT_SIZE;
		auto segment = std::vector<std::uint8_t>(start_index, end_index);
		send_segment(sockfd, client_address, session, segment, current_block, offset + 1);
	}
	if (num_segments < SEGMENT_COUNT && block_size % SEGMENT_SIZE == 0)
	{
		send_segment(sockfd, client_address, session, {}, current_block, offset + 1);
	}
}

bool send_segment(int sockfd, sockaddr_in client_address, int session, std::vector<std::uint8_t> segment, int current_block, int current_segment)
{
	DataMessage data;
	data.session = session;
	data.block = current_block;
	data.segment = current_segment;
	data.data = segment;
	auto data_buffer = encodeDataMessage(data);
	auto bytes_sent = sendto(sockfd, data_buffer.data(), data_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
	if (bytes_sent < 0)
	{
		std::cerr << "Failed to send data segment" << std::endl;
		close(sockfd);
		return false;
	}
	std::cout << "Sent data segment with session: " << session << ", block: " << current_block << ", segment: " << current_segment << " (" << segment.size() << ")" << std::endl;

	// Wait for ack message to arrive
	std::vector<std::uint8_t> ack_buffer(1031);
	socklen_t len = sizeof(client_address);
	ssize_t bytes_received = recvfrom(sockfd, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&client_address, &len);
	if (bytes_received < 0)
	{
		std::cerr << "Failed to receive ack message" << std::endl;
		close(sockfd);
		return false;
	}

	// Validate ack message
	Opcode opcode = decodeOpcode(ack_buffer);
	if (opcode != Opcode::ACK)
	{
		std::cout << "Expected ack message" << std::endl;
		close(sockfd);
		return false;
	}
	AckMessage ack = decodeAckMessage(ack_buffer);
	std::cout << "Received ack packet" << std::endl;

	if (ack.session == session && ack.block == current_block && ack.segment == current_segment)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void receive_file(int sockfd, sockaddr_in address, int session, std::string filename, std::string working_directory)
{
	std::ofstream file(working_directory + filename, std::ios::out | std::ios::binary);

	while (1)
	{
		// Write block to file
		std::vector<std::uint8_t> block = receive_block(sockfd, address, session);
		file.write(reinterpret_cast<const char *>(block.data()), block.size());

		// We are done reading when the incoming block is less than the max block size
		if (block.size() < BLOCK_SIZE)
			break;
	}

	std::cout << "Done receiving " << filename << std::endl;
	file.close();
}

std::vector<std::uint8_t> receive_block(int sockfd, sockaddr_in client_address, int session)
{
	// Block buffer
	std::vector<std::uint8_t> block(BLOCK_SIZE, 1);
	int bytes_received;

	for (int i = 0; i < SEGMENT_COUNT; i++)
	{
		// Receive a segment
		auto segment = receive_segment(sockfd, client_address, session);

		// Add segment to block and track total size
		std::memcpy(&block[i * SEGMENT_SIZE], &segment[0], segment.size());
		bytes_received += segment.size();

		// Stop reading block if received segment is less than the max segment size
		if (segment.size() < SEGMENT_SIZE)
		{
			break;
		}
	}
	block.resize(bytes_received);

	std::cout << "Received block: " << bytes_received << std::endl;
	return block;
}

std::vector<std::uint8_t> receive_segment(int sockfd, sockaddr_in client_address, int session)
{
	// Wait for a message to arrive
	std::vector<std::uint8_t> buffer(DATA_HEADER_SIZE + SEGMENT_SIZE, 0);
	socklen_t len = sizeof(client_address);
	ssize_t bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&client_address, &len);
	if (bytes_received < 0)
	{
		std::cerr << "Failed to receive message" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Validate data message
	Opcode opcode = decodeOpcode(buffer);
	if (opcode != Opcode::DATA)
	{
		std::cout << "Expected data message" << std::endl;
		exit(EXIT_FAILURE);
	}
	buffer.resize(bytes_received);
	DataMessage data = decodeDataMessage(buffer);
	std::cout << "Received data packet (" << data.data.size() << ")" << std::endl;

	// Send an ack message
	AckMessage ack;
	ack.session = session;
	ack.block = data.block;
	ack.segment = data.segment;
	auto ack_buffer = encodeAckMessage(ack);
	auto bytes_sent = sendto(sockfd, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&client_address, sizeof(client_address));
	std::cout << "Sent ack message with session: " << ack.session << ", block: " << ack.block << ", segment: " << +ack.segment << std::endl;

	// Return the data buffer
	return data.data;
}