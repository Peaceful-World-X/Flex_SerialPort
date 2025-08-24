#ifndef BUTTONDATABASE_H
#define BUTTONDATABASE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStringConverter>

// 按键数据结构
struct ButtonData {
    QString remark;      // 按键备注
    QString command;     // 按键指令
    int row;            // 行位置
    int col;            // 列位置
    bool isValid;       // 是否有效
    bool isHexCommand;  // 是否为16进制指令，false为字符指令

    ButtonData() : row(-1), col(-1), isValid(false), isHexCommand(false) {}
    ButtonData(const QString &r, const QString &c, int row, int col, bool isHex = false)
        : remark(r), command(c), row(row), col(col), isValid(true), isHexCommand(isHex) {}
};

// 串口配置结构
struct SerialPortConfig {
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
    QString encoding;  // 中文编码方式

    SerialPortConfig() {
        portName = "";
        baudRate = 9600;
        dataBits = 8;
        stopBits = 1;
        parity = "NoParity";
        timestampDisplay = true;
        hexDisplay = false;
        hexSend = false;
        autoSendEnter = true;
        enterChars = "0D0A";
        encoding = "UTF-8";
    }
};

class ButtonDatabase : public QObject
{
    Q_OBJECT

public:
    explicit ButtonDatabase(QObject *parent = nullptr);
    ~ButtonDatabase();

    // 按键数据管理
    void setButtonData(int row, int col, const QString &remark, const QString &command, bool isHexCommand = false);
    ButtonData getButtonData(int row, int col) const;
    void removeButtonData(int row, int col);
    void clearAllButtons();

    // 获取所有按键数据
    QMap<QString, ButtonData> getAllButtons() const;

    // 串口配置管理
    void setSerialConfig(const SerialPortConfig &config);
    SerialPortConfig getSerialConfig() const;

    // 表格配置
    void setTableSize(int rows, int cols);
    QPair<int, int> getTableSize() const;

    // 窗口配置已移除

    // 保存和加载
    bool saveToFile();
    bool loadFromFile();

    // 获取配置文件路径
    QString getConfigFilePath() const;

signals:
    void dataChanged();

private:
    QMap<QString, ButtonData> buttonMap;  // 使用"row,col"作为key
    SerialPortConfig serialConfig;
    int tableRows;
    int tableCols;
    // windowGeometry 已移除

    QString configFilePath;
    QSettings *settings;  // 使用QSettings作为YAML的替代方案

    QString makeKey(int row, int col) const;
    void initializeConfig();
    void saveButtonToSettings(const QString &key, const ButtonData &data);
    ButtonData loadButtonFromSettings(const QString &key);
};

#endif // BUTTONDATABASE_H
