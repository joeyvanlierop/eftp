#include <tuple>
#include <string>
#include <arpa/inet.h>

int main(int argc, char *argv[]);

void read_request(int sockfd, sockaddr_in server_address, int session, std::string filename);

std::tuple<std::string, std::string, std::string, int> parse_auth(const std::string &input);