#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Eftp message opcodes
enum class Opcode : std::uint16_t
{
  AUTH = 1,
  RRQ = 2,
  WRQ = 3,
  DATA = 4,
  ACK = 5,
  ERROR = 6,
};

// Auth message
struct AuthMessage
{
  Opcode opcode = Opcode::AUTH;
  std::string username;
  std::string password;
};

// Read request message structure
struct ReadRequestMessage
{
  Opcode opcode = Opcode::RRQ;
  std::uint16_t session;
  std::string filename;
};

// Write request message structure
struct WriteRequestMessage
{
  Opcode opcode = Opcode::WRQ;
  std::uint16_t session;
  std::string filename;
};

// EFTP data message structure
struct DataMessage
{
  Opcode opcode = Opcode::DATA;
  std::uint16_t session;
  std::uint16_t block;
  std::uint8_t segment;
  std::vector<std::uint8_t> data;
};

// EFTP acknowledgment message structure
struct AckMessage
{
  Opcode opcode = Opcode::ACK;
  std::uint16_t session;
  std::uint16_t block;
  std::uint8_t segment;
};

// EFTP error message structure
struct ErrorMessage
{
  Opcode opcode = Opcode::ERROR;
  std::string message;
};

// Message structure sizes
constexpr std::size_t AUTH_SIZE = 2 + 32 + 1 + 32 + 1;
constexpr std::size_t RRQ_SIZE = 2 + 2 + 255 + 1;
constexpr std::size_t WRQ_SIZE = 2 + 2 + 255 + 1;
constexpr std::size_t DATA_HEADER_SIZE = 2 + 2 + 2 + 1;
constexpr std::size_t ACK_SIZE = 2 + 2 + 2 + 1;
constexpr std::size_t ERROR_SIZE = 2 + 512 + 1;

// Message encoding functions
std::vector<std::uint8_t> encodeAuthMessage(const AuthMessage &message);
std::vector<std::uint8_t> encodeReadRequestMessage(const ReadRequestMessage &message);
std::vector<std::uint8_t> encodeWriteRequestMessage(const WriteRequestMessage &message);
std::vector<std::uint8_t> encodeDataMessage(const DataMessage &message);
std::vector<std::uint8_t> encodeAckMessage(const AckMessage &message);
std::vector<std::uint8_t> encodeErrorMessage(const ErrorMessage &message);

// Message decoding functions
Opcode decodeOpcode(const std::vector<std::uint8_t> &data);
AuthMessage decodeAuthMessage(const std::vector<std::uint8_t> &data);
ReadRequestMessage decodeReadRequestMessage(const std::vector<std::uint8_t> &data);
WriteRequestMessage decodeWriteRequestMessage(const std::vector<std::uint8_t> &data);
DataMessage decodeDataMessage(const std::vector<std::uint8_t> &data);
AckMessage decodeAckMessage(const std::vector<std::uint8_t> &data);
ErrorMessage decodeErrorMessage(const std::vector<std::uint8_t> &data);