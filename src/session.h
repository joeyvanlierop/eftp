#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

void session(std::vector<std::uint8_t> buffer, sockaddr_in port, std::string username, std::string password, int session);

void read_request(std::vector<uint8_t> buffer, ssize_t bytes_received, sockaddr_in client_address, int session, int sockfd);

void write_request(std::vector<uint8_t> buffer, ssize_t bytes_received, sockaddr_in client_address, int session, int sockfd);