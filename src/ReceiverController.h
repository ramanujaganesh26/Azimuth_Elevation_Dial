#ifndef RECEIVERCONTROLLER_H
#define RECEIVERCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include "PacketProtocol.h"
#include "SerialTransport.h"
#include "UdpTransport.h"
#include "TcpTransport.h"
#include "LogModel.h"

class ReceiverController : public QObject
{
    Q_OBJECT

    // Telemetry Properties
    Q_PROPERTY(double azimuthDegrees READ azimuthDegrees NOTIFY anglesChanged)
    Q_PROPERTY(double elevationDegrees READ elevationDegrees NOTIFY anglesChanged)
    Q_PROPERTY(int azimuthRaw READ azimuthRaw NOTIFY anglesChanged)
    Q_PROPERTY(int elevationRaw READ elevationRaw NOTIFY anglesChanged)
    Q_PROPERTY(bool telemetryFresh READ isTelemetryFresh NOTIFY telemetryFreshChanged)
    Q_PROPERTY(int stalenessTimeoutMs READ stalenessTimeoutMs WRITE setStalenessTimeoutMs NOTIFY stalenessTimeoutChanged)

    // Connection & Transport Properties
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(int selectedTransport READ selectedTransport WRITE setSelectedTransport NOTIFY selectedTransportChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString currentTransportName READ currentTransportName NOTIFY selectedTransportChanged)

    // Packet Statistics
    Q_PROPERTY(int validPacketCount READ validPacketCount NOTIFY statsChanged)
    Q_PROPERTY(int crcErrorCount READ crcErrorCount NOTIFY statsChanged)
    Q_PROPERTY(int malformedPacketCount READ malformedPacketCount NOTIFY statsChanged)
    Q_PROPERTY(int unknownTypeCount READ unknownTypeCount NOTIFY statsChanged)
    Q_PROPERTY(int totalPacketsCount READ totalPacketsCount NOTIFY statsChanged)

    // Packet Analyzer Inspection Properties
    Q_PROPERTY(QString lastPacketHex READ lastPacketHex NOTIFY lastPacketChanged)
    Q_PROPERTY(QString lastReceivedBytesHex READ lastReceivedBytesHex NOTIFY lastBytesChanged)
    Q_PROPERTY(QString lastPacketTimestamp READ lastPacketTimestamp NOTIFY lastPacketChanged)
    Q_PROPERTY(QString lastPacketMessageType READ lastPacketMessageType NOTIFY lastPacketChanged)
    Q_PROPERTY(int lastPacketSequence READ lastPacketSequence NOTIFY lastPacketChanged)
    Q_PROPERTY(int lastPacketPayloadLen READ lastPacketPayloadLen NOTIFY lastPacketChanged)
    Q_PROPERTY(QString lastPacketCrcReceived READ lastPacketCrcReceived NOTIFY lastPacketChanged)
    Q_PROPERTY(QString lastPacketCrcCalculated READ lastPacketCrcCalculated NOTIFY lastPacketChanged)
    Q_PROPERTY(bool lastPacketCrcValid READ lastPacketCrcValid NOTIFY lastPacketChanged)
    Q_PROPERTY(QString lastPacketSource READ lastPacketSource NOTIFY lastPacketChanged)

    // Log & Settings Properties
    Q_PROPERTY(LogModel* logModel READ logModel CONSTANT)
    Q_PROPERTY(QStringList availableSerialPorts READ availableSerialPorts NOTIFY serialPortsChanged)
    Q_PROPERTY(bool simulationActive READ isSimulationActive WRITE setSimulationActive NOTIFY simulationActiveChanged)

public:
    enum TransportType {
        TransportSerial = 0,
        TransportUdp = 1,
        TransportTcpClient = 2,
        TransportTcpServer = 3
    };
    Q_ENUM(TransportType)

    explicit ReceiverController(QObject *parent = nullptr);
    ~ReceiverController() override;

    // Telemetry getters
    double azimuthDegrees() const { return m_azimuthDegrees; }
    double elevationDegrees() const { return m_elevationDegrees; }
    int azimuthRaw() const { return m_azimuthRaw; }
    int elevationRaw() const { return m_elevationRaw; }
    bool isTelemetryFresh() const { return m_telemetryFresh; }
    int stalenessTimeoutMs() const { return m_stalenessTimeoutMs; }
    void setStalenessTimeoutMs(int ms);

    // Transport getters
    bool isConnected() const;
    int selectedTransport() const { return m_selectedTransport; }
    void setSelectedTransport(int transport);
    QString connectionStatus() const { return m_connectionStatus; }
    QString currentTransportName() const;

