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

	// Track unacked segments
	std::map<int, std::vector<std::uint8_t>> unacked;
	int offset = 0;
	for (offset = 0; offset < num_segments; offset++)
	{
		// Construct segment
		auto start_index = block.begin() + offset * SEGMENT_SIZE;
		auto end_index = (offset + 1) * SEGMENT_SIZE > block_size ? block.end() : start_index + SEGMENT_SIZE;
		auto segment = std::vector<std::uint8_t>(start_index, end_index);

		// Track segment
		unacked[offset + 1] = segment;
	}

	// Hacky solution lmao
	// Send empty segment if block divides perfectly
	if (num_segments < SEGMENT_COUNT && block_size % SEGMENT_SIZE == 0 && block_size != 0)
	{
		unacked[offset + 1] = {};
	}

	// Send all of the segments
	for (auto const &entry : unacked)
	{
		std::thread segment_thread(send_segment, sockfd, address, session, entry.second, current_block, entry.first);
		segment_thread.detach();
	}

	// Wait for all acks
	int retry_count = 3;
	while (unacked.size() > 0)
	{
		try
		{
			auto ack = receive_ack(sockfd, address);
			if (ack.block == current_block)
				unacked.erase(ack.segment);
		}
		catch (eftp_exception const &e)
		{
			if (retry_count > 0)
			{
				for (auto const &entry : unacked)
				{
					std::thread segment_thread(send_segment, sockfd, address, session, entry.second, current_block, entry.first);
					segment_thread.detach();
				}
				retry_count--;
			}
			else
			{
				timeout_error("transfer timed out after 3 retries");
			}
		}
	}
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
	send_data(sockfd, address, data_buffer);
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

std::vector<std::uint8_t> receive_block(int sockfd, sockaddr_in address, int session)
{
	// Block buffer
	std::vector<std::uint8_t> block(BLOCK_SIZE, 1);
	int bytes_received;

	// Track unreceived segments
	std::set<int> unreceived;
	for (int i = 0; i < SEGMENT_COUNT; i++)
		unreceived.insert(i);

	// Wait for all acks
	int retry_count = 3;
	while (unreceived.size() > 0)
	{
		try
		{
			// Receive a segment
			auto data = receive_segment(sockfd, address, session);
			auto segment = data.data;
			auto offset = data.segment - 1;

			// Add segment to block and track total size
			std::memcpy(&block[offset * SEGMENT_SIZE], &segment[0], segment.size());
			bytes_received += segment.size();

			// Unmark this segment as unreceived (wow nice double negative)
			unreceived.erase(offset);

			// Unmark all unreceived segments if this is the final segment
			if (segment.size() < SEGMENT_SIZE)
				for (int i = offset + 1; i <= SEGMENT_COUNT; i++)
					unreceived.erase(i);
		}
		catch (eftp_exception const &e)
		{
			if (retry_count > 0)
				retry_count--;
			else
				timeout_error("receive timed out after 3 retries");
		}
	}
	block.resize(bytes_received);

	std::cout << "Received block: " << bytes_received << std::endl;
	return block;
}

DataMessage receive_segment(int sockfd, sockaddr_in address, int session)
{
	// Wait for a message to arrive
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
	std::cout << "Received data packet (" << data.data.size() << " bytes)" << std::endl;

	// Send an ack message
	send_ack(sockfd, address, session, data.block, data.segment);

	// Return the data buffer
	return data;
}