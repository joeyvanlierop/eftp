#pragma once

#include "messages.h"
#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>
#include <tuple>

#define BLOCK_SIZE 8192
#define SEGMENT_COUNT 8
#define SEGMENT_SIZE 1024

int create_socket(bool timeout);

ssize_t send_data(int sockfd, sockaddr_in &address, std::vector<std::uint8_t> data_buffer);

ssize_t send_ack(int sockfd, sockaddr_in &address, std::uint16_t session, std::uint16_t block, std::uint8_t segment);

std::tuple<ssize_t, std::vector<std::uint8_t>> receive_data(int sockfd, sockaddr_in &address);

AckMessage receive_ack(int sockfd, sockaddr_in &address);