    // Statistics getters
    int validPacketCount() const { return m_validPacketCount; }
    int crcErrorCount() const { return m_crcErrorCount; }
    int malformedPacketCount() const { return m_malformedPacketCount; }
    int unknownTypeCount() const { return m_unknownTypeCount; }
    int totalPacketsCount() const { return m_validPacketCount + m_crcErrorCount + m_malformedPacketCount + m_unknownTypeCount; }

    // Last Packet getters
    QString lastPacketHex() const { return m_lastPacketHex; }
    QString lastReceivedBytesHex() const { return m_lastReceivedBytesHex; }
    QString lastPacketTimestamp() const { return m_lastPacketTimestamp; }
    QString lastPacketMessageType() const { return m_lastPacketMessageType; }
    int lastPacketSequence() const { return m_lastPacketSequence; }
    int lastPacketPayloadLen() const { return m_lastPacketPayloadLen; }
    QString lastPacketCrcReceived() const { return m_lastPacketCrcReceived; }
    QString lastPacketCrcCalculated() const { return m_lastPacketCrcCalculated; }
    bool lastPacketCrcValid() const { return m_lastPacketCrcValid; }
    QString lastPacketSource() const { return m_lastPacketSource; }

    // Log model & Simulation
    LogModel* logModel() const { return m_logModel; }
    QStringList availableSerialPorts() const;
    bool isSimulationActive() const { return m_simulationActive; }
    void setSimulationActive(bool active);

    // QML Invokables
    Q_INVOKABLE void connectCurrentTransport();
    Q_INVOKABLE void disconnectCurrentTransport();
    Q_INVOKABLE void refreshSerialPorts();
    Q_INVOKABLE void resetCounters();

    // Serial settings
    Q_INVOKABLE void setSerialPort(const QString &port);
    Q_INVOKABLE void setSerialBaudRate(int baud);
    Q_INVOKABLE void setSerialDataBits(int bits);
    Q_INVOKABLE void setSerialParity(int parity);
    Q_INVOKABLE void setSerialStopBits(int stopBits);
    Q_INVOKABLE void setSerialFlowControl(int flow);

    // UDP settings
    Q_INVOKABLE void setUdpBindAddress(const QString &address);
    Q_INVOKABLE void setUdpBindPort(int port);

    // TCP settings
    Q_INVOKABLE void setTcpRemoteHost(const QString &host);
    Q_INVOKABLE void setTcpRemotePort(int port);
    Q_INVOKABLE void setTcpLocalBindAddress(const QString &address);
    Q_INVOKABLE void setTcpLocalListeningPort(int port);

signals:
    void anglesChanged();
    void telemetryFreshChanged();
    void stalenessTimeoutChanged();
    void connectionStateChanged();
    void selectedTransportChanged();
    void connectionStatusChanged();
    void statsChanged();
    void lastPacketChanged();
    void lastBytesChanged();
    void serialPortsChanged();
    void simulationActiveChanged();

private slots:
    void handleIncomingBytes(const QByteArray &data, const QString &sourceInfo);
    void handleTransportStateChanged(bool connected, const QString &statusMessage);
    void handleTransportError(const QString &errorMessage);
    void handleTransportLog(const QString &event, const QString &details, bool isError);
    void handleStalenessTimeout();
    void generateSimulatedPacket();

private:
    ITransport* currentTransportPtr() const;
    void processRxBuffer(const QString &sourceInfo);

    // Transports
    SerialTransport *m_serialTransport;
    UdpTransport *m_udpTransport;
    TcpTransport *m_tcpTransport;
    int m_selectedTransport;
    QString m_connectionStatus;

    // Buffer for stream parsing
    QByteArray m_rxBuffer;

    // Angles & Telemetry
    double m_azimuthDegrees;
    double m_elevationDegrees;
    int m_azimuthRaw;
    int m_elevationRaw;
    bool m_telemetryFresh;
    int m_stalenessTimeoutMs;
    QTimer *m_stalenessTimer;

    // Statistics
    int m_validPacketCount;
    int m_crcErrorCount;
    int m_malformedPacketCount;
    int m_unknownTypeCount;

    // Packet Analyzer details
    QString m_lastPacketHex;
    QString m_lastReceivedBytesHex;
    QString m_lastPacketTimestamp;
    QString m_lastPacketMessageType;
    int m_lastPacketSequence;
    int m_lastPacketPayloadLen;
    QString m_lastPacketCrcReceived;
    QString m_lastPacketCrcCalculated;
    bool m_lastPacketCrcValid;
    QString m_lastPacketSource;

    // Logging & Simulation
    LogModel *m_logModel;
    bool m_simulationActive;
    QTimer *m_simulationTimer;
    uint16_t m_simSequence;
    double m_simAzimuth;
    double m_simElevation;
    double m_simElStep;
};

#endif // RECEIVERCONTROLLER_H
