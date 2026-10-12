#include "UdpTransport.h"
#include <QNetworkDatagram>

UdpTransport::UdpTransport(QObject *parent)
    : ITransport(parent)
    , m_socket(new QUdpSocket(this))
    , m_bindAddress(QHostAddress::AnyIPv4)
    , m_bindPort(14550)
    , m_isBound(false)
{
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpTransport::handlePendingDatagrams);
    connect(m_socket, &QUdpSocket::errorOccurred, this, &UdpTransport::handleSocketError);
}

UdpTransport::~UdpTransport()
{
    closeConnection();
}

bool UdpTransport::isConnected() const
{
    return m_isBound && m_socket->state() == QAbstractSocket::BoundState;
}

QString UdpTransport::connectionDetails() const
{
    if (!isConnected()) {
        return QStringLiteral("Unbound");
    }
    return QStringLiteral("UDP Bound to %1:%2").arg(m_bindAddress.toString()).arg(m_bindPort);
}

void UdpTransport::setBindAddress(const QString &address)
{
    QString trimmed = address.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("0.0.0.0")) {
        m_bindAddress = QHostAddress::AnyIPv4;
    } else {
        m_bindAddress.setAddress(trimmed);
    }
}

void UdpTransport::setBindPort(quint16 port)
{
    m_bindPort = port;
}

void UdpTransport::openConnection()
{
    if (isConnected()) {
        closeConnection();
    }

    m_isBound = m_socket->bind(m_bindAddress, m_bindPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (m_isBound) {
        emit connectionStateChanged(true, QStringLiteral("Listening on UDP %1:%2").arg(m_bindAddress.toString()).arg(m_bindPort));
        emit logEvent(QStringLiteral("UDP_BIND"), QStringLiteral("Bound socket to %1:%2").arg(m_bindAddress.toString()).arg(m_bindPort), false);
    } else {
        QString err = m_socket->errorString();
        emit transportError(QStringLiteral("UDP Bind Failed: %1").arg(err));
        emit connectionStateChanged(false, QStringLiteral("Bind Failed: %1").arg(err));
        emit logEvent(QStringLiteral("UDP_ERROR"), QStringLiteral("Failed to bind UDP %1:%2 - %3").arg(m_bindAddress.toString()).arg(m_bindPort).arg(err), true);
    }
}

void UdpTransport::closeConnection()
{
    if (m_isBound || m_socket->isOpen()) {
        m_socket->close();
        m_isBound = false;
        emit connectionStateChanged(false, QStringLiteral("Socket closed"));
        emit logEvent(QStringLiteral("UDP_CLOSE"), QStringLiteral("Closed UDP socket on port %1").arg(m_bindPort), false);
    }
}

void UdpTransport::handlePendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        if (!data.isEmpty()) {
            QString source = QStringLiteral("%1:%2").arg(datagram.senderAddress().toString()).arg(datagram.senderPort());
            emit dataReceived(data, source);
        }
    }
}

void UdpTransport::handleSocketError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) return;
    QString err = m_socket->errorString();
    emit transportError(err);
    emit logEvent(QStringLiteral("UDP_ERROR"), err, true);
}
