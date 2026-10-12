#include "PacketProtocol.h"
#include <cmath>
#include <algorithm>

namespace Protocol {

// Standard table-based or bitwise CRC-16/CCITT-FALSE
uint16_t calculateCrc16CcittFalse(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF; // Initial value
    for (size_t i = 0; i < length; ++i) {
        crc ^= (static_cast<uint16_t>(data[i]) << 8);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

uint16_t calculateCrc16CcittFalse(const QByteArray &data)
{
    return calculateCrc16CcittFalse(reinterpret_cast<const uint8_t*>(data.constData()), static_cast<size_t>(data.size()));
}

QString messageTypeName(uint8_t msgType)
{
    switch (msgType) {
    case MSG_ANGLE_COMMAND:
        return QStringLiteral("ANGLE_COMMAND (0x01)");
    case MSG_ANGLE_TELEMETRY:
        return QStringLiteral("ANGLE_TELEMETRY (0x02)");
    case MSG_ACK:
        return QStringLiteral("ACK (0x03)");
    case MSG_PING:
        return QStringLiteral("PING (0x04)");
    default:
        return QStringLiteral("UNKNOWN (0x%1)").arg(msgType, 2, 16, QLatin1Char('0')).toUpper();
    }
}

double rawToAzimuthDegrees(uint16_t raw)
{
    // Range 0 to 360 degrees
    double deg = static_cast<double>(raw) * 360.0 / 1023.0;
    if (deg < 0.0) deg = 0.0;
    if (deg > 360.0) deg = 360.0;
    return deg;
}

double rawToElevationDegrees(uint16_t raw)
{
    // Range -90 to +90 degrees
    double deg = (static_cast<double>(raw) * 180.0 / 1023.0) - 90.0;
    if (deg < -90.0) deg = -90.0;
    if (deg > 90.0) deg = 90.0;
    return deg;
}

uint16_t azimuthDegreesToRaw(double degrees)
{
    double clamped = std::clamp(degrees, 0.0, 360.0);
    int raw = static_cast<int>(std::round(clamped * 1023.0 / 360.0));
    return static_cast<uint16_t>(std::clamp(raw, 0, 1023));
}

uint16_t elevationDegreesToRaw(double degrees)
{
    double clamped = std::clamp(degrees, -90.0, 90.0);
    int raw = static_cast<int>(std::round((clamped + 90.0) * 1023.0 / 180.0));
    return static_cast<uint16_t>(std::clamp(raw, 0, 1023));
}

QByteArray buildPacket(uint8_t msgType, uint16_t sequence, const QByteArray &payload)
{
    QByteArray packet;
    packet.reserve(HEADER_SIZE + payload.size() + FOOTER_SIZE);

    // Sync bytes
    packet.append(static_cast<char>(SYNC_BYTE_1));
    packet.append(static_cast<char>(SYNC_BYTE_2));

    // Message Type
    packet.append(static_cast<char>(msgType));

    // Payload Length
    uint8_t payloadLen = static_cast<uint8_t>(std::min(static_cast<size_t>(payload.size()), static_cast<size_t>(MAX_PAYLOAD_SIZE)));
    packet.append(static_cast<char>(payloadLen));

    // Sequence Number (Little-Endian)
    packet.append(static_cast<char>(sequence & 0xFF));
    packet.append(static_cast<char>((sequence >> 8) & 0xFF));

    // Payload
    if (payloadLen > 0) {
        packet.append(payload.constData(), payloadLen);
    }

    // CRC over message type, payload length, sequence bytes, and payload (excluding sync bytes)
    // Offset 2 to end of payload
    const uint8_t *crcData = reinterpret_cast<const uint8_t*>(packet.constData()) + 2;
    size_t crcLen = 4 + payloadLen; // Type(1) + Len(1) + Seq(2) + Payload(payloadLen)
    uint16_t crc = calculateCrc16CcittFalse(crcData, crcLen);

    // Append CRC (Little-Endian: low byte first, high byte second)
    packet.append(static_cast<char>(crc & 0xFF));
    packet.append(static_cast<char>((crc >> 8) & 0xFF));

    return packet;
}

QByteArray buildAnglePacket(uint8_t msgType, uint16_t sequence, uint16_t azRaw, uint16_t elRaw)
{
    azRaw &= 0x03FF; // Enforce 10-bit
    elRaw &= 0x03FF;

    QByteArray payload;
    payload.resize(4);
    payload[0] = static_cast<char>(azRaw & 0xFF);
    payload[1] = static_cast<char>((azRaw >> 8) & 0xFF);
    payload[2] = static_cast<char>(elRaw & 0xFF);
    payload[3] = static_cast<char>((elRaw >> 8) & 0xFF);

    return buildPacket(msgType, sequence, payload);
}

ParseStatus extractNextPacket(QByteArray &rxBuffer, ParsedPacket &outPacket)
{
    while (!rxBuffer.isEmpty()) {
        // Step 1: Locate sync sequence 0xAA 0x55
        int syncIndex = -1;
        for (int i = 0; i < rxBuffer.size() - 1; ++i) {
            if (static_cast<uint8_t>(rxBuffer[i]) == SYNC_BYTE_1 &&
                static_cast<uint8_t>(rxBuffer[i + 1]) == SYNC_BYTE_2) {
                syncIndex = i;
                break;
            }
        }

        if (syncIndex == -1) {
            // No sync sequence found in entire buffer.
            // If the last byte is SYNC_BYTE_1, keep it in case SYNC_BYTE_2 arrives next.
            if (!rxBuffer.isEmpty() && static_cast<uint8_t>(rxBuffer.back()) == SYNC_BYTE_1) {
                rxBuffer = rxBuffer.right(1);
            } else {
                rxBuffer.clear();
            }
            return ParseStatus::NeedMoreData;
        }

        // Discard any noise prior to the sync bytes
        if (syncIndex > 0) {
            rxBuffer.remove(0, syncIndex);
        }

        // Now rxBuffer starts with 0xAA 0x55
        // Need at least 6 bytes for header
        if (rxBuffer.size() < HEADER_SIZE) {
            return ParseStatus::NeedMoreData;
        }

        uint8_t msgType = static_cast<uint8_t>(rxBuffer[2]);
        uint8_t payloadLen = static_cast<uint8_t>(rxBuffer[3]);

        // Validate payload length
        if (payloadLen > MAX_PAYLOAD_SIZE) {
            // Malformed header / invalid payload length
            outPacket.rawBytes = rxBuffer.left(HEADER_SIZE);
            outPacket.rawBytesHex = outPacket.rawBytes.toHex(' ').toUpper();
            outPacket.errorMessage = QStringLiteral("Payload length %1 exceeds maximum allowed (%2)").arg(payloadLen).arg(MAX_PAYLOAD_SIZE);
            outPacket.timestamp = QDateTime::currentDateTime();

            // Discard sync byte to resynchronize on next candidate
            rxBuffer.remove(0, 1);
            return ParseStatus::MalformedPacket;
        }

        int fullPacketSize = HEADER_SIZE + payloadLen + FOOTER_SIZE;
        if (rxBuffer.size() < fullPacketSize) {
            // Waiting for remainder of packet
            return ParseStatus::NeedMoreData;
        }

        // We have the full packet candidate
        QByteArray candidate = rxBuffer.left(fullPacketSize);
        rxBuffer.remove(0, fullPacketSize);

        outPacket.rawBytes = candidate;
        outPacket.rawBytesHex = candidate.toHex(' ').toUpper();
        outPacket.timestamp = QDateTime::currentDateTime();
        outPacket.messageType = msgType;
        outPacket.messageTypeName = messageTypeName(msgType);
        outPacket.payloadLength = payloadLen;

        // Sequence number (little-endian)
        uint16_t seqLow = static_cast<uint8_t>(candidate[4]);
        uint16_t seqHigh = static_cast<uint8_t>(candidate[5]);
        outPacket.sequenceNumber = seqLow | (seqHigh << 8);

        // Payload
        outPacket.payload = candidate.mid(HEADER_SIZE, payloadLen);

        // CRC bytes (little-endian: low byte first, high byte second)
        uint16_t crcLow = static_cast<uint8_t>(candidate[HEADER_SIZE + payloadLen]);
        uint16_t crcHigh = static_cast<uint8_t>(candidate[HEADER_SIZE + payloadLen + 1]);
        outPacket.receivedCrc = crcLow | (crcHigh << 8);

        // Calculate CRC over Type, Len, Seq, and Payload (bytes 2 to 6+payloadLen-1)
        const uint8_t *crcData = reinterpret_cast<const uint8_t*>(candidate.constData()) + 2;
        size_t crcDataLen = 4 + payloadLen;
        outPacket.calculatedCrc = calculateCrc16CcittFalse(crcData, crcDataLen);

        if (outPacket.receivedCrc != outPacket.calculatedCrc) {
            outPacket.crcValid = false;
            outPacket.errorMessage = QStringLiteral("CRC mismatch: received 0x%1 != calculated 0x%2")
                                         .arg(outPacket.receivedCrc, 4, 16, QLatin1Char('0')).toUpper()
                                         .arg(outPacket.calculatedCrc, 4, 16, QLatin1Char('0')).toUpper();
            return ParseStatus::CrcError;
        }

        outPacket.crcValid = true;

        // Check message type semantics
        if (msgType == MSG_ANGLE_COMMAND || msgType == MSG_ANGLE_TELEMETRY) {
            outPacket.isAnglePacket = true;
            if (payloadLen != 4) {
                outPacket.errorMessage = QStringLiteral("Angle message requires exactly 4 payload bytes, got %1").arg(payloadLen);
                return ParseStatus::MalformedPacket;
            }

            uint16_t rawAz = static_cast<uint8_t>(outPacket.payload[0]) |
                             (static_cast<uint8_t>(outPacket.payload[1]) << 8);
            uint16_t rawEl = static_cast<uint8_t>(outPacket.payload[2]) |
                             (static_cast<uint8_t>(outPacket.payload[3]) << 8);

            // Raw angle values must be in range 0-1023 (upper 6 bits must be 0)
            if (rawAz > 1023 || rawEl > 1023) {
                outPacket.errorMessage = QStringLiteral("Raw angle exceeds 10-bit limit (Az: %1, El: %2, max 1023)")
                                             .arg(rawAz).arg(rawEl);
                outPacket.rawAzimuth = rawAz;
                outPacket.rawElevation = rawEl;
                return ParseStatus::InvalidPayloadValue;
            }

            outPacket.rawAzimuth = rawAz;
            outPacket.rawElevation = rawEl;
            outPacket.azimuthDegrees = rawToAzimuthDegrees(rawAz);
            outPacket.elevationDegrees = rawToElevationDegrees(rawEl);
        }

        return ParseStatus::ValidPacket;
    }

    return ParseStatus::NeedMoreData;
}

} // namespace Protocol
