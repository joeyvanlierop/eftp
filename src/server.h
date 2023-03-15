#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

void sig_handler(int signal_num);

int main(int argc, char *argv[]);

void session(std::vector<std::uint8_t> buffer, sockaddr_in port, std::string username, std::string password);

void process_message(std::vector<uint8_t> buffer, ssize_t bytes_received);

std::tuple<std::string, std::string> parse_auth(const std::string &input);