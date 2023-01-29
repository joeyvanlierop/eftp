#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

int main() {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(2000);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    int mysocket1;
    mysocket1 = socket(AF_INET, SOCK_STREAM, 0);
    if (mysocket1 == -1) {
        printf("socket() call failed");
    }
    int status;
    status = connect(mysocket1, (struct sockaddr *) &address, sizeof(struct sockaddr_in));
    if (status == -1) {
        printf("connect() call failed");
    }
    int count;
    char snd_message[100] = {"hello"};
    count = send(mysocket1, snd_message, 5, 0);
    if (count == -1) {
        printf("send() call failed.");
    }
    close(mysocket1);
}
