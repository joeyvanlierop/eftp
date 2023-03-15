#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

int main(int argc, char *argv[]);

void process_auth(std::vector<std::uint8_t> buffer, sockaddr_in port, std::string username, std::string password);

void process_message(std::vector<uint8_t> buffer, ssize_t bytes_received);

std::tuple<std::string, std::string> parse_auth(const std::string &input);