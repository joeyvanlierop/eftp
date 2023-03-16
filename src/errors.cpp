#include "errors.h"

eftp_exception::eftp_exception(std::string message) : message("Error: " + message) {}

const char *eftp_exception::what() const noexcept
{
    return message.c_str();
}

error_message::error_message(std::string message) : eftp_exception(message) {}

io_error::io_error(std::string message) : eftp_exception(message) {}

receive_error::receive_error(std::string message) : eftp_exception(message) {}

unexpected_message::unexpected_message(std::string message) : receive_error(message) {}

send_error::send_error(std::string message) : eftp_exception(message) {}

timeout_error::timeout_error(std::string message) : eftp_exception(message) {}

socket_error::socket_error(std::string message) : eftp_exception(message) {}