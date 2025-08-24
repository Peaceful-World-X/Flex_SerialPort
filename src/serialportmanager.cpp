#include "serialportmanager.h"
#include <QDebug>

SerialPortManager::SerialPortManager(QObject *parent)
    : QObject(parent)
    , serialPort(new QSerialPort(this))
    , sentBytes(0)
    , receivedBytes(0)
{
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortManager::handleReadyRead);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &SerialPortManager::handleError);
}

SerialPortManager::~SerialPortManager()
{
    closePort();
}

QStringList SerialPortManager::getAvailablePorts()
{
    QStringList portNames;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        portNames << info.portName();
    }
    return portNames;
}

bool SerialPortManager::openPort(const QString &portName, int baudRate, int dataBits, 
                                 int stopBits, const QString &parity)
{
    if (serialPort->isOpen()) {
        closePort();
    }

    serialPort->setPortName(portName);
    
    if (!serialPort->open(QIODevice::ReadWrite)) {
        return false;
    }

    // 设置串口参数
    if (!serialPort->setBaudRate(baudRate) ||
        !serialPort->setDataBits(intToDataBits(dataBits)) ||
        !serialPort->setStopBits(intToStopBits(stopBits)) ||
        !serialPort->setParity(stringToParity(parity))) {
        
        serialPort->close();
        return false;
    }

    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->clear();

    // 保存当前设置
    currentSettings.portName = portName;
    currentSettings.baudRate = baudRate;
    currentSettings.dataBits = dataBits;
    currentSettings.stopBits = stopBits;
    currentSettings.parity = parity;

    emit portOpened();
    return true;
}

void SerialPortManager::closePort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        emit portClosed();
    }
}

bool SerialPortManager::isPortOpen() const
{
    return serialPort->isOpen();
}

QString SerialPortManager::getPortName() const
{
    return serialPort->portName();
}

QString SerialPortManager::getErrorString() const
{
    return serialPort->errorString();
}

qint64 SerialPortManager::sendData(const QByteArray &data)
{
    if (!serialPort->isOpen()) {
        return -1;
    }

    qint64 bytesWritten = serialPort->write(data);
    if (bytesWritten > 0) {
        sentBytes += bytesWritten;
        emit statisticsChanged(sentBytes, receivedBytes);
    }
    return bytesWritten;
}

qint64 SerialPortManager::sendHexData(const QString &hexString)
{
    QString cleanHex = hexString;
    cleanHex.remove(' ').remove('\n').remove('\r');
    QByteArray data = QByteArray::fromHex(cleanHex.toLatin1());
    return sendData(data);
}

qint64 SerialPortManager::sendTextData(const QString &text)
{
    return sendData(text.toUtf8());
}

qint64 SerialPortManager::getSentBytes() const
{
    return sentBytes;
}

qint64 SerialPortManager::getReceivedBytes() const
{
    return receivedBytes;
}

void SerialPortManager::resetStatistics()
{
    sentBytes = 0;
    receivedBytes = 0;
    emit statisticsChanged(sentBytes, receivedBytes);
}

SerialPortManager::PortSettings SerialPortManager::getCurrentSettings() const
{
    return currentSettings;
}

void SerialPortManager::handleReadyRead()
{
    QByteArray data = serialPort->readAll();
    if (!data.isEmpty()) {
        receivedBytes += data.size();
        emit statisticsChanged(sentBytes, receivedBytes);
        emit dataReceived(data);
    }
}

void SerialPortManager::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    QString errorString;
    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorString = "设备未找到";
            break;
        case QSerialPort::PermissionError:
            errorString = "权限错误，设备可能被其他程序占用";
            break;
        case QSerialPort::OpenError:
            errorString = "设备打开失败";
            break;
        case QSerialPort::WriteError:
            errorString = "数据写入失败";
            break;
        case QSerialPort::ReadError:
            errorString = "数据读取失败";
            break;
        case QSerialPort::ResourceError:
            errorString = "设备资源不可用，设备可能被意外移除";
            break;
        case QSerialPort::UnsupportedOperationError:
            errorString = "不支持的操作";
            break;
        case QSerialPort::TimeoutError:
            errorString = "操作超时";
            break;
        default:
            errorString = QString("未知错误 (错误代码: %1)").arg(error);
            break;
    }

    emit errorOccurred(errorString);
}

QSerialPort::DataBits SerialPortManager::intToDataBits(int dataBits)
{
    switch (dataBits) {
        case 5: return QSerialPort::Data5;
        case 6: return QSerialPort::Data6;
        case 7: return QSerialPort::Data7;
        case 8: return QSerialPort::Data8;
        default: return QSerialPort::Data8;
    }
}

QSerialPort::StopBits SerialPortManager::intToStopBits(int stopBits)
{
    return (stopBits == 2) ? QSerialPort::TwoStop : QSerialPort::OneStop;
}

QSerialPort::Parity SerialPortManager::stringToParity(const QString &parity)
{
    if (parity == "EvenParity") {
        return QSerialPort::EvenParity;
    } else if (parity == "OddParity") {
        return QSerialPort::OddParity;
    } else {
        return QSerialPort::NoParity;
    }
}
