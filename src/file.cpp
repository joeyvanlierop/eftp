#include "file.h"
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
#include <signal.h>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

void send_file(int sockfd, sockaddr_in address, int session, std::string filename, std::string working_directory)
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
		send_block(sockfd, address, session, block, current_block++, bytes_read);

		// Done reading file once bytes read is less than the block size
		if (bytes_read < BLOCK_SIZE)
			break;
	}

	std::cout << "Done sending " << filename << std::endl;
	close(sockfd);
}

void send_block(int sockfd, sockaddr_in address, int session, std::vector<std::uint8_t> block, int current_block, int block_size)
{
	// Split into segments
	int num_segments = std::max(1, (int)std::ceil((double)block_size / (double)SEGMENT_SIZE));
	int offset = 0;
	for (offset = 0; offset < num_segments; offset++)
	{
		auto start_index = block.begin() + offset * SEGMENT_SIZE;
		auto end_index = (offset + 1) * SEGMENT_SIZE > block_size ? block.end() : start_index + SEGMENT_SIZE;
		auto segment = std::vector<std::uint8_t>(start_index, end_index);
		send_segment(sockfd, address, session, segment, current_block, offset + 1);
	}
	if (num_segments < SEGMENT_COUNT && block_size % SEGMENT_SIZE == 0)
	{
		send_segment(sockfd, address, session, {}, current_block, offset + 1);
	}
}

bool send_segment(int sockfd, sockaddr_in address, int session, std::vector<std::uint8_t> segment, int current_block, int current_segment)
{
	// Send data message
	DataMessage data;
	data.session = session;
	data.block = current_block;
	data.segment = current_segment;
	data.data = segment;
	auto data_buffer = encodeDataMessage(data);
	send_data(sockfd, address, data_buffer);
	std::cout << "Sent data segment with session: " << session << ", block: " << current_block << ", segment: " << current_segment << " (" << segment.size() << ")" << std::endl;

	// Wait for ack to arrive
	AckMessage ack;
	try {
		ack = receive_ack(sockfd, address);
	} catch(std::exception const& e) {
    std::cout << "Error: " << e.what() << std::endl;
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// Validate received ack
	return (ack.session == session && ack.block == current_block && ack.segment == current_segment);
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

std::vector<std::uint8_t> receive_block(int sockfd, sockaddr_in address, int session)
{
	// Block buffer
	std::vector<std::uint8_t> block(BLOCK_SIZE, 1);
	int bytes_received;

	for (int i = 0; i < SEGMENT_COUNT; i++)
	{
		// Receive a segment
		auto segment = receive_segment(sockfd, address, session);

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

std::vector<std::uint8_t> receive_segment(int sockfd, sockaddr_in address, int session)
{
	// Wait for a message to arrive
	ssize_t bytes_received;
	std::vector<std::uint8_t> buffer;
	try {
		std::tie(bytes_received, buffer) = receive_data(sockfd, address);
	} catch(std::exception const& e) {
    std::cout << "Error: " << e.what() << std::endl;
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
	send_ack(sockfd, address, session, data.block, data.segment);

	// Return the data buffer
	return data.data;
}