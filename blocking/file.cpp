#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

#include "file.h"
#include "socket.h"
#include "errors.h"

void send_file(int sockfd, sockaddr_in address, int session, std::string filename, std::string working_directory)
{
	std::ifstream file(working_directory + filename, std::ios::in | std::ios::binary);

	// Send error message if file does not exist
	if (file.fail())
	{
		// Send the error message and exit
		ErrorMessage error;
		error.message = "File does not exist";
		auto error_buffer = encodeErrorMessage(error);
		send_data(sockfd, address, error_buffer);
		throw io_error("file does not exist");
	}

	// Send file blocks
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
	file.close();
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

	// Send empty segment if block divides perfectly
	if (num_segments < SEGMENT_COUNT && block_size % SEGMENT_SIZE == 0 && block_size != 0)
		send_segment(sockfd, address, session, {}, current_block, offset + 1);
}

void send_segment(int sockfd, sockaddr_in address, int session, std::vector<std::uint8_t> segment, int current_block, int current_segment)
{
	// Send data message
	DataMessage data;
	data.session = session;
	data.block = current_block;
	data.segment = current_segment;
	data.data = segment;
	auto data_buffer = encodeDataMessage(data);

	// Exchange data
	std::cout << "Sending data segment with session: " << session << ", block: " << current_block << ", segment: " << current_segment << " (" << segment.size() << ")" << std::endl;
	AckMessage ack = exchange_data(sockfd, address, data_buffer);

	// Validate ack
	if (!(ack.session == session && ack.block == current_block && ack.segment == current_segment))
	{
		throw misordered_ack("received misordered ack");
	}
}

void receive_file(int sockfd, sockaddr_in address, int session, std::string filename, std::string working_directory)
{
	// Send error message if file already exist
	std::ifstream infile(working_directory + filename, std::ios::out | std::ios::binary);
	if (infile.good())
	{
		// Send the error message and exit
		ErrorMessage error;
		error.message = "File already exist";
		auto error_buffer = encodeErrorMessage(error);
		send_data(sockfd, address, error_buffer);
		throw io_error("file already exist");
	}

	std::ofstream file(working_directory + filename, std::ios::out | std::ios::binary);

	while (1)
	{
		// Construct block
		std::vector<std::uint8_t> block = receive_block(sockfd, address, session);

		// Write block to file
		file.write(reinterpret_cast<const char *>(block.data()), block.size());

		// We are done reading when the incoming block is less than the max block size
		if (block.size() < BLOCK_SIZE)
			break;
	}

	std::cout << "Done receiving " << filename << std::endl;
	file.close();
}

bool has_skipped = true;
std::vector<std::uint8_t> receive_block(int sockfd, sockaddr_in address, int session)
{
	// Block buffer
	std::vector<std::uint8_t> block(BLOCK_SIZE, 1);
	int bytes_received;

	for (int i = 0; i < SEGMENT_COUNT; i++)
	{
		// Receive a segment
		DataMessage message;

		// Retry functionality
		while(1) {
			try {
				message = receive_segment(sockfd, address, session);
				break;
			} catch (eftp_exception const &e) {}
		}
		auto segment = message.data;

		// Send an ack message
		// Skip functionality used for demonstration
		if(message.segment == 4 && !has_skipped) {
			has_skipped = true;
			continue;
		} else {
			send_ack(sockfd, address, session, message.block, message.segment);
		}

		// Add segment to block and track total size
		std::memcpy(&block[(message.segment - 1) * SEGMENT_SIZE], &segment[0], segment.size());
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

DataMessage receive_segment(int sockfd, sockaddr_in address, int session)
{
	// Wait for a message to arriv
	auto [bytes_received, buffer] = receive_data(sockfd, address);

	// Validate data message
	Opcode opcode = decodeOpcode(buffer);
	if (opcode != Opcode::DATA)
	{
		std::cout << "Expected data message, received: " << (int)opcode << std::endl;
		exit(EXIT_FAILURE);
	}
	buffer.resize(bytes_received);
	DataMessage data = decodeDataMessage(buffer);
	std::cout << "Received data packet (" << data.data.size() << ")" << std::endl;

	// Return the data buffer
	return data;
}