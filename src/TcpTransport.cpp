#include "TcpTransport.h"

TcpTransport::TcpTransport(QObject *parent)
    : ITransport(parent)
    , m_mode(Mode::Client)
    , m_clientSocket(new QTcpSocket(this))
    , m_remoteHost(QStringLiteral("127.0.0.1"))
    , m_remotePort(5000)
    , m_server(new QTcpServer(this))
    , m_localBindAddress(QHostAddress::AnyIPv4)
    , m_localListeningPort(5000)
{
    // Client socket connections
    connect(m_clientSocket, &QTcpSocket::connected, this, &TcpTransport::handleClientConnected);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &TcpTransport::handleClientDisconnected);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &TcpTransport::handleClientReadyRead);
    connect(m_clientSocket, &QTcpSocket::errorOccurred, this, &TcpTransport::handleClientError);

    // Server connections
    connect(m_server, &QTcpServer::newConnection, this, &TcpTransport::handleServerNewConnection);
}

TcpTransport::~TcpTransport()
{
    closeConnection();
}

bool TcpTransport::isConnected() const
{
    if (m_mode == Mode::Client) {
        return m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState;
    } else {
        return m_server && m_server->isListening();
    }
}

QString TcpTransport::connectionDetails() const
{
    if (m_mode == Mode::Client) {
        if (isConnected()) {
            return QStringLiteral("TCP Client connected to %1:%2").arg(m_remoteHost).arg(m_remotePort);
        }
        return QStringLiteral("TCP Client disconnected");
    } else {
        if (m_server && m_server->isListening()) {
            return QStringLiteral("TCP Server listening on %1:%2 (%3 client%4)")
                .arg(m_localBindAddress.toString())
                .arg(m_localListeningPort)
                .arg(m_connectedClients.size())
                .arg(m_connectedClients.size() == 1 ? "" : "s");
        }
        return QStringLiteral("TCP Server stopped");
    }
}

void TcpTransport::setMode(Mode mode)
{
    if (m_mode != mode) {
        if (isConnected()) {
            closeConnection();
        }
        m_mode = mode;
    }
}

void TcpTransport::setRemoteHost(const QString &host)
{
    m_remoteHost = host.trimmed();
}

void TcpTransport::setRemotePort(quint16 port)
{
    m_remotePort = port;
}

void TcpTransport::setLocalBindAddress(const QString &address)
{
    QString trimmed = address.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("0.0.0.0")) {
        m_localBindAddress = QHostAddress::AnyIPv4;
    } else {
        m_localBindAddress.setAddress(trimmed);
    }
}

void TcpTransport::setLocalListeningPort(quint16 port)
{
    m_localListeningPort = port;
}

void TcpTransport::openConnection()
{
    closeConnection();

    if (m_mode == Mode::Client) {
        emit connectionStateChanged(false, QStringLiteral("Connecting to %1:%2...").arg(m_remoteHost).arg(m_remotePort));
        emit logEvent(QStringLiteral("TCP_CONNECTING"), QStringLiteral("Initiating connection to %1:%2").arg(m_remoteHost).arg(m_remotePort), false);
        m_clientSocket->connectToHost(m_remoteHost, m_remotePort, QIODevice::ReadOnly);
    } else {
        bool ok = m_server->listen(m_localBindAddress, m_localListeningPort);
        if (ok) {
            emit connectionStateChanged(true, QStringLiteral("Server listening on %1:%2").arg(m_localBindAddress.toString()).arg(m_localListeningPort));
            emit logEvent(QStringLiteral("TCP_SERVER_START"), QStringLiteral("Listening on %1:%2").arg(m_localBindAddress.toString()).arg(m_localListeningPort), false);
        } else {
            QString err = m_server->errorString();
            emit transportError(QStringLiteral("Server listen failed: %1").arg(err));
            emit connectionStateChanged(false, QStringLiteral("Listen Failed: %1").arg(err));
            emit logEvent(QStringLiteral("TCP_SERVER_ERROR"), err, true);
        }
    }
}

