#include "SerialTransport.h"

SerialTransport::SerialTransport(QObject *parent)
    : ITransport(parent)
    , m_serialPort(new QSerialPort(this))
    , m_portName(QString())
    , m_baudRate(115200)
    , m_dataBits(QSerialPort::Data8)
    , m_parity(QSerialPort::NoParity)
    , m_stopBits(QSerialPort::OneStop)
    , m_flowControl(QSerialPort::NoFlowControl)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialTransport::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialTransport::handleError);
}

SerialTransport::~SerialTransport()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

bool SerialTransport::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen();
}

QString SerialTransport::connectionDetails() const
{
    if (!isConnected()) {
        return QStringLiteral("Disconnected");
    }
    return QStringLiteral("%1 @ %2 baud (8N1)").arg(m_portName).arg(m_baudRate);
}

void SerialTransport::setPortName(const QString &port)
{
    m_portName = port;
}

void SerialTransport::setBaudRate(qint32 baud)
{
    m_baudRate = baud;
    if (isConnected()) {
        m_serialPort->setBaudRate(baud);
    }
}

void SerialTransport::setDataBits(QSerialPort::DataBits bits)
{
    m_dataBits = bits;
    if (isConnected()) {
        m_serialPort->setDataBits(bits);
    }
}

void SerialTransport::setParity(QSerialPort::Parity parity)
{
    m_parity = parity;
    if (isConnected()) {
        m_serialPort->setParity(parity);
    }
}

void SerialTransport::setStopBits(QSerialPort::StopBits stopBits)
{
    m_stopBits = stopBits;
    if (isConnected()) {
        m_serialPort->setStopBits(stopBits);
    }
}

void SerialTransport::setFlowControl(QSerialPort::FlowControl flow)
{
    m_flowControl = flow;
    if (isConnected()) {
        m_serialPort->setFlowControl(flow);
    }
}

QStringList SerialTransport::availablePorts()
{
    QStringList list;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &p : ports) {
        QString desc = p.portName();
        if (!p.description().isEmpty()) {
            desc += QStringLiteral(" (%1)").arg(p.description());
        }
        list.append(desc);
    }
    return list;
}

void SerialTransport::openConnection()
{
    if (isConnected()) {
        closeConnection();
    }

    if (m_portName.isEmpty()) {
        emit transportError(QStringLiteral("No serial port specified"));
        emit connectionStateChanged(false, QStringLiteral("Error: No port selected"));
        return;
    }

    // Strip description if included in UI (e.g. "COM3 (USB Serial)" -> "COM3")
    QString actualPort = m_portName;
    int spaceIndex = actualPort.indexOf(' ');
    if (spaceIndex != -1) {
        actualPort = actualPort.left(spaceIndex);
    }

    m_serialPort->setPortName(actualPort);
    m_serialPort->setBaudRate(m_baudRate);
    m_serialPort->setDataBits(m_dataBits);
    m_serialPort->setParity(m_parity);
    m_serialPort->setStopBits(m_stopBits);
    m_serialPort->setFlowControl(m_flowControl);

    // Open strictly in ReadOnly mode
    if (m_serialPort->open(QIODevice::ReadOnly)) {
        emit connectionStateChanged(true, QStringLiteral("Connected to %1 (%2 bps)").arg(actualPort).arg(m_baudRate));
        emit logEvent(QStringLiteral("SERIAL_OPEN"), QStringLiteral("Port %1 opened at %2 baud").arg(actualPort).arg(m_baudRate), false);
    } else {
        QString err = m_serialPort->errorString();
        emit transportError(QStringLiteral("Failed to open %1: %2").arg(actualPort, err));
        emit connectionStateChanged(false, QStringLiteral("Open Failed: %1").arg(err));
        emit logEvent(QStringLiteral("SERIAL_ERROR"), QStringLiteral("Failed to open %1: %2").arg(actualPort, err), true);
    }
}

void SerialTransport::closeConnection()
{
    if (m_serialPort->isOpen()) {
        QString port = m_serialPort->portName();
        m_serialPort->close();
        emit connectionStateChanged(false, QStringLiteral("Disconnected"));
        emit logEvent(QStringLiteral("SERIAL_CLOSE"), QStringLiteral("Port %1 closed").arg(port), false);
    }
}

void SerialTransport::handleReadyRead()
{
    if (!m_serialPort) return;
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data, m_serialPort->portName());
    }
}

void SerialTransport::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        QString errStr = m_serialPort->errorString();
        emit transportError(errStr);
        emit connectionStateChanged(false, QStringLiteral("Port error: %1").arg(errStr));
        emit logEvent(QStringLiteral("SERIAL_ERROR"), errStr, true);
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
    }
}
