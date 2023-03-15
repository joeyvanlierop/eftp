#include "messages.h"
#include <cstring>
#include <iostream>

std::vector<std::uint8_t> encodeAuthMessage(const AuthMessage &message)
{
  std::vector<std::uint8_t> data(AUTH_SIZE, 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], message.username.c_str(), message.username.length());
  std::memset(&data[4 + message.username.length()], 0, 1);
  std::memcpy(&data[34 + 1], message.password.c_str(), message.password.length());
  std::memset(&data[34 + 1 + message.password.length()], 0, 1);
  return data;
}

std::vector<std::uint8_t> encodeReadRequestMessage(const ReadRequestMessage &message)
{
  std::vector<std::uint8_t> data(RRQ_SIZE, 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], &message.session, 2);
  std::memcpy(&data[4], message.filename.c_str(), message.filename.length());
  std::memset(&data[4 + message.filename.length()], 0, 1);
  return data;
}

std::vector<std::uint8_t> encodeWriteRequestMessage(const WriteRequestMessage &message)
{
  std::vector<std::uint8_t> data(WRQ_SIZE, 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], &message.session, 2);
  std::memcpy(&data[4], message.filename.c_str(), message.filename.length());
  std::memset(&data[4 + message.filename.length()], 0, 1);
  return data;
}

std::vector<std::uint8_t> encodeDataMessage(const DataMessage &message)
{
  std::vector<std::uint8_t> data(DATA_HEADER_SIZE + message.data.size(), 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], &message.session, 2);
  std::memcpy(&data[4], &message.block, 2);
  std::memcpy(&data[6], &message.segment, 1);
  std::memcpy(&data[7], &message.data[0], message.data.size());
  return data;
}

std::vector<std::uint8_t> encodeAckMessage(const AckMessage &message)
{
  std::vector<std::uint8_t> data(ACK_SIZE, 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], &message.session, 2);
  std::memcpy(&data[4], &message.block, 2);
  std::memcpy(&data[6], &message.segment, 1);
  return data;
}

std::vector<std::uint8_t> encodeErrorMessage(const ErrorMessage &message)
{
  std::vector<std::uint8_t> data(ERROR_SIZE, 0);
  std::memcpy(&data[0], &message.opcode, 2);
  std::memcpy(&data[2], message.message.c_str(), message.message.length());
  std::memset(&data[2 + message.message.length()], 0, 1);
  return data;
}

Opcode decodeOpcode(const std::vector<std::uint8_t> &data)
{
  Opcode opcode;
  std::memcpy(&opcode, &data[0], 2);
  return opcode;
}

AuthMessage decodeAuthMessage(const std::vector<std::uint8_t> &data)
{
  AuthMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  message.username = std::string(reinterpret_cast<const char *>(&data[2]), 32);
  message.password = std::string(reinterpret_cast<const char *>(&data[35]), 32);
  message.username.erase(message.username.find('\0'));
  message.password.erase(message.password.find('\0'));
  return message;
}

ReadRequestMessage decodeReadRequestMessage(const std::vector<std::uint8_t> &data)
{
  ReadRequestMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  std::memcpy(&message.session, &data[2], 2);
  message.filename = std::string(reinterpret_cast<const char *>(&data[4]), 255);
  return message;
}

WriteRequestMessage decodeWriteRequestMessage(const std::vector<std::uint8_t> &data)
{
  WriteRequestMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  std::memcpy(&message.session, &data[2], 2);
  message.filename = std::string(reinterpret_cast<const char *>(&data[4]), 255);
  return message;
}

DataMessage decodeDataMessage(const std::vector<std::uint8_t> &data)
{
  DataMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  std::memcpy(&message.session, &data[2], 2);
  std::memcpy(&message.block, &data[4], 2);
  std::memcpy(&message.segment, &data[6], 1);
  message.data.resize(data.size() - DATA_HEADER_SIZE);
  std::memcpy(&message.data[0], &data[7], message.data.size());
  return message;
}

AckMessage decodeAckMessage(const std::vector<std::uint8_t> &data)
{
  AckMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  std::memcpy(&message.session, &data[2], 2);
  std::memcpy(&message.block, &data[4], 2);
  std::memcpy(&message.segment, &data[6], 1);
  return message;
}

ErrorMessage decodeErrorMessage(const std::vector<std::uint8_t> &data)
{
  ErrorMessage message;
  std::memcpy(&message.opcode, &data[0], 2);
  message.message = std::string(reinterpret_cast<const char *>(&data[2]), 512);
  return message;
}