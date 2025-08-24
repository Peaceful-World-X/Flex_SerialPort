#include "configmanager.h"

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    buttonConfigFile = configDir + "/button_config.json";
    serialConfigFile = configDir + "/serial_config.json";
    windowConfigFile = configDir + "/window_config.json";

    ensureConfigDirExists();
}

void ConfigManager::ensureConfigDirExists()
{
    QDir dir;
    if (!dir.exists(configDir)) {
        dir.mkpath(configDir);
    }
}

QString ConfigManager::getConfigPath() const
{
    return configDir;
}

bool ConfigManager::isCommandText(const QString &text)
{
    // 检查是否是十六进制指令格式
    QRegularExpression hexPattern("^[0-9A-Fa-f\\s]+$");
    return hexPattern.match(text.trimmed()).hasMatch() && !text.trimmed().isEmpty();
}

bool ConfigManager::saveButtonConfig(QTableWidget *tableWidget)
{
    if (!tableWidget) return false;

    QJsonObject config;
    QJsonArray buttons;

    for (int row = 0; row < tableWidget->rowCount(); row++) {
        for (int col = 0; col < tableWidget->columnCount(); col++) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            if (item && !item->text().isEmpty()) {
                QJsonObject button;
                button["row"] = row;
                button["col"] = col;
                button["text"] = item->text();
                button["isCommand"] = isCommandText(item->text());
                buttons.append(button);
            }
        }
    }

    config["buttons"] = buttons;
    config["rows"] = tableWidget->rowCount();
    config["cols"] = tableWidget->columnCount();
    config["version"] = "1.0";

    QJsonDocument doc(config);
    QFile file(buttonConfigFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }
    return false;
}

bool ConfigManager::loadButtonConfig(QTableWidget *tableWidget)
{
    if (!tableWidget) return false;

    QFile file(buttonConfigFile);
    if (!file.exists()) return false;

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject config = doc.object();

        if (config.contains("rows") && config.contains("cols")) {
            tableWidget->setRowCount(config["rows"].toInt());
            tableWidget->setColumnCount(config["cols"].toInt());

            // 设置表头
            QStringList headers;
            for (int i = 0; i < tableWidget->columnCount(); i++) {
                headers << QString("按键%1").arg(i + 1);
            }
            tableWidget->setHorizontalHeaderLabels(headers);

            // 设置列宽
            for (int i = 0; i < tableWidget->columnCount(); i++) {
                tableWidget->setColumnWidth(i, 115);
            }
        }

        if (config.contains("buttons")) {
            QJsonArray buttons = config["buttons"].toArray();
            for (const QJsonValue &value : buttons) {
                QJsonObject button = value.toObject();
                int row = button["row"].toInt();
                int col = button["col"].toInt();
                QString text = button["text"].toString();
                bool isCommand = button["isCommand"].toBool();

                if (row < tableWidget->rowCount() && col < tableWidget->columnCount()) {
                    QTableWidgetItem *item = new QTableWidgetItem(text);
                    item->setTextAlignment(Qt::AlignCenter);

                    // 如果是指令，设置背景色
                    if (isCommand) {
                        item->setBackground(QBrush(QColor(240, 240, 240)));
                    }

                    tableWidget->setItem(row, col, item);
                }
            }
        }
        return true;
    }
    return false;
}

bool ConfigManager::saveSerialConfig(const SerialConfig &config)
{
    QJsonObject jsonConfig;
    jsonConfig["portName"] = config.portName;
    jsonConfig["baudRate"] = config.baudRate;
    jsonConfig["dataBits"] = config.dataBits;
    jsonConfig["stopBits"] = config.stopBits;
    jsonConfig["parity"] = config.parity;
    jsonConfig["timestampDisplay"] = config.timestampDisplay;
    jsonConfig["hexDisplay"] = config.hexDisplay;
    jsonConfig["hexSend"] = config.hexSend;
    jsonConfig["autoSendEnter"] = config.autoSendEnter;
    jsonConfig["enterChars"] = config.enterChars;
    jsonConfig["version"] = "1.0";

    QJsonDocument doc(jsonConfig);
    QFile file(serialConfigFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }
    return false;
}

SerialConfig ConfigManager::loadSerialConfig()
{
    SerialConfig config;
    // 设置默认值
    config.portName = "";
    config.baudRate = 9600;
    config.dataBits = 8;
    config.stopBits = 1;
    config.parity = "NoParity";
    config.timestampDisplay = true;
    config.hexDisplay = false;
    config.hexSend = false;
    config.autoSendEnter = true;
    config.enterChars = "0D0A";

    QFile file(serialConfigFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject jsonConfig = doc.object();

        if (jsonConfig.contains("portName")) config.portName = jsonConfig["portName"].toString();
        if (jsonConfig.contains("baudRate")) config.baudRate = jsonConfig["baudRate"].toInt();
        if (jsonConfig.contains("dataBits")) config.dataBits = jsonConfig["dataBits"].toInt();
        if (jsonConfig.contains("stopBits")) config.stopBits = jsonConfig["stopBits"].toInt();
        if (jsonConfig.contains("parity")) config.parity = jsonConfig["parity"].toString();
        if (jsonConfig.contains("timestampDisplay")) config.timestampDisplay = jsonConfig["timestampDisplay"].toBool();
        if (jsonConfig.contains("hexDisplay")) config.hexDisplay = jsonConfig["hexDisplay"].toBool();
        if (jsonConfig.contains("hexSend")) config.hexSend = jsonConfig["hexSend"].toBool();
        if (jsonConfig.contains("autoSendEnter")) config.autoSendEnter = jsonConfig["autoSendEnter"].toBool();
        if (jsonConfig.contains("enterChars")) config.enterChars = jsonConfig["enterChars"].toString();
    }

    return config;
}

bool ConfigManager::saveWindowGeometry(const QByteArray &geometry)
{
    QJsonObject config;
    config["geometry"] = QString::fromLatin1(geometry.toBase64());
    config["version"] = "1.0";

    QJsonDocument doc(config);
    QFile file(windowConfigFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }
    return false;
}

QByteArray ConfigManager::loadWindowGeometry()
{
    QFile file(windowConfigFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject config = doc.object();

        if (config.contains("geometry")) {
            QString geometryStr = config["geometry"].toString();
            return QByteArray::fromBase64(geometryStr.toLatin1());
        }
    }
    return QByteArray();
}
