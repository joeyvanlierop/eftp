#pragma once

#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

void sig_handler(int signal_num);

int main(int argc, char *argv[]);

std::tuple<std::string, std::string> parse_auth(const std::string &input);