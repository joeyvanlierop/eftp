#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#include "file.h"
#include "session.h"
#include "socket.h"
#include "messages.h"
#include "errors.h"

void session(std::vector<std::uint8_t> buffer, sockaddr_in client_address, std::string username, std::string password, int session, std::string working_directory)
{
	// Validate opcode
	Opcode opcode = decodeOpcode(buffer);
	if (opcode != Opcode::AUTH)
	{
		std::cout << "Expected auth message, received: " << (std::uint16_t)opcode << std::endl;
		return;
	}

	// Decode message
	AuthMessage message = decodeAuthMessage(buffer);
	std::cout << "Received auth message with username: " << message.username << ", password: " << message.password << std::endl;

	// Create and bind the session to a random port
	int sockfd = create_socket(true);

	// Validate credentials
	if (username != message.username || password != message.password)
	{
		// Send the error message and exit
		ErrorMessage error;
		error.message = "Invalid credentials";
		auto error_buffer = encodeErrorMessage(error);
		send_data(sockfd, client_address, error_buffer);
		close(sockfd);
		return;
	}

	// Send ack message with new session information
	send_ack(sockfd, client_address, session, 0, 0);

	// Wait for a read or write request message to arrive
	std::vector<std::uint8_t> req_buffer;
	try
	{
		std::tie(std::ignore, req_buffer) = receive_data(sockfd, client_address);
	}
	catch (eftp_exception const &e)
	{
		std::cout << e.what() << std::endl;
		close(sockfd);
		return;
	}

	// Process message
	// Should be a read or write request
	opcode = decodeOpcode(req_buffer);
	if (opcode == Opcode::RRQ)
	{
		ReadRequestMessage rrq = decodeReadRequestMessage(req_buffer);
		std::cout << "Received read request message with session: " << rrq.session << ", filename: " << rrq.filename << std::endl;
		try
		{
			send_file(sockfd, client_address, session, rrq.filename, working_directory);
		}
		catch (eftp_exception const &e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	else if (opcode == Opcode::WRQ)
	{
		WriteRequestMessage wrq = decodeWriteRequestMessage(req_buffer);
		std::cout << "Received write request message with session: " << wrq.session << ", filename: " << wrq.filename << std::endl;
		send_ack(sockfd, client_address, session, 1, 0);
		try
		{
			receive_file(sockfd, client_address, session, wrq.filename, working_directory);
		}
		catch (eftp_exception const &e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	else
	{
		std::cout << "Expected read request message or write request message" << std::endl;
	}

	// Close
	close(sockfd);
}
