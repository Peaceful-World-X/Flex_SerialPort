#include "buttondatabase.h"
#include <QDebug>

ButtonDatabase::ButtonDatabase(QObject *parent)
    : QObject(parent)
    , tableRows(6)
    , tableCols(8)
{
    initializeConfig();
    loadFromFile();
}

ButtonDatabase::~ButtonDatabase()
{
    saveToFile();
    delete settings;
}

void ButtonDatabase::initializeConfig()
{
    // 获取exe文件所在目录
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir().mkpath(exeDir);

    configFilePath = exeDir + "/flex_serialport_config.yaml";

    // 使用QSettings模拟YAML格式（实际为INI格式，但文件名为.yaml）
    settings = new QSettings(configFilePath, QSettings::IniFormat);
    // Qt6中已移除setIniCodec，默认使用UTF-8编码
}

QString ButtonDatabase::makeKey(int row, int col) const
{
    return QString("%1,%2").arg(row).arg(col);
}

void ButtonDatabase::setButtonData(int row, int col, const QString &remark, const QString &command, bool isHexCommand)
{
    QString key = makeKey(row, col);
    ButtonData data(remark, command, row, col, isHexCommand);
    buttonMap[key] = data;

    saveToFile(); // 直接保存整个文件
    emit dataChanged();
}

ButtonData ButtonDatabase::getButtonData(int row, int col) const
{
    QString key = makeKey(row, col);
    return buttonMap.value(key, ButtonData());
}

void ButtonDatabase::removeButtonData(int row, int col)
{
    QString key = makeKey(row, col);
    buttonMap.remove(key);

    saveToFile(); // 直接保存整个文件
    emit dataChanged();
}

void ButtonDatabase::clearAllButtons()
{
    buttonMap.clear();
    settings->beginGroup("Buttons");
    settings->clear();
    settings->endGroup();

    emit dataChanged();
}

QMap<QString, ButtonData> ButtonDatabase::getAllButtons() const
{
    return buttonMap;
}

void ButtonDatabase::setSerialConfig(const SerialPortConfig &config)
{
    serialConfig = config;

    settings->beginGroup("SerialPort");
    settings->setValue("portName", config.portName);
    settings->setValue("baudRate", config.baudRate);
    settings->setValue("dataBits", config.dataBits);
    settings->setValue("stopBits", config.stopBits);
    settings->setValue("parity", config.parity);
    settings->setValue("timestampDisplay", config.timestampDisplay);
    settings->setValue("hexDisplay", config.hexDisplay);
    settings->setValue("hexSend", config.hexSend);
    settings->setValue("autoSendEnter", config.autoSendEnter);
    settings->setValue("enterChars", config.enterChars);
    settings->setValue("encoding", config.encoding);
    settings->endGroup();

    settings->sync();
}

SerialPortConfig ButtonDatabase::getSerialConfig() const
{
    return serialConfig;
}

void ButtonDatabase::setTableSize(int rows, int cols)
{
    tableRows = rows;
    tableCols = cols;

    settings->beginGroup("Table");
    settings->setValue("rows", rows);
    settings->setValue("cols", cols);
    settings->endGroup();

    settings->sync();
}

QPair<int, int> ButtonDatabase::getTableSize() const
{
    return QPair<int, int>(tableRows, tableCols);
}

// 窗口几何信息相关方法已移除

