#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

int main() {
    printf("This is the server\n");

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(2000);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    int mysocket1;
    mysocket1 = socket(AF_INET, SOCK_STREAM, 0);
    if (mysocket1 == -1) {
        printf("socket() call failed\n");
    }

    int status;
    status = bind(mysocket1, (struct sockaddr *)
            &address, sizeof(struct sockaddr_in));
    if (status == -1) {
        printf("bind() call failed\n");
    }

    status = listen(mysocket1, 5);
    if (status == -1) {
        printf("listen() call failed\n");
    }

    int mysocket2;
    mysocket2 = accept(mysocket1, NULL, NULL);
    if (mysocket2 == -1) {
        printf("accept() call failed\n");
    }

    int count;
    char rcv_message[100];
    count = recv(mysocket2, rcv_message, 100, 0);
    if (count == -1) {
        printf("recv() call failed\n");
    }
    printf("%s\n", rcv_message);

    close(mysocket2);
    close(mysocket1);
}
