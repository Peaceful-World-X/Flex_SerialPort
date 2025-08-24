#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>
#include <QRegularExpression>

struct ButtonConfig {
    int row;
    int col;
    QString text;
    bool isCommand;
};

struct SerialConfig {
    QString portName;
    int baudRate;
    int dataBits;
    int stopBits;
    QString parity;
    bool timestampDisplay;
    bool hexDisplay;
    bool hexSend;
    bool autoSendEnter;
    QString enterChars;
};

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    
    // 按键配置管理
    bool saveButtonConfig(QTableWidget *tableWidget);
    bool loadButtonConfig(QTableWidget *tableWidget);
    
    // 串口配置管理
    bool saveSerialConfig(const SerialConfig &config);
    SerialConfig loadSerialConfig();
    
    // 窗口配置管理
    bool saveWindowGeometry(const QByteArray &geometry);
    QByteArray loadWindowGeometry();
    
    // 获取配置文件路径
    QString getConfigPath() const;
    
private:
    QString configDir;
    QString buttonConfigFile;
    QString serialConfigFile;
    QString windowConfigFile;
    
    void ensureConfigDirExists();
    bool isCommandText(const QString &text);
};

#endif // CONFIGMANAGER_H
