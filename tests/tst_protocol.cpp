#include <QTest>
#include "../src/PacketProtocol.h"

class TestPacketProtocol : public QObject
{
    Q_OBJECT

private slots:
    void testCrcKnownVector();
    void testValidAnglePacket();
    void testCrcRejection();
    void testIncompletePacket();
    void testConcatenatedPackets();
    void testInvalidPayloadLength();
    void testInvalid10BitValues();
    void testSyncRecoveryAfterNoise();
    void testAngleConversions();
};

void TestPacketProtocol::testCrcKnownVector()
{
    // Standard test vector: "123456789" must produce CRC-16/CCITT-FALSE 0x29B1
    QByteArray vector = "123456789";
    uint16_t crc = Protocol::calculateCrc16CcittFalse(vector);
    QCOMPARE(crc, static_cast<uint16_t>(0x29B1));
}

void TestPacketProtocol::testValidAnglePacket()
{
    // Build packet for Azimuth = 180.0 deg (raw ~512), Elevation = 0.0 deg (raw ~512)
    uint16_t azRaw = Protocol::azimuthDegreesToRaw(180.0);
    uint16_t elRaw = Protocol::elevationDegreesToRaw(0.0);
    QByteArray packetBytes = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_COMMAND, 42, azRaw, elRaw);

    QCOMPARE(packetBytes.size(), 8 + 4); // 12 bytes
    QCOMPARE(static_cast<uint8_t>(packetBytes[0]), Protocol::SYNC_BYTE_1);
    QCOMPARE(static_cast<uint8_t>(packetBytes[1]), Protocol::SYNC_BYTE_2);
    QCOMPARE(static_cast<uint8_t>(packetBytes[2]), static_cast<uint8_t>(Protocol::MSG_ANGLE_COMMAND));

    Protocol::ParsedPacket parsed;
    QByteArray buffer = packetBytes;
    Protocol::ParseStatus status = Protocol::extractNextPacket(buffer, parsed);

    QCOMPARE(status, Protocol::ParseStatus::ValidPacket);
    QVERIFY(parsed.crcValid);
    QCOMPARE(parsed.sequenceNumber, static_cast<uint16_t>(42));
    QCOMPARE(parsed.rawAzimuth, azRaw);
    QCOMPARE(parsed.rawElevation, elRaw);
    QVERIFY(std::abs(parsed.azimuthDegrees - 180.0) < 0.5);
    QVERIFY(std::abs(parsed.elevationDegrees - 0.0) < 0.5);
    QVERIFY(buffer.isEmpty());
}

void TestPacketProtocol::testCrcRejection()
{
    QByteArray packetBytes = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_COMMAND, 1, 100, 200);
    // Corrupt one byte of payload
    packetBytes[6] = static_cast<char>(packetBytes[6] ^ 0xFF);

    Protocol::ParsedPacket parsed;
    QByteArray buffer = packetBytes;
    Protocol::ParseStatus status = Protocol::extractNextPacket(buffer, parsed);

    QCOMPARE(status, Protocol::ParseStatus::CrcError);
    QVERIFY(!parsed.crcValid);
}

void TestPacketProtocol::testIncompletePacket()
{
    QByteArray packetBytes = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_COMMAND, 1, 100, 200);
    QByteArray partial = packetBytes.left(packetBytes.size() - 2); // missing CRC

    Protocol::ParsedPacket parsed;
    QByteArray buffer = partial;
    Protocol::ParseStatus status = Protocol::extractNextPacket(buffer, parsed);

    QCOMPARE(status, Protocol::ParseStatus::NeedMoreData);
    QCOMPARE(buffer.size(), partial.size()); // buffer preserved
}

