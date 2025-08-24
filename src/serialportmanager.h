#ifndef SERIALPORTMANAGER_H
#define SERIALPORTMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QStringList>

class SerialPortManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager();

    // 串口管理
    QStringList getAvailablePorts();
    bool openPort(const QString &portName, int baudRate, int dataBits, 
                  int stopBits, const QString &parity);
    void closePort();
    bool isPortOpen() const;
    QString getPortName() const;
    QString getErrorString() const;

    // 数据发送
    qint64 sendData(const QByteArray &data);
    qint64 sendHexData(const QString &hexString);
    qint64 sendTextData(const QString &text);

    // 统计信息
    qint64 getSentBytes() const;
    qint64 getReceivedBytes() const;
    void resetStatistics();

    // 串口参数
    struct PortSettings {
        QString portName;
        int baudRate;
        int dataBits;
        int stopBits;
        QString parity;
    };

    PortSettings getCurrentSettings() const;

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorString);
    void portOpened();
    void portClosed();
    void statisticsChanged(qint64 sent, qint64 received);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *serialPort;
    qint64 sentBytes;
    qint64 receivedBytes;
    PortSettings currentSettings;

    QSerialPort::DataBits intToDataBits(int dataBits);
    QSerialPort::StopBits intToStopBits(int stopBits);
    QSerialPort::Parity stringToParity(const QString &parity);
};

#endif // SERIALPORTMANAGER_H
