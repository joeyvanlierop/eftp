#include <string>
#include <iostream>
#include <exception>

class eftp_exception : std::exception
{
private:
    std::string message;

public:
    eftp_exception(std::string message);

    const char *what() const noexcept override;
};

struct error_message : public eftp_exception
{
    error_message(std::string message);
};

struct io_error : public eftp_exception
{
    io_error(std::string message);
};

struct receive_error : public eftp_exception
{
    receive_error(std::string message);
};

struct unexpected_message : public receive_error
{
    unexpected_message(std::string message);
};

struct send_error : public eftp_exception
{
    send_error(std::string message);
};

struct timeout_error : public eftp_exception
{
    timeout_error(std::string message);
};

struct socket_error : public eftp_exception
{
    socket_error(std::string message);
};