bool ButtonDatabase::saveToFile()
{
    // 使用自定义YAML格式保存
    QFile file(configFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开配置文件进行写入:" << configFilePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8); // Qt6中使用setEncoding

    // 写入YAML格式的配置
    out << "# Flex SerialPort Configuration File\n";
    out << "# 自动生成，请勿手动编辑\n\n";

    // 保存串口配置
    out << "SerialPort:\n";
    out << "  portName: \"" << serialConfig.portName << "\"\n";
    out << "  baudRate: " << serialConfig.baudRate << "\n";
    out << "  dataBits: " << serialConfig.dataBits << "\n";
    out << "  stopBits: " << serialConfig.stopBits << "\n";
    out << "  parity: \"" << serialConfig.parity << "\"\n";
    out << "  timestampDisplay: " << (serialConfig.timestampDisplay ? "true" : "false") << "\n";
    out << "  hexDisplay: " << (serialConfig.hexDisplay ? "true" : "false") << "\n";
    out << "  hexSend: " << (serialConfig.hexSend ? "true" : "false") << "\n";
    out << "  autoSendEnter: " << (serialConfig.autoSendEnter ? "true" : "false") << "\n";
    out << "  enterChars: \"" << serialConfig.enterChars << "\"\n";
    out << "  encoding: \"" << serialConfig.encoding << "\"\n\n";

    // 保存表格配置
    out << "Table:\n";
    out << "  rows: " << tableRows << "\n";
    out << "  cols: " << tableCols << "\n\n";

    // 窗口配置已移除，不再保存窗口几何信息

    // 保存按键数据
    if (!buttonMap.isEmpty()) {
        out << "Buttons:\n";
        for (auto it = buttonMap.begin(); it != buttonMap.end(); ++it) {
            const ButtonData &data = it.value();
            if (data.isValid) {
                out << "  \"" << it.key() << "\":\n";
                out << "    remark: \"" << data.remark << "\"\n";
                out << "    command: \"" << data.command << "\"\n";
                out << "    row: " << data.row << "\n";
                out << "    col: " << data.col << "\n";
                out << "    isValid: " << (data.isValid ? "true" : "false") << "\n";
            }
        }
    }

    file.close();
    return true;
}

bool ButtonDatabase::loadFromFile()
{
    buttonMap.clear();

    QFile configFile(configFilePath);
    if(!configFile.exists()){
        return false;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&configFile);
    in.setEncoding(QStringConverter::Utf8);

    QString currentSection;
    QString currentButtonKey;
    ButtonData currentButtonData;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QString trimmedLine = line.trimmed();

        // 跳过注释和空行
        if (trimmedLine.isEmpty() || trimmedLine.startsWith("#")) {
            continue;
        }

        // 检查是否是节标题 (顶级节，不以空格开头)
        if (trimmedLine.endsWith(":") && !line.startsWith(" ")) {
            // 保存之前的按键数据
            if (!currentButtonKey.isEmpty() && currentButtonData.isValid) {
                buttonMap[currentButtonKey] = currentButtonData;
                currentButtonKey.clear();
                currentButtonData = ButtonData();
            }

            currentSection = trimmedLine.left(trimmedLine.length() - 1);
            continue;
        }

        // 在Buttons节中处理按键数据
        if (currentSection == "Buttons") {
            // 检查是否是按键名 (两个空格缩进，以引号开头，以冒号结尾)
            if (line.startsWith("  ") && !line.startsWith("    ") && trimmedLine.startsWith("\"") && trimmedLine.endsWith(":")) {
                // 保存之前的按键数据
                if (!currentButtonKey.isEmpty() && currentButtonData.isValid) {
                    buttonMap[currentButtonKey] = currentButtonData;
                }

                // 提取按键名 (去掉引号和冒号)
                currentButtonKey = trimmedLine.mid(1, trimmedLine.length() - 3);
                currentButtonData = ButtonData();
                currentButtonData.isValid = true;
                continue;
            }

            // 检查是否是按键属性 (四个空格缩进)
            if (line.startsWith("    ") && trimmedLine.contains(":")) {
                QStringList parts = trimmedLine.split(":", Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts[1].trimmed().remove("\"");

                    if (key == "remark") {
                        currentButtonData.remark = value;
                    }
                    else if (key == "command") {
                        currentButtonData.command = value;
                    }
                    else if (key == "row") {
                        currentButtonData.row = value.toInt();
                    }
                    else if (key == "col") {
                        currentButtonData.col = value.toInt();
                    }
                    else if (key == "isValid") {
                        currentButtonData.isValid = (value == "true");
                    }
                }
                continue;
            }
        }

        // 处理其他节的键值对
        if (trimmedLine.contains(":") && line.startsWith("  ") && !line.startsWith("    ")) {
            QStringList parts = trimmedLine.split(":", Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString key = parts[0].trimmed();
                QString value = parts[1].trimmed().remove("\"");

                if (currentSection == "SerialPort") {
                    if (key == "portName") serialConfig.portName = value;
                    else if (key == "baudRate") serialConfig.baudRate = value.toInt();
                    else if (key == "dataBits") serialConfig.dataBits = value.toInt();
                    else if (key == "stopBits") serialConfig.stopBits = value.toInt();
                    else if (key == "parity") serialConfig.parity = value;
                    else if (key == "timestampDisplay") serialConfig.timestampDisplay = (value == "true");
                    else if (key == "hexDisplay") serialConfig.hexDisplay = (value == "true");
                    else if (key == "hexSend") serialConfig.hexSend = (value == "true");
                    else if (key == "autoSendEnter") serialConfig.autoSendEnter = (value == "true");
                    else if (key == "enterChars") serialConfig.enterChars = value;
                    else if (key == "encoding") serialConfig.encoding = value;
                }
                else if (currentSection == "Table") {
                    if (key == "rows") tableRows = value.toInt();
                    else if (key == "cols") tableCols = value.toInt();
                }
                // Window配置已移除
            }
        }
    }

    // 保存最后一个按键数据
    if (!currentButtonKey.isEmpty() && currentButtonData.isValid) {
        buttonMap[currentButtonKey] = currentButtonData;
    }

    configFile.close();
    return true;
}

QString ButtonDatabase::getConfigFilePath() const
{
    return configFilePath;
}

void ButtonDatabase::saveButtonToSettings(const QString &key, const ButtonData &data)
{
    settings->beginGroup("Buttons");
    settings->beginGroup(key);
    settings->setValue("remark", data.remark);
    settings->setValue("command", data.command);
    settings->setValue("row", data.row);
    settings->setValue("col", data.col);
    settings->setValue("isValid", data.isValid);
    settings->setValue("isHexCommand", data.isHexCommand);
    settings->endGroup(); // 结束key组
    settings->endGroup(); // 结束Buttons组
}

ButtonData ButtonDatabase::loadButtonFromSettings(const QString &key)
{
    settings->beginGroup("Buttons");
    settings->beginGroup(key);
    ButtonData data;
    data.remark = settings->value("remark", "").toString();
    data.command = settings->value("command", "").toString();
    data.row = settings->value("row", -1).toInt();
    data.col = settings->value("col", -1).toInt();
    data.isValid = settings->value("isValid", false).toBool();
    data.isHexCommand = settings->value("isHexCommand", false).toBool();
    settings->endGroup(); // 结束key组
    settings->endGroup(); // 结束Buttons组

    return data;
}
