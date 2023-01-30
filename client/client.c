#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
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
	int client_socket, bytes_sent, bytes_received, passcode;
	struct sockaddr_in server_address;
	char ucid[MAX_UCID_LEN];
	char datetime[MAX_DATETIME_LEN];
	char file_contents[MAX_FILE_LEN + 1];
	FILE *file;

	// Get user's UCID as input
	printf("Enter your UCID: ");
	scanf("%s", ucid);

	// Create a TCP connection with the server
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		printf("Error creating client socket\n");
		exit(EXIT_FAILURE);
	}
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_address.sin_port = htons(port);
	if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		printf("Error connecting to server\n");
		exit(EXIT_FAILURE);
	}

	// Send user's UCID to the server
	bytes_sent = send(client_socket, ucid, MAX_UCID_LEN, 0);
	if (bytes_sent < 0)
	{
		printf("Error sending user's UCID to server\n");
		close(client_socket);
		exit(EXIT_FAILURE);
	}

	// Read the current time sent by the server
	bytes_received = recv(client_socket, datetime, MAX_DATETIME_LEN, 0);
	if (bytes_received < 0)
	{
		printf("Error receiving current day and time from server\n");
		close(client_socket);
		exit(EXIT_FAILURE);
	}
	printf("Received current day and time: %s\n", datetime);

	// Create the passcode
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(struct tm));
	strptime(datetime, "%F %T", &timeinfo);
	passcode = timeinfo.tm_sec + atoi(ucid) % 10000;

	// Send the passcode to the server
	bytes_sent = send(client_socket, (const char *)&passcode, 4, 0);
	if (bytes_sent < 0)
	{
		printf("Error sending passcode to server\n");
		close(client_socket);
		exit(EXIT_FAILURE);
	}
	printf("Sent passcode: %d\n", passcode);

	// Save the file contents into a text file in the run directory
	file = fopen("received_file.txt", "w");
	if (file == NULL)
	{
		printf("Error opening file\n");
		close(client_socket);
		exit(EXIT_FAILURE);
	}

	// Read the file contents sent by the server
	while((bytes_received = recv(client_socket, file_contents, MAX_FILE_LEN, 0)) > 0) {
		printf("Received file contents: (%d bytes)\n", bytes_received);
		//if (bytes_received < 0)
		//{
		//	printf("Error receiving file contents from server\n");
		//	close(client_socket);
		//	exit(EXIT_FAILURE);
		//}
		file_contents[bytes_received] = '\0';
		if (fputs(file_contents, file) == EOF)
		{
			printf("Error writing file contents to file\n");
			fclose(file);
			close(client_socket);
			exit(EXIT_FAILURE);
		}
	}

	// Close the file stream and socket
	fclose(file);
	close(client_socket);
	return EXIT_SUCCESS;
}
