#include <tuple>
#include <string>
#include <arpa/inet.h>
#include <vector>

void session(std::vector<std::uint8_t> buffer, sockaddr_in port, std::string username, std::string password, int session, std::string working_directory);
