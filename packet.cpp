#include "header.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

Packet* PacketFactory::GetPacket(uint16_t packetId) {
    if (packetId == loginPacket) return new LoginPacket();
    else if (packetId == loginResponsePacket) return new LoginResponsePacket();
    else if (packetId == informationPacket) return new InformationPacket();
    else if (packetId == informationResponsePacket) return new InformationResponsePacket();
    else return nullptr;
}

uint16_t CalculateChecksum(const std::vector<uint8_t>& buffer) {
    // Initialize MD5 context
    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    // Update the context with your data
    MD5_Update(&md5Context, buffer.data(), buffer.size());

    // Finalize the MD5 checksum
    uint8_t md5Checksum[MD5_DIGEST_LENGTH];
    MD5_Final(md5Checksum, &md5Context);

    // Convert the 2 last bytes MD5 checksum to a uint16_t
    uint16_t checksumtemp;
    checksumtemp = static_cast<uint16_t>(md5Checksum[MD5_DIGEST_LENGTH - 2]) << 8;
    checksumtemp |= static_cast<uint16_t>(md5Checksum[MD5_DIGEST_LENGTH - 1]);

    return checksumtemp;
}

bool ValidateChecksum(std::vector<uint8_t> buffer, uint16_t& checksum) {
    buffer.erase(buffer.end() - 2, buffer.end());
    // Calculate the checksum of the buffer
    uint16_t calculatedChecksum = CalculateChecksum(buffer);

    // Compare the checksums
    if (calculatedChecksum != checksum) return false;

    return true;
}

