#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 5
#define MAX_UCID_LEN 8
#define MAX_DATETIME_LEN 25
#define MAX_FILE_LEN 3

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	int port = atoi(argv[1]);
	int server_socket, client_socket, client_len, bytes_received, passcode, user_passcode, read_size;
	struct sockaddr_in server_address, client_address;
	char ucid[MAX_UCID_LEN];
	char datetime[MAX_DATETIME_LEN];
	char file_contents[MAX_FILE_LEN];
	time_t now;
	struct tm *timeinfo;
	FILE *file;

	// Create server socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		printf("Error creating server socket\n");
		exit(EXIT_FAILURE);
	}

	// Bind server socket to a specific port
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		printf("Error binding server socket\n");
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(server_socket, MAX_CLIENTS) < 0)
	{
		printf("Error listening for incoming connections\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// Accept a new connection
		client_len = sizeof(client_address);
		client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
		if (client_socket < 0)
		{
			printf("Error accepting a new connection\n");
			continue;
		}

		// Receive user's UCID from the client
		bytes_received = recv(client_socket, ucid, MAX_UCID_LEN, 0);
		if (bytes_received < 0)
		{
			printf("Error receiving user's UCID\n");
			close(client_socket);
			continue;
		}
		printf("Received UCID: %d\n", atoi(ucid));

		// Send current day and time to the connected client
		time(&now);
		timeinfo = localtime(&now);
		strftime(datetime, MAX_DATETIME_LEN, "%F %T", timeinfo);
		datetime[MAX_DATETIME_LEN] = '\0';
		if (send(client_socket, datetime, MAX_DATETIME_LEN, 0) < 0)
		{
			printf("Error sending current day and time\n");
			close(client_socket);
		}
		printf("Sent current day and time: %s\n", datetime);

		// Receive user's passcode from the client
		bytes_received = recv(client_socket, &user_passcode, 4, 0);
		if (bytes_received < 0)
		{
			printf("Error receiving user's passcode\n");
			close(client_socket);
			continue;
		}

		// Calculate passcode
		passcode = (int)timeinfo->tm_sec + atoi(ucid) % 10000;
		printf("Generated passcode: %d\n", passcode);

		// Compare received password
		if (user_passcode != passcode)
		{
			printf("Received mismatched passcode: %d\n", user_passcode);
			close(client_socket);
			continue;
		}
		printf("Received passcode: %d\n", user_passcode);

		// Open the file "data.txt" from the run directory
		file = fopen("data.txt", "r");
		if (file == NULL)
		{
			printf("Error opening data.txt\n");
			close(client_socket);
			continue;
		}

		while(!feof(file)) {
			// Read the file contents into the file_contents buffer
			read_size = fread(file_contents, 1, MAX_FILE_LEN, file);

			// Send the data to the client
			if (send(client_socket, file_contents, read_size, 0) < 0)
			{
				printf("Error sending data.txt\n");
				continue;
			}
			printf("Sent file content: (%d bytes)\n", read_size);
		}

		// Close the file and socket
		fclose(file);
		close(client_socket);
	}
}
