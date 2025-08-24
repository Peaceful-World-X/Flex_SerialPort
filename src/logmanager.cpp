#include "logmanager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

LogManager::LogManager(QObject *parent)
    : QObject(parent)
    , sendLogWidget(nullptr)
    , receiveLogWidget(nullptr)
    , timestampEnabled(true)
    , hexDisplayEnabled(false)
    , sendLogPaused(false)
    , receiveLogPaused(false)
{
}

void LogManager::setSendLogWidget(QTextBrowser *widget)
{
    sendLogWidget = widget;
}

void LogManager::setReceiveLogWidget(QTextBrowser *widget)
{
    receiveLogWidget = widget;
}

void LogManager::addSendLog(const QString &data, bool isHex)
{
    if (sendLogPaused || !sendLogWidget) return;
    
    QString logEntry = formatLogEntry(data, Send, isHex);
    sendLogWidget->insertPlainText(logEntry);
}

void LogManager::addReceiveLog(const QString &data, bool isHex)
{
    if (receiveLogPaused || !receiveLogWidget) return;
    
    QString logEntry = formatLogEntry(data, Receive, isHex);
    receiveLogWidget->insertPlainText(logEntry);
}

void LogManager::addSendLog(const QByteArray &data, bool isHex)
{
    addSendLog(formatData(data, isHex), isHex);
}

void LogManager::addReceiveLog(const QByteArray &data, bool isHex)
{
    addReceiveLog(formatData(data, isHex), isHex);
}

void LogManager::setTimestampEnabled(bool enabled)
{
    timestampEnabled = enabled;
}

void LogManager::setHexDisplayEnabled(bool enabled)
{
    hexDisplayEnabled = enabled;
}

void LogManager::setPauseSendLog(bool paused)
{
    sendLogPaused = paused;
}

void LogManager::setPauseReceiveLog(bool paused)
{
    receiveLogPaused = paused;
}

bool LogManager::isTimestampEnabled() const
{
    return timestampEnabled;
}

bool LogManager::isHexDisplayEnabled() const
{
    return hexDisplayEnabled;
}

bool LogManager::isSendLogPaused() const
{
    return sendLogPaused;
}

bool LogManager::isReceiveLogPaused() const
{
    return receiveLogPaused;
}

void LogManager::clearSendLog()
{
    if (sendLogWidget) {
        sendLogWidget->clear();
    }
}

void LogManager::clearReceiveLog()
{
    if (receiveLogWidget) {
        receiveLogWidget->clear();
    }
}

void LogManager::clearAllLogs()
{
    clearSendLog();
    clearReceiveLog();
}

bool LogManager::saveSendLog(const QString &filePath)
{
    if (!sendLogWidget) return false;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << sendLogWidget->toPlainText();
        file.close();
        return true;
    }
    return false;
}

bool LogManager::saveReceiveLog(const QString &filePath)
{
    if (!receiveLogWidget) return false;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << receiveLogWidget->toPlainText();
        file.close();
        return true;
    }
    return false;
}

bool LogManager::saveAllLogs(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "=== 发送日志 ===" << Qt::endl;
        if (sendLogWidget) {
            out << sendLogWidget->toPlainText();
        }
        out << Qt::endl << "=== 接收日志 ===" << Qt::endl;
        if (receiveLogWidget) {
            out << receiveLogWidget->toPlainText();
        }
        file.close();
        return true;
    }
    return false;
}

QString LogManager::getSendLogContent() const
{
    return sendLogWidget ? sendLogWidget->toPlainText() : QString();
}

QString LogManager::getReceiveLogContent() const
{
    return receiveLogWidget ? receiveLogWidget->toPlainText() : QString();
}

QString LogManager::formatLogEntry(const QString &data, LogType type, bool isHex)
{
    QString prefix = (type == Send) ? "[发送]" : "[接收]";
    QString formattedData = formatData(data, isHex || hexDisplayEnabled);
    
    if (timestampEnabled) {
        return QString("%1 %2 %3\n").arg(getCurrentTimestamp(), prefix, formattedData);
    } else {
        return QString("%1 %2\n").arg(prefix, formattedData);
    }
}

QString LogManager::formatData(const QString &data, bool isHex)
{
    if (isHex) {
        // 如果数据已经是十六进制格式，直接返回
        return data.toUpper();
    } else {
        // 如果需要转换为十六进制
        if (hexDisplayEnabled) {
            return data.toUtf8().toHex(' ').toUpper();
        } else {
            return data;
        }
    }
}

QString LogManager::formatData(const QByteArray &data, bool isHex)
{
    if (isHex || hexDisplayEnabled) {
        return data.toHex(' ').toUpper();
    } else {
        return QString::fromUtf8(data);
    }
}

QString LogManager::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}
