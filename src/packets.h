#include <cstdint>
#include <string>
#include <vector>

// EFTP packet types
enum class PacketType : std::uint16_t
{
  AUTH = 1,
  RRQ = 2,
  WRQ = 3,
  DATA = 4,
  ACK = 5,
  ERROR = 6,
};

// Auth packet
struct AuthPacket
{
  PacketType opcode = PacketType::AUTH;
  std::string username;
  std::string password;
};

// Read request packet structure
struct ReadRequestPacket
{
  PacketType opcode = PacketType::RRQ;
  std::uint16_t session;
  std::string filename;
};

// Write request packet structure
struct WriteRequestPacket
{
  PacketType opcode = PacketType::WRQ;
  std::uint16_t session;
  std::string filename;
};

// EFTP data packet structure
struct DataPacket
{
  PacketType opcode = PacketType::DATA;
  std::uint16_t session;
  std::uint16_t block;
  std::uint8_t segment;
  std::vector<std::uint8_t> data;
};

// EFTP acknowledgment packet structure
struct AckPacket
{
  PacketType opcode = PacketType::ACK;
  std::uint16_t session;
  std::uint16_t block;
  std::uint8_t segment;
};

// EFTP error packet structure
struct ErrorPacket
{
  PacketType opcode = PacketType::ERROR;
  std::string message;
};

// Packet structure sizes
constexpr std::size_t MESSAGE_PACKET_SIZE = 4 + 32 + 1 + 32 + 1;
constexpr std::size_t REQUEST_PACKET_SIZE = 4 + 2 + 255 + 1;
constexpr std::size_t DATA_PACKET_HEADER_SIZE = 4 + 2 + 2 + 2;
constexpr std::size_t ACK_PACKET_SIZE = 4 + 2 + 2 + 2;
constexpr std::size_t ERROR_PACKET_SIZE = 4 + 255 + 1;

// Packet encoding functions
std::vector<std::uint8_t> encodeAuthPacket(const AuthPacket &packet);
std::vector<std::uint8_t> encodeReadRequestPacket(const ReadRequestPacket &packet);
std::vector<std::uint8_t> encodeWriteRequestPacket(const WriteRequestPacket &packet);
std::vector<std::uint8_t> encodeDataPacket(const DataPacket &packet);
std::vector<std::uint8_t> encodeAckPacket(const AckPacket &packet);
std::vector<std::uint8_t> encodeErrorPacket(const ErrorPacket &packet);

// Packet decoding functions
AuthPacket decodeAuthPacket(const std::vector<std::uint8_t> &data);
ReadRequestPacket decodeReadRequestPacket(const std::vector<std::uint8_t> &data);
WriteRequestPacket decodeWriteRequestPacket(const std::vector<std::uint8_t> &data);
DataPacket decodeDataPacket(const std::vector<std::uint8_t> &data);
AckPacket decodeAckPacket(const std::vector<std::uint8_t> &data);
ErrorPacket decodeErrorPacket(const std::vector<std::uint8_t> &data);