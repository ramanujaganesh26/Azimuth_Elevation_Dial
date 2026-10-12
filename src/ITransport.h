#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QString>

class ITransport : public QObject
{
    Q_OBJECT
public:
    explicit ITransport(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ITransport() = default;

    virtual QString transportName() const = 0;
    virtual bool isConnected() const = 0;
    virtual QString connectionDetails() const = 0;

    virtual void openConnection() = 0;
    virtual void closeConnection() = 0;

signals:
    // Strictly receive-only: receives bytes from physical/network layer
    void dataReceived(const QByteArray &data, const QString &sourceInfo);
    void connectionStateChanged(bool connected, const QString &statusMessage);
    void transportError(const QString &errorMessage);
    void logEvent(const QString &event, const QString &details, bool isError = false);
};

#endif // ITRANSPORT_H
