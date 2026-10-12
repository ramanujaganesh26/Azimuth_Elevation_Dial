#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "ITransport.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QList>

class TcpTransport : public ITransport
{
    Q_OBJECT
public:
    enum class Mode {
        Client,
        Server
    };
    Q_ENUM(Mode)

    explicit TcpTransport(QObject *parent = nullptr);
    ~TcpTransport() override;

    QString transportName() const override { return QStringLiteral("TCP"); }
    bool isConnected() const override;
    QString connectionDetails() const override;

    void openConnection() override;
    void closeConnection() override;

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);

    // Client settings
    QString remoteHost() const { return m_remoteHost; }
    void setRemoteHost(const QString &host);

    quint16 remotePort() const { return m_remotePort; }
    void setRemotePort(quint16 port);

    // Server settings
    QString localBindAddress() const { return m_localBindAddress.toString(); }
    void setLocalBindAddress(const QString &address);

    quint16 localListeningPort() const { return m_localListeningPort; }
    void setLocalListeningPort(quint16 port);

private slots:
    // Client slots
    void handleClientConnected();
    void handleClientDisconnected();
    void handleClientReadyRead();
    void handleClientError(QAbstractSocket::SocketError error);

    // Server slots
    void handleServerNewConnection();
    void handleServerClientReadyRead();
    void handleServerClientDisconnected();

private:
    void closeClientMode();
    void closeServerMode();

    Mode m_mode;

    // Client mode members
    QTcpSocket *m_clientSocket;
    QString m_remoteHost;
    quint16 m_remotePort;

    // Server mode members
    QTcpServer *m_server;
    QHostAddress m_localBindAddress;
    quint16 m_localListeningPort;
    QList<QTcpSocket*> m_connectedClients;
};

#endif // TCPTRANSPORT_H