void TcpTransport::closeConnection()
{
    if (m_mode == Mode::Client) {
        closeClientMode();
    } else {
        closeServerMode();
    }
}

void TcpTransport::closeClientMode()
{
    if (m_clientSocket && (m_clientSocket->state() != QAbstractSocket::UnconnectedState)) {
        m_clientSocket->disconnectFromHost();
        if (m_clientSocket->state() != QAbstractSocket::UnconnectedState) {
            m_clientSocket->abort();
        }
    }
    emit connectionStateChanged(false, QStringLiteral("Disconnected"));
}

void TcpTransport::closeServerMode()
{
    for (QTcpSocket *client : std::as_const(m_connectedClients)) {
        if (client) {
            client->disconnect(this);
            client->disconnectFromHost();
            client->deleteLater();
        }
    }
    m_connectedClients.clear();

    if (m_server && m_server->isListening()) {
        m_server->close();
        emit connectionStateChanged(false, QStringLiteral("Server stopped"));
        emit logEvent(QStringLiteral("TCP_SERVER_STOP"), QStringLiteral("Stopped listening on port %1").arg(m_localListeningPort), false);
    }
}

void TcpTransport::handleClientConnected()
{
    emit connectionStateChanged(true, QStringLiteral("Connected to %1:%2").arg(m_remoteHost).arg(m_remotePort));
    emit logEvent(QStringLiteral("TCP_CONNECTED"), QStringLiteral("Connected to %1:%2").arg(m_remoteHost).arg(m_remotePort), false);
}

void TcpTransport::handleClientDisconnected()
{
    emit connectionStateChanged(false, QStringLiteral("Disconnected from server"));
    emit logEvent(QStringLiteral("TCP_DISCONNECTED"), QStringLiteral("Disconnected from %1:%2").arg(m_remoteHost).arg(m_remotePort), false);
}

void TcpTransport::handleClientReadyRead()
{
    if (!m_clientSocket) return;
    QByteArray data = m_clientSocket->readAll();
    if (!data.isEmpty()) {
        QString source = QStringLiteral("TCP %1:%2").arg(m_clientSocket->peerAddress().toString()).arg(m_clientSocket->peerPort());
        emit dataReceived(data, source);
    }
}

void TcpTransport::handleClientError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString err = m_clientSocket->errorString();
    emit transportError(err);
    emit logEvent(QStringLiteral("TCP_ERROR"), err, true);
}

void TcpTransport::handleServerNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_connectedClients.append(client);

        connect(client, &QTcpSocket::readyRead, this, &TcpTransport::handleServerClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &TcpTransport::handleServerClientDisconnected);

        QString clientEp = QStringLiteral("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        emit logEvent(QStringLiteral("TCP_CLIENT_JOIN"), QStringLiteral("Client connected from %1").arg(clientEp), false);
        emit connectionStateChanged(true, QStringLiteral("Server active: %1 client%2").arg(m_connectedClients.size()).arg(m_connectedClients.size() == 1 ? "" : "s"));
    }
}

void TcpTransport::handleServerClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    if (!data.isEmpty()) {
        QString source = QStringLiteral("TCP Client %1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        emit dataReceived(data, source);
    }
}

void TcpTransport::handleServerClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QString clientEp = QStringLiteral("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
    emit logEvent(QStringLiteral("TCP_CLIENT_LEFT"), QStringLiteral("Client disconnected from %1").arg(clientEp), false);

    m_connectedClients.removeAll(client);
    client->deleteLater();

    if (m_server && m_server->isListening()) {
        emit connectionStateChanged(true, QStringLiteral("Server active: %1 client%2").arg(m_connectedClients.size()).arg(m_connectedClients.size() == 1 ? "" : "s"));
    }
}
