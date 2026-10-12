#ifndef PACKETPROTOCOL_H
#define PACKETPROTOCOL_H

#include <cstdint>
#include <QByteArray>
#include <QString>
#include <QDateTime>

namespace Protocol {

// Protocol constants
constexpr uint8_t SYNC_BYTE_1 = 0xAA;
constexpr uint8_t SYNC_BYTE_2 = 0x55;
constexpr uint8_t MAX_PAYLOAD_SIZE = 64;
constexpr uint8_t HEADER_SIZE = 6;      // Sync1, Sync2, Type, Len, SeqL, SeqH
constexpr uint8_t FOOTER_SIZE = 2;      // CrcL, CrcH
constexpr uint8_t MIN_PACKET_SIZE = HEADER_SIZE + FOOTER_SIZE; // 8 bytes

// Message Types
enum MessageType : uint8_t {
    MSG_ANGLE_COMMAND   = 0x01,
    MSG_ANGLE_TELEMETRY = 0x02,
    MSG_ACK             = 0x03,
    MSG_PING            = 0x04,
    MSG_UNKNOWN         = 0xFF
};

// Parse Status Results
enum class ParseStatus {
    NeedMoreData,
    ValidPacket,
    CrcError,
    MalformedPacket,
    InvalidPayloadValue
};

// Parsed Packet Data Structure
struct ParsedPacket {
    uint8_t messageType = 0;
    QString messageTypeName;
    uint16_t sequenceNumber = 0;
    uint8_t payloadLength = 0;
    QByteArray payload;
    uint16_t receivedCrc = 0;
    uint16_t calculatedCrc = 0;
    bool crcValid = false;

    // Angle fields (if type is ANGLE_COMMAND or ANGLE_TELEMETRY)
    bool isAnglePacket = false;
    uint16_t rawAzimuth = 0;
    uint16_t rawElevation = 0;
    double azimuthDegrees = 0.0;
    double elevationDegrees = 0.0;

    // Diagnostics
    QByteArray rawBytes;
    QString rawBytesHex;
    QString sourceInfo;
    QDateTime timestamp;
    QString errorMessage;
};

// CRC-16/CCITT-FALSE Calculation
// Poly: 0x1021, Init: 0xFFFF, RefIn: false, RefOut: false, XorOut: 0x0000
uint16_t calculateCrc16CcittFalse(const uint8_t *data, size_t length);
uint16_t calculateCrc16CcittFalse(const QByteArray &data);

// Message Type name helper
QString messageTypeName(uint8_t msgType);

// Angle Conversions (10-bit raw 0-1023)
double rawToAzimuthDegrees(uint16_t raw);
double rawToElevationDegrees(uint16_t raw);
uint16_t azimuthDegreesToRaw(double degrees);
uint16_t elevationDegreesToRaw(double degrees);

// Packet construction helper (used for testing and simulation)
QByteArray buildPacket(uint8_t msgType, uint16_t sequence, const QByteArray &payload);
QByteArray buildAnglePacket(uint8_t msgType, uint16_t sequence, uint16_t azRaw, uint16_t elRaw);

// Stream Packet Extraction
// Inspects and consumes from the front of rxBuffer
ParseStatus extractNextPacket(QByteArray &rxBuffer, ParsedPacket &outPacket);

} // namespace Protocol

#endif // PACKETPROTOCOL_H
