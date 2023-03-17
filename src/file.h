#pragma once

#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

#include "messages.h"

#define BLOCK_SIZE 8192
#define SEGMENT_COUNT 8
#define SEGMENT_SIZE 1024

void send_file(int sockfd, sockaddr_in client_address, int session, std::string filename, std::string working_directory);

void send_block(int sockfd, sockaddr_in client_address, int session, std::vector<std::uint8_t> block, int current_block, int block_size);

void send_segment(int sockfd, sockaddr_in client_address, int session, std::vector<std::uint8_t> block, int current_block, int current_segment);

void receive_file(int sockfd, sockaddr_in client_address, int session, std::string filename, std::string working_directory);

std::vector<std::uint8_t> receive_block(int sockfd, sockaddr_in client_address, int session);

DataMessage receive_segment(int sockfd, sockaddr_in client_address, int session);
