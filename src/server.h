#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <tuple>
#include <string>

int main(int argc, char *argv[]);

std::tuple<std::string, std::string> parse_auth(const std::string& input);

class Server {
  private:
    std::string username;
    std::string password;
    std::string working_dir;
    int port;
    int server_socket;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
  
  public:
    Server(const std::string& username, const std::string& password, int port, const std::string& working_dir);
    void run();
};