#include "ReceiverController.h"
#include <QDebug>
#include <cmath>

ReceiverController::ReceiverController(QObject *parent)
    : QObject(parent)
    , m_serialTransport(new SerialTransport(this))
    , m_udpTransport(new UdpTransport(this))
    , m_tcpTransport(new TcpTransport(this))
    , m_selectedTransport(TransportSerial)
    , m_connectionStatus(QStringLiteral("Disconnected"))
    , m_azimuthDegrees(0.0)
    , m_elevationDegrees(0.0)
    , m_azimuthRaw(0)
    , m_elevationRaw(511) // Midpoint ~0.0 degrees
    , m_telemetryFresh(false)
    , m_stalenessTimeoutMs(2000)
    , m_stalenessTimer(new QTimer(this))
    , m_validPacketCount(0)
    , m_crcErrorCount(0)
    , m_malformedPacketCount(0)
    , m_unknownTypeCount(0)
    , m_lastPacketHex(QStringLiteral("--"))
    , m_lastReceivedBytesHex(QStringLiteral("--"))
    , m_lastPacketTimestamp(QStringLiteral("--"))
    , m_lastPacketMessageType(QStringLiteral("NONE"))
    , m_lastPacketSequence(0)
    , m_lastPacketPayloadLen(0)
    , m_lastPacketCrcReceived(QStringLiteral("--"))
    , m_lastPacketCrcCalculated(QStringLiteral("--"))
    , m_lastPacketCrcValid(false)
    , m_lastPacketSource(QStringLiteral("--"))
    , m_logModel(new LogModel(this))
    , m_simulationActive(false)
    , m_simulationTimer(new QTimer(this))
    , m_simSequence(1)
    , m_simAzimuth(0.0)
    , m_simElevation(0.0)
    , m_simElStep(1.5)
{
    // Staleness timer
    m_stalenessTimer->setSingleShot(true);
    m_stalenessTimer->setInterval(m_stalenessTimeoutMs);
    connect(m_stalenessTimer, &QTimer::timeout, this, &ReceiverController::handleStalenessTimeout);

    // Simulation timer
    m_simulationTimer->setInterval(100); // 10 Hz
    connect(m_simulationTimer, &QTimer::timeout, this, &ReceiverController::generateSimulatedPacket);

    // Wire transports
    auto wireTransport = [this](ITransport *transport) {
        connect(transport, &ITransport::dataReceived, this, &ReceiverController::handleIncomingBytes);
        connect(transport, &ITransport::connectionStateChanged, this, &ReceiverController::handleTransportStateChanged);
        connect(transport, &ITransport::transportError, this, &ReceiverController::handleTransportError);
        connect(transport, &ITransport::logEvent, this, &ReceiverController::handleTransportLog);
    };

    wireTransport(m_serialTransport);
    wireTransport(m_udpTransport);
    wireTransport(m_tcpTransport);
}

ReceiverController::~ReceiverController()
{
    if (m_simulationTimer->isActive()) {
        m_simulationTimer->stop();
    }
    m_serialTransport->closeConnection();
    m_udpTransport->closeConnection();
    m_tcpTransport->closeConnection();
}

bool ReceiverController::isConnected() const
{
    if (m_simulationActive) return true;
    ITransport *t = currentTransportPtr();
    return t ? t->isConnected() : false;
}

QString ReceiverController::currentTransportName() const
{
    if (m_simulationActive) return QStringLiteral("SIMULATION");
    switch (m_selectedTransport) {
    case TransportSerial: return QStringLiteral("Serial");
    case TransportUdp: return QStringLiteral("UDP");
    case TransportTcpClient: return QStringLiteral("TCP Client");
    case TransportTcpServer: return QStringLiteral("TCP Server");
    default: return QStringLiteral("None");
    }
}

ITransport* ReceiverController::currentTransportPtr() const
{
    switch (m_selectedTransport) {
    case TransportSerial: return m_serialTransport;
    case TransportUdp: return m_udpTransport;
    case TransportTcpClient:
    case TransportTcpServer: return m_tcpTransport;
    default: return nullptr;
    }
}

void ReceiverController::setSelectedTransport(int transport)
{
    if (m_selectedTransport == transport) return;

    // Disconnect active transport before switching
    disconnectCurrentTransport();

    m_selectedTransport = transport;
    if (m_selectedTransport == TransportTcpClient) {
        m_tcpTransport->setMode(TcpTransport::Mode::Client);
    } else if (m_selectedTransport == TransportTcpServer) {
        m_tcpTransport->setMode(TcpTransport::Mode::Server);
    }

    m_connectionStatus = QStringLiteral("Ready (%1)").arg(currentTransportName());
    emit selectedTransportChanged();
    emit connectionStatusChanged();
}

void ReceiverController::setStalenessTimeoutMs(int ms)
{
    if (m_stalenessTimeoutMs == ms) return;
    m_stalenessTimeoutMs = ms;
    m_stalenessTimer->setInterval(m_stalenessTimeoutMs);
    emit stalenessTimeoutChanged();
}

QStringList ReceiverController::availableSerialPorts() const
{
    return SerialTransport::availablePorts();
}

void ReceiverController::refreshSerialPorts()
{
    emit serialPortsChanged();
}

void ReceiverController::connectCurrentTransport()
{
    if (m_simulationActive) {
        m_connectionStatus = QStringLiteral("Simulation active (internal source)");
        emit connectionStateChanged();
        emit connectionStatusChanged();
        return;
    }

    ITransport *t = currentTransportPtr();
    if (t) {
        m_rxBuffer.clear();
        t->openConnection();
    }
}

void ReceiverController::disconnectCurrentTransport()
{
    ITransport *t = currentTransportPtr();
    if (t && t->isConnected()) {
        t->closeConnection();
    }
    m_rxBuffer.clear();
    m_telemetryFresh = false;
    emit telemetryFreshChanged();
    emit connectionStateChanged();
}

void ReceiverController::resetCounters()
{
    m_validPacketCount = 0;
    m_crcErrorCount = 0;
    m_malformedPacketCount = 0;
    m_unknownTypeCount = 0;
    emit statsChanged();
}

void ReceiverController::setSerialPort(const QString &port)
{
    m_serialTransport->setPortName(port);
}

void ReceiverController::setSerialBaudRate(int baud)
{
    m_serialTransport->setBaudRate(baud);
}

void ReceiverController::setSerialDataBits(int bits)
{
    m_serialTransport->setDataBits(static_cast<QSerialPort::DataBits>(bits));
}

void ReceiverController::setSerialParity(int parity)
{
    m_serialTransport->setParity(static_cast<QSerialPort::Parity>(parity));
}

void ReceiverController::setSerialStopBits(int stopBits)
{
    m_serialTransport->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
}

void ReceiverController::setSerialFlowControl(int flow)
{
    m_serialTransport->setFlowControl(static_cast<QSerialPort::FlowControl>(flow));
}

void ReceiverController::setUdpBindAddress(const QString &address)
{
    m_udpTransport->setBindAddress(address);
}

void ReceiverController::setUdpBindPort(int port)
{
    m_udpTransport->setBindPort(static_cast<quint16>(port));
}

void ReceiverController::setTcpRemoteHost(const QString &host)
{
    m_tcpTransport->setRemoteHost(host);
}

void ReceiverController::setTcpRemotePort(int port)
{
    m_tcpTransport->setRemotePort(static_cast<quint16>(port));
}

void ReceiverController::setTcpLocalBindAddress(const QString &address)
{
    m_tcpTransport->setLocalBindAddress(address);
}

void ReceiverController::setTcpLocalListeningPort(int port)
{
    m_tcpTransport->setLocalListeningPort(static_cast<quint16>(port));
}

void ReceiverController::setSimulationActive(bool active)
{
    if (m_simulationActive == active) return;
    m_simulationActive = active;

    if (m_simulationActive) {
        m_simulationTimer->start();
        m_connectionStatus = QStringLiteral("Simulation generator running (10 Hz)");
        m_logModel->appendEntry(QStringLiteral("SIMULATION"),
                                QStringLiteral("Internal"),
                                QStringLiteral("SIM_START"),
                                QStringLiteral("--"),
                                QStringLiteral("Loopback packet generator started"),
                                QStringLiteral("ACTIVE"),
                                false,
                                QString());
    } else {
        m_simulationTimer->stop();
        m_connectionStatus = currentTransportPtr() && currentTransportPtr()->isConnected()
                                 ? currentTransportPtr()->connectionDetails()
                                 : QStringLiteral("Disconnected");
        m_logModel->appendEntry(QStringLiteral("SIMULATION"),
                                QStringLiteral("Internal"),
                                QStringLiteral("SIM_STOP"),
                                QStringLiteral("--"),
                                QStringLiteral("Loopback packet generator stopped"),
                                QStringLiteral("STOPPED"),
                                false,
                                QString());
    }

    emit simulationActiveChanged();
    emit connectionStateChanged();
    emit connectionStatusChanged();
}

void ReceiverController::handleIncomingBytes(const QByteArray &data, const QString &sourceInfo)
{
    if (data.isEmpty()) return;

    m_lastReceivedBytesHex = data.toHex(' ').toUpper();
    emit lastBytesChanged();

    // Log raw incoming byte batch
    m_logModel->appendEntry(currentTransportName(),
                            sourceInfo,
                            QStringLiteral("RX_BYTES"),
                            QStringLiteral("--"),
                            QStringLiteral("Received %1 bytes").arg(data.size()),
                            QStringLiteral("OK"),
                            false,
                            m_lastReceivedBytesHex);

    m_rxBuffer.append(data);
    processRxBuffer(sourceInfo);
}

void ReceiverController::processRxBuffer(const QString &sourceInfo)
{
    Protocol::ParsedPacket parsed;
    Protocol::ParseStatus status;

    while ((status = Protocol::extractNextPacket(m_rxBuffer, parsed)) != Protocol::ParseStatus::NeedMoreData) {
        m_lastPacketHex = parsed.rawBytesHex;
        m_lastPacketTimestamp = parsed.timestamp.toString(QStringLiteral("hh:mm:ss.zzz"));
        m_lastPacketMessageType = parsed.messageTypeName;
        m_lastPacketSequence = parsed.sequenceNumber;
        m_lastPacketPayloadLen = parsed.payloadLength;
        m_lastPacketCrcReceived = QStringLiteral("0x%1").arg(parsed.receivedCrc, 4, 16, QLatin1Char('0')).toUpper();
        m_lastPacketCrcCalculated = QStringLiteral("0x%1").arg(parsed.calculatedCrc, 4, 16, QLatin1Char('0')).toUpper();
        m_lastPacketCrcValid = parsed.crcValid;
        m_lastPacketSource = sourceInfo;
        emit lastPacketChanged();

        if (status == Protocol::ParseStatus::ValidPacket) {
            m_validPacketCount++;

            if (parsed.isAnglePacket) {
                // Update live angles
                m_azimuthDegrees = parsed.azimuthDegrees;
                m_elevationDegrees = parsed.elevationDegrees;
                m_azimuthRaw = parsed.rawAzimuth;
                m_elevationRaw = parsed.rawElevation;
                emit anglesChanged();

                // Telemetry is live & restart staleness timer
                if (!m_telemetryFresh) {
                    m_telemetryFresh = true;
                    emit telemetryFreshChanged();
                }
                m_stalenessTimer->start();

                // Log successfully decoded angle update
                m_logModel->appendEntry(currentTransportName(),
                                        sourceInfo,
                                        QStringLiteral("ANGLE_UPDATE"),
                                        parsed.messageTypeName,
                                        QStringLiteral("Az: %1 deg (raw %2), El: %3 deg (raw %4)")
                                            .arg(m_azimuthDegrees, 0, 'f', 2)
                                            .arg(m_azimuthRaw)
                                            .arg(m_elevationDegrees, 0, 'f', 2)
                                            .arg(m_elevationRaw),
                                        QStringLiteral("VALID"),
                                        false,
                                        parsed.rawBytesHex);
            } else if (parsed.messageType == Protocol::MSG_ACK || parsed.messageType == Protocol::MSG_PING) {
                // Receive-only: recognized for analysis only, never respond
                m_logModel->appendEntry(currentTransportName(),
                                        sourceInfo,
                                        QStringLiteral("DIAG_PACKET"),
                                        parsed.messageTypeName,
                                        QStringLiteral("Seq: %1, Len: %2").arg(parsed.sequenceNumber).arg(parsed.payloadLength),
                                        QStringLiteral("VALID"),
                                        false,
                                        parsed.rawBytesHex);
            } else {
                m_unknownTypeCount++;
                m_logModel->appendEntry(currentTransportName(),
                                        sourceInfo,
                                        QStringLiteral("UNKNOWN_TYPE"),
                                        parsed.messageTypeName,
                                        QStringLiteral("Unrecognized msg type 0x%1").arg(parsed.messageType, 2, 16, QLatin1Char('0')),
                                        QStringLiteral("IGNORED"),
                                        false,
                                        parsed.rawBytesHex);
            }
        } else if (status == Protocol::ParseStatus::CrcError) {
            m_crcErrorCount++;
            m_logModel->appendEntry(currentTransportName(),
                                    sourceInfo,
                                    QStringLiteral("CRC_ERROR"),
                                    parsed.messageTypeName,
                                    parsed.errorMessage,
                                    QStringLiteral("CRC_FAIL"),
                                    true,
                                    parsed.rawBytesHex);
        } else if (status == Protocol::ParseStatus::MalformedPacket || status == Protocol::ParseStatus::InvalidPayloadValue) {
            m_malformedPacketCount++;
            m_logModel->appendEntry(currentTransportName(),
                                    sourceInfo,
                                    QStringLiteral("MALFORMED"),
                                    parsed.messageTypeName,
                                    parsed.errorMessage,
                                    QStringLiteral("REJECTED"),
                                    true,
                                    parsed.rawBytesHex);
        }

        emit statsChanged();
    }
}

void ReceiverController::handleTransportStateChanged(bool connected, const QString &statusMessage)
{
    m_connectionStatus = statusMessage;
    if (!connected) {
        m_telemetryFresh = false;
        emit telemetryFreshChanged();
    }
    emit connectionStateChanged();
    emit connectionStatusChanged();
}

void ReceiverController::handleTransportError(const QString &errorMessage)
{
    m_connectionStatus = QStringLiteral("Error: %1").arg(errorMessage);
    emit connectionStatusChanged();
}

void ReceiverController::handleTransportLog(const QString &event, const QString &details, bool isError)
{
    m_logModel->appendEntry(currentTransportName(),
                            currentTransportPtr() ? currentTransportPtr()->connectionDetails() : QStringLiteral("--"),
                            event,
                            QStringLiteral("--"),
                            details,
                            isError ? QStringLiteral("ERROR") : QStringLiteral("INFO"),
                            isError,
                            QString());
}

void ReceiverController::handleStalenessTimeout()
{
    if (m_telemetryFresh) {
        m_telemetryFresh = false;
        emit telemetryFreshChanged();

        m_logModel->appendEntry(currentTransportName(),
                                QStringLiteral("--"),
                                QStringLiteral("TIMEOUT"),
                                QStringLiteral("--"),
                                QStringLiteral("No valid packet received for %1 ms. Telemetry marked STALE.").arg(m_stalenessTimeoutMs),
                                QStringLiteral("STALE"),
                                true,
                                QString());
    }
}

void ReceiverController::generateSimulatedPacket()
{
    if (!m_simulationActive) return;

    // Advance simulated azimuth (sweeps 0 to 360, wraps smoothly)
    m_simAzimuth += 2.4;
    if (m_simAzimuth >= 360.0) {
        m_simAzimuth -= 360.0;
    }

    // Oscillate elevation between -10 and +85
    m_simElevation += m_simElStep;
    if (m_simElevation >= 85.0) {
        m_simElevation = 85.0;
        m_simElStep = -m_simElStep;
    } else if (m_simElevation <= -10.0) {
        m_simElevation = -10.0;
        m_simElStep = -m_simElStep;
    }

    uint16_t azRaw = Protocol::azimuthDegreesToRaw(m_simAzimuth);
    uint16_t elRaw = Protocol::elevationDegreesToRaw(m_simElevation);

    // Alternate between MSG_ANGLE_COMMAND (0x01) and MSG_ANGLE_TELEMETRY (0x02)
    uint8_t type = (m_simSequence % 2 == 0) ? Protocol::MSG_ANGLE_COMMAND : Protocol::MSG_ANGLE_TELEMETRY;

    QByteArray packet = Protocol::buildAnglePacket(type, m_simSequence++, azRaw, elRaw);

    // Feed generated packet into receiver stream parser
    handleIncomingBytes(packet, QStringLiteral("SIM_SRC_INTERNAL"));
}