void TestPacketProtocol::testConcatenatedPackets()
{
    QByteArray pkt1 = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_COMMAND, 10, 100, 200);
    QByteArray pkt2 = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_TELEMETRY, 11, 300, 400);

    QByteArray stream = pkt1 + pkt2;

    Protocol::ParsedPacket p1;
    Protocol::ParseStatus s1 = Protocol::extractNextPacket(stream, p1);
    QCOMPARE(s1, Protocol::ParseStatus::ValidPacket);
    QCOMPARE(p1.sequenceNumber, static_cast<uint16_t>(10));
    QCOMPARE(p1.rawAzimuth, static_cast<uint16_t>(100));

    Protocol::ParsedPacket p2;
    Protocol::ParseStatus s2 = Protocol::extractNextPacket(stream, p2);
    QCOMPARE(s2, Protocol::ParseStatus::ValidPacket);
    QCOMPARE(p2.sequenceNumber, static_cast<uint16_t>(11));
    QCOMPARE(p2.rawAzimuth, static_cast<uint16_t>(300));
    QVERIFY(stream.isEmpty());
}

void TestPacketProtocol::testInvalidPayloadLength()
{
    // Build packet with invalid payload length > 64
    QByteArray corrupt;
    corrupt.append(static_cast<char>(Protocol::SYNC_BYTE_1));
    corrupt.append(static_cast<char>(Protocol::SYNC_BYTE_2));
    corrupt.append(static_cast<char>(Protocol::MSG_ANGLE_COMMAND));
    corrupt.append(static_cast<char>(75)); // Length 75 > 64 max
    corrupt.append(static_cast<char>(0x01));
    corrupt.append(static_cast<char>(0x00));

    Protocol::ParsedPacket parsed;
    QByteArray buffer = corrupt;
    Protocol::ParseStatus status = Protocol::extractNextPacket(buffer, parsed);

    QCOMPARE(status, Protocol::ParseStatus::MalformedPacket);
}

void TestPacketProtocol::testInvalid10BitValues()
{
    // Build packet with raw value > 1023 (e.g. 1024 or 0x0500)
    QByteArray payload;
    payload.resize(4);
    payload[0] = static_cast<char>(0x00);
    payload[1] = static_cast<char>(0x08); // 0x0800 = 2048 > 1023
    payload[2] = static_cast<char>(0x00);
    payload[3] = static_cast<char>(0x00);

    QByteArray packet = Protocol::buildPacket(Protocol::MSG_ANGLE_COMMAND, 5, payload);

    Protocol::ParsedPacket parsed;
    QByteArray buffer = packet;
    Protocol::ParseStatus status = Protocol::extractNextPacket(buffer, parsed);

    QCOMPARE(status, Protocol::ParseStatus::InvalidPayloadValue);
}

void TestPacketProtocol::testSyncRecoveryAfterNoise()
{
    QByteArray noise = "GARBAGE_NOISE_12345";
    QByteArray validPkt = Protocol::buildAnglePacket(Protocol::MSG_ANGLE_COMMAND, 99, 500, 600);

    QByteArray stream = noise + validPkt;

    Protocol::ParsedPacket parsed;
    Protocol::ParseStatus status = Protocol::extractNextPacket(stream, parsed);

    QCOMPARE(status, Protocol::ParseStatus::ValidPacket);
    QCOMPARE(parsed.sequenceNumber, static_cast<uint16_t>(99));
    QCOMPARE(parsed.rawAzimuth, static_cast<uint16_t>(500));
}

void TestPacketProtocol::testAngleConversions()
{
    // Azimuth: 0 to 360 deg, raw 0 to 1023
    QCOMPARE(Protocol::rawToAzimuthDegrees(0), 0.0);
    QCOMPARE(Protocol::rawToAzimuthDegrees(1023), 360.0);
    QVERIFY(std::abs(Protocol::rawToAzimuthDegrees(511) - 179.82) < 0.1);

    // Elevation: -90 to +90 deg, raw 0 to 1023
    QCOMPARE(Protocol::rawToElevationDegrees(0), -90.0);
    QCOMPARE(Protocol::rawToElevationDegrees(1023), 90.0);
    QVERIFY(std::abs(Protocol::rawToElevationDegrees(511) - (-0.088)) < 0.1);
    QVERIFY(std::abs(Protocol::rawToElevationDegrees(512) - (0.088)) < 0.1);
}

QTEST_MAIN(TestPacketProtocol)
#include "tst_protocol.moc"
