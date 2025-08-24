#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QTextBrowser>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

class LogManager : public QObject
{
    Q_OBJECT

public:
    enum LogType {
        Send,
        Receive
    };

    explicit LogManager(QObject *parent = nullptr);

    // 设置日志显示控件
    void setSendLogWidget(QTextBrowser *widget);
    void setReceiveLogWidget(QTextBrowser *widget);

    // 日志记录
    void addSendLog(const QString &data, bool isHex = false);
    void addReceiveLog(const QString &data, bool isHex = false);
    void addSendLog(const QByteArray &data, bool isHex = false);
    void addReceiveLog(const QByteArray &data, bool isHex = false);

    // 显示控制
    void setTimestampEnabled(bool enabled);
    void setHexDisplayEnabled(bool enabled);
    void setPauseSendLog(bool paused);
    void setPauseReceiveLog(bool paused);

    bool isTimestampEnabled() const;
    bool isHexDisplayEnabled() const;
    bool isSendLogPaused() const;
    bool isReceiveLogPaused() const;

    // 日志管理
    void clearSendLog();
    void clearReceiveLog();
    void clearAllLogs();

    bool saveSendLog(const QString &filePath);
    bool saveReceiveLog(const QString &filePath);
    bool saveAllLogs(const QString &filePath);

    // 获取日志内容
    QString getSendLogContent() const;
    QString getReceiveLogContent() const;

private:
    QTextBrowser *sendLogWidget;
    QTextBrowser *receiveLogWidget;

    bool timestampEnabled;
    bool hexDisplayEnabled;
    bool sendLogPaused;
    bool receiveLogPaused;

    QString formatLogEntry(const QString &data, LogType type, bool isHex = false);
    QString formatData(const QString &data, bool isHex);
    QString formatData(const QByteArray &data, bool isHex);
    QString getCurrentTimestamp();
};

#endif // LOGMANAGER_H
