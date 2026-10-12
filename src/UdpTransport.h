#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include "ITransport.h"
#include <QUdpSocket>
#include <QHostAddress>

class UdpTransport : public ITransport
{
    Q_OBJECT
public:
    explicit UdpTransport(QObject *parent = nullptr);
    ~UdpTransport() override;

    QString transportName() const override { return QStringLiteral("UDP"); }
    bool isConnected() const override;
    QString connectionDetails() const override;

    void openConnection() override;
    void closeConnection() override;

    QString bindAddress() const { return m_bindAddress.toString(); }
    void setBindAddress(const QString &address);

    quint16 bindPort() const { return m_bindPort; }
    void setBindPort(quint16 port);

private slots:
    void handlePendingDatagrams();
    void handleSocketError(QAbstractSocket::SocketError error);

private:
    QUdpSocket *m_socket;
    QHostAddress m_bindAddress;
    quint16 m_bindPort;
    bool m_isBound;
};

#endif // UDPTRANSPORT_H