// Helper function to serialize a uint16_t into the buffer
void Packet::SerializeUInt16(std::vector<uint8_t>& buffer, uint16_t value) const {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Helper function to serialize a uint32_t into the buffer
void Packet::SerializeUInt32(std::vector<uint8_t>& buffer, uint32_t value) const {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Helper function to serialize a string into the buffer
void Packet::SerializeString(std::vector<uint8_t>& buffer, const std::string& value) const {
    // Serialize the string length as a uint16_t
    SerializeUInt16(buffer, static_cast<uint16_t>(value.length()));

    // Serialize the string data
    for (char c : value) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

// Helper function to deserialize a uint16_t from the buffer
bool Packet::DeserializeUInt16(const std::vector<uint8_t>& buffer, size_t& offset, uint16_t& value){
    if (offset + 2 > buffer.size()) return false;
    value = static_cast<uint16_t>(buffer[offset]) << 8;
    value |= static_cast<uint16_t>(buffer[offset + 1]);
    offset += 2;
    return true;
}

// Helper function to deserialize a uint32_t from the buffer
bool Packet::DeserializeUInt32(const std::vector<uint8_t>& buffer, size_t& offset, uint32_t& value){
    if (offset + 4 > buffer.size()) return false;
    value = static_cast<uint32_t>(buffer[offset]) << 24;
    value |= static_cast<uint32_t>(buffer[offset + 1]) << 16;
    value |= static_cast<uint32_t>(buffer[offset + 2]) << 8;
    value |= static_cast<uint32_t>(buffer[offset + 3]);
    offset += 4;
    return true;
}

// Helper function to deserialize a string from the buffer
bool Packet::DeserializeString(const std::vector<uint8_t>& buffer, size_t& offset, std::string& value){
    // Deserialize the string length as a uint16_t
    uint16_t length;
    DeserializeUInt16(buffer, offset, length);

    // Deserialize the string data
    for (size_t i = 0; i < length; ++i) {
        value += static_cast<char>(buffer[offset + i]);
    }
    offset += length;

    return true;
}

// Helper function to get the packetId from the buffer
uint16_t Packet::GetPacketId(std::vector<uint8_t>& buffer){
    uint16_t PacketId;
    size_t offset = 2;
    DeserializeUInt16(buffer, offset, PacketId);
    return PacketId;
}

void Packet::SerializePacketStarting(std::vector<uint8_t>& buffer) const {
    SerializeUInt16(buffer, startMarker);
    SerializeUInt16(buffer, packetId);
    SerializeString(buffer, deviceId);
    SerializeUInt16(buffer, packetOrderIndex);
}

void Packet::SerializePacketEnding(std::vector<uint8_t>& buffer) const {
    SerializeUInt16(buffer, checksum);
    SerializeUInt16(buffer, endMarker);
}

bool Packet::DeserializePacketStarting(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (!DeserializeUInt16(buffer, offset, startMarker)) return false;
    if (!DeserializeUInt16(buffer, offset, packetId)) return false;
    if (!DeserializeString(buffer, offset, deviceId)) return false;
    if (!DeserializeUInt16(buffer, offset, packetOrderIndex)) return false;
    return true;
}

bool Packet::DeserializePacketEnding(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (!DeserializeUInt16(buffer, offset, checksum)) return false;
    if (!DeserializeUInt16(buffer, offset, endMarker)) return false;
    return true;
}

bool InformationPacket::DeserializePacketBody(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (!DeserializeUInt32(buffer, offset, latitude)) return false;
    if (!DeserializeUInt32(buffer, offset, longitude)) return false;
    return true;
}

bool InformationPacket::Deserialize(const std::vector<uint8_t>& buffer) {
    size_t offset = 0;
    if (!DeserializePacketStarting(buffer, offset)) return false;
    if (!DeserializePacketBody(buffer, offset)) return false;
    if (!DeserializePacketEnding(buffer, offset)) return false;
    return true;
}

void InformationPacket::PrintInformation() {
    std::cout << "Information packet startMarker: " << startMarker << std::endl;
    std::cout << "Information packet packetId: " << packetId << std::endl;
    std::cout << "Information packet deviceId: " << deviceId << std::endl;
    std::cout << "Information packet packetOrderIndex: " << packetOrderIndex << std::endl;
    std::cout << "Information packet latitude: " << latitude << std::endl;
    std::cout << "Information packet longitude: " << longitude << std::endl;
    std::cout << "Information packet checksum: " << checksum << std::endl;
    std::cout << "Information packet endMarker: " << endMarker << std::endl << std::endl;
}

void InformationResponsePacket::FillResponseInformation(Packet* receivedPacket) {
    receivedPacket = dynamic_cast<InformationPacket*>(receivedPacket);
    startMarker = receivedPacket->startMarker;
    packetId = receivedPacket->packetId;
    deviceId = receivedPacket->deviceId;
    packetOrderIndex = receivedPacket->packetOrderIndex + 1;
    receivedPacketIndex = receivedPacket->packetOrderIndex;
    checksum = receivedPacket->checksum;
    endMarker = receivedPacket->endMarker;
}

void InformationResponsePacket::SerializePacketBody(std::vector<uint8_t>& buffer) const {
    SerializeUInt16(buffer, receivedPacketIndex);
}

std::vector<uint8_t> InformationResponsePacket::Serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(2 + 2 + deviceId.size() + 2 + 2 + 2 + 2);

    SerializePacketStarting(buffer);
    SerializePacketBody(buffer);
    SerializePacketEnding(buffer);

    return buffer;
}

bool LoginPacket::DeserializePacketBody(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (!DeserializeString(buffer, offset, username)) return false;
    if (!DeserializeString(buffer, offset, password)) return false;
    return true;
}

bool LoginPacket::Deserialize(const std::vector<uint8_t>& buffer) {
    size_t offset = 0;
    if (!DeserializePacketStarting(buffer, offset)) return false;
    if (!DeserializePacketBody(buffer, offset)) return false;
    if (!DeserializePacketEnding(buffer, offset)) return false;
    return true;
}

void LoginPacket::PrintInformation() {
    std::cout << "Login packet startMarker: " << startMarker << std::endl;
    std::cout << "Login packet packetId: " << packetId << std::endl;
    std::cout << "Login packet deviceId: " << deviceId << std::endl;
    std::cout << "Login packet packetOrderIndex: " << packetOrderIndex << std::endl;
    std::cout << "Login packet username: " << username << std::endl;
    std::cout << "Login packet password: " << password << std::endl;
    std::cout << "Login packet checksum: " << checksum << std::endl;
    std::cout << "Login packet endMarker: " << endMarker << std::endl << std::endl;
}

void LoginResponsePacket::FillResponseInformation(Packet* receivedPacket) {
    receivedPacket = dynamic_cast<LoginPacket*>(receivedPacket);
    startMarker = receivedPacket->startMarker;
    packetId = receivedPacket->packetId;
    deviceId = receivedPacket->deviceId;
    packetOrderIndex = receivedPacket->packetOrderIndex + 1;
    receivedPacketIndex = receivedPacket->packetOrderIndex;
    checksum = receivedPacket->checksum;
    endMarker = receivedPacket->endMarker;
}

void LoginResponsePacket::SerializePacketBody(std::vector<uint8_t>& buffer) const {
    SerializeUInt16(buffer, receivedPacketIndex);
}

std::vector<uint8_t> LoginResponsePacket::Serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(2 + 2 + deviceId.size() + 2 + 2 + 2 + 2);

    SerializePacketStarting(buffer);
    SerializePacketBody(buffer);
    SerializePacketEnding(buffer);

    return buffer;
}