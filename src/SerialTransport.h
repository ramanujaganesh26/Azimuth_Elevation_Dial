#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "ITransport.h"
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialTransport : public ITransport
{
    Q_OBJECT
public:
    explicit SerialTransport(QObject *parent = nullptr);
    ~SerialTransport() override;

    QString transportName() const override { return QStringLiteral("Serial"); }
    bool isConnected() const override;
    QString connectionDetails() const override;

    void openConnection() override;
    void closeConnection() override;

    // Configuration getters & setters
    QString portName() const { return m_portName; }
    void setPortName(const QString &port);

    qint32 baudRate() const { return m_baudRate; }
    void setBaudRate(qint32 baud);

    QSerialPort::DataBits dataBits() const { return m_dataBits; }
    void setDataBits(QSerialPort::DataBits bits);

    QSerialPort::Parity parity() const { return m_parity; }
    void setParity(QSerialPort::Parity parity);

    QSerialPort::StopBits stopBits() const { return m_stopBits; }
    void setStopBits(QSerialPort::StopBits stopBits);

    QSerialPort::FlowControl flowControl() const { return m_flowControl; }
    void setFlowControl(QSerialPort::FlowControl flow);

    static QStringList availablePorts();

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    QSerialPort::DataBits m_dataBits;
    QSerialPort::Parity m_parity;
    QSerialPort::StopBits m_stopBits;
    QSerialPort::FlowControl m_flowControl;
};

#endif // SERIALTRANSPORT_H
