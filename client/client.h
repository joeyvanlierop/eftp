#include <tuple>
#include <string>

int main(int argc, char *argv[]);

std::tuple<std::string, std::string, std::string, int> parse_auth(const std::string& input);