#include "packets.h"
#include <cstring>

std::vector<std::uint8_t> encodeAuthPacket(const AuthPacket& packet) {
  std::vector<std::uint8_t> data(MESSAGE_PACKET_SIZE, 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], packet.username.c_str(), packet.username.length());
  std::memcpy(&data[34], packet.password.c_str(), packet.password.length());
  return data;
}

std::vector<std::uint8_t> encodeReadRequestPacket(const ReadRequestPacket& packet) {
  std::vector<std::uint8_t> data(REQUEST_PACKET_SIZE, 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], &packet.session, 2);
  std::memcpy(&data[4], packet.filename.c_str(), packet.filename.length());
  return data;
}

std::vector<std::uint8_t> encodeWriteRequestPacket(const WriteRequestPacket& packet) {
  std::vector<std::uint8_t> data(REQUEST_PACKET_SIZE, 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], &packet.session, 2);
  std::memcpy(&data[4], packet.filename.c_str(), packet.filename.length());
  return data;
}

std::vector<std::uint8_t> encodeDataPacket(const DataPacket& packet) {
  std::vector<std::uint8_t> data(DATA_PACKET_HEADER_SIZE + packet.data.size(), 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], &packet.session, 2);
  std::memcpy(&data[4], &packet.block, 2);
  std::memcpy(&data[6], &packet.segment, 2);
  std::memcpy(&data[8], &packet.data[0], packet.data.size());
  return data;
}

std::vector<std::uint8_t> encodeAckPacket(const AckPacket& packet) {
  std::vector<std::uint8_t> data(ACK_PACKET_SIZE, 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], &packet.session, 2);
  std::memcpy(&data[4], &packet.block, 2);
  std::memcpy(&data[6], &packet.segment, 2);
  return data;
}

std::vector<std::uint8_t> encodeErrorPacket(const ErrorPacket& packet) {
  std::vector<std::uint8_t> data(ERROR_PACKET_SIZE, 0);
  std::memcpy(&data[0], &packet.opcode, 2);
  std::memcpy(&data[2], packet.message.c_str(), packet.message.length());
  return data;
}

AuthPacket decodeAuthPacket(const std::vector<std::uint8_t>& data) {
  AuthPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  packet.username = std::string(reinterpret_cast<const char*>(&data[2]), 32);
  packet.password = std::string(reinterpret_cast<const char*>(&data[34]), 32);
  return packet;
}

ReadRequestPacket decodeReadRequestPacket(const std::vector<std::uint8_t>& data) {
  ReadRequestPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  std::memcpy(&packet.session, &data[2], 2);
  packet.filename = std::string(reinterpret_cast<const char*>(&data[4]), 255);
  return packet;
}

WriteRequestPacket decodeWriteRequestPacket(const std::vector<std::uint8_t>& data) {
  WriteRequestPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  std::memcpy(&packet.session, &data[2], 2);
  packet.filename = std::string(reinterpret_cast<const char*>(&data[4]), 255);
  return packet;
}

DataPacket decodeDataPacket(const std::vector<std::uint8_t>& data) {
  DataPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  std::memcpy(&packet.session, &data[2], 2);
  std::memcpy(&packet.block, &data[4], 2);
  std::memcpy(&packet.segment, &data[6], 2);
  packet.data.resize(data.size() - DATA_PACKET_HEADER_SIZE);
  std::memcpy(&packet.data[0], &data[8], packet.data.size());
  return packet;
}

AckPacket decodeAckPacket(const std::vector<std::uint8_t>& data) {
  AckPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  std::memcpy(&packet.session, &data[2], 2);
  std::memcpy(&packet.block, &data[4], 2);
  std::memcpy(&packet.segment, &data[6], 2);
  return packet;
}

ErrorPacket decodeErrorPacket(const std::vector<std::uint8_t>& data) {
  ErrorPacket packet;
  std::memcpy(&packet.opcode, &data[0], 2);
  packet.message = std::string(reinterpret_cast<const char*>(&data[2]), 512);
  return packet;
}