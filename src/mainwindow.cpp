#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QRegularExpression>
#include <QAbstractItemView>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    this->serialPort = new QSerialPort;
    this->configManager = new ConfigManager(this);
    this->buttonDatabase = new ButtonDatabase(this);
    this->autoSendTimer = new QTimer(this);

    // 初始化变量
    sendCount = 0;
    receiveCount = 0;
    isPauseSendLog = false;
    isPauseReceiveLog = false;
    isHexDisplay = false;
    isTimestampDisplay = true;

    findFreePorts();
    loadAllConfigs();
    setupTableWidget();

    // 串口连接信号槽
    connect(ui->btnOpenPort, &QPushButton::clicked, this, &MainWindow::onOpenSerialPort);
    connect(ui->btnClosePort, &QPushButton::clicked, this, &MainWindow::onCloseSerialPort);

    // 为端口下拉框安装事件过滤器，实现点击时自动刷新
    ui->portName->installEventFilter(this);

    connect(this->serialPort, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    connect(this->serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
            this, SLOT(onSerialError(QSerialPort::SerialPortError)));
    connect(ui->btnSend, &QPushButton::clicked, [=](){
        sendMsg(ui->message->toPlainText());
    });

    // TableWidget信号连接
    connect(ui->tableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(onTableCellClicked(int,int)));
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(onRemoveRowClicked()));
    connect(ui->pushButton_7, SIGNAL(clicked()), this, SLOT(onAddRowClicked()));

    // 日志管理按钮连接
    connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(onClearSendLogClicked()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(onSaveSendLogClicked()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(onPauseSendLogClicked()));
    connect(ui->pushButton_4, SIGNAL(clicked()), this, SLOT(onClearReceiveLogClicked()));
    connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(onSaveReceiveLogClicked()));
    connect(ui->pushButton_6, SIGNAL(clicked()), this, SLOT(onPauseReceiveLogClicked()));

    // 显示选项连接
    connect(ui->checkBox_1, &QCheckBox::toggled, [=](bool checked){
        isTimestampDisplay = checked;
    });
    connect(ui->checkBox_2, &QCheckBox::toggled, [=](bool checked){
        isHexDisplay = checked;
    });

    // 自动发送功能连接
    connect(ui->checkBox_autoSend, &QCheckBox::toggled, [=](bool checked){
        if(checked && serialPort->isOpen()){
            autoSendData = ui->message->toPlainText();
            if(!autoSendData.isEmpty()){
                autoSendTimer->start(ui->spinBox_interval->value());
            }
        } else {
            autoSendTimer->stop();
        }
    });

    connect(ui->spinBox_interval, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value){
        if(autoSendTimer->isActive()){
            autoSendTimer->setInterval(value);
        }
    });

    connect(autoSendTimer, &QTimer::timeout, this, &MainWindow::onAutoSendTimeout);

    // 为发送输入框安装事件过滤器，实现回车发送功能
    ui->message->installEventFilter(this);

    // 为快速配置输入框安装事件过滤器，实现回车配置功能
    ui->lineEdit_cig->installEventFilter(this);

    // 延迟调整表格列宽，确保UI完全初始化后再调整
    QTimer::singleShot(100, this, [this](){
        adjustTableColumnWidths();
    });
}

MainWindow::~MainWindow()
{
    saveAllConfigs();

    // 停止自动发送定时器
    if(autoSendTimer && autoSendTimer->isActive()){
        autoSendTimer->stop();
    }

    // 关闭串口
    if(serialPort && serialPort->isOpen()){
        serialPort->close();
    }

    // 清理资源（Qt的父子关系会自动清理，但显式清理更安全）
    delete serialPort;
    delete configManager;
    delete buttonDatabase;
    delete ui;
}

// =====================================================================================
//寻找空闲状态串口
void MainWindow::findFreePorts(){
    // 清空现有端口列表
    ui->portName->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : ports){
        ui->portName->addItem(portInfo.portName());
    }

    if (ports.isEmpty()){
        // 不显示警告弹窗，而是在端口列表中显示友好提示
        ui->portName->addItem("无可用端口");
        ui->portName->setEnabled(false);
        showStatusMessage("未检测到可用串口，请插入串口设备后刷新");
        return;
    } else {
        ui->portName->setEnabled(true);
        showStatusMessage(QString("检测到 %1 个可用串口").arg(ports.size()));
    }
}

void MainWindow::refreshPorts(){
    QString currentPort = ui->portName->currentText();

    // 如果当前显示的是"无可用端口"，清空当前端口记录
    if (currentPort == "无可用端口") {
        currentPort.clear();
    }

    findFreePorts();

    // 尝试恢复之前选择的端口
    if (!currentPort.isEmpty()) {
        int index = ui->portName->findText(currentPort);
        if (index >= 0) {
            ui->portName->setCurrentIndex(index);
            showStatusMessage(QString("已恢复端口选择: %1").arg(currentPort));
        } else {
            showStatusMessage(QString("端口 %1 不再可用").arg(currentPort));
        }
    }
}



bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    // 处理发送输入框的回车事件
    if (obj == ui->message && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 检查是否按下了Shift键，如果按下则插入换行符
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false; // 让默认处理继续，插入换行符
            } else {
                // 检查是否启用了回车自动发送功能
                if (ui->checkBox_3->isChecked() && serialPort->isOpen()) {
                    sendMsg(ui->message->toPlainText());
                    return true; // 阻止默认处理
                }
                // 如果没有启用回车自动发送，让默认处理继续（插入换行符）
                return false;
            }
        }
    }
    // 处理快速配置输入框的回车事件
    else if (obj == ui->lineEdit_cig && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 解析并应用快速配置
            parseAndApplyQuickConfig(ui->lineEdit_cig->text());
            return true; // 阻止默认处理
        }
    }
    // 处理端口下拉框的鼠标点击事件
    else if (obj == ui->portName && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // 在显示下拉列表前刷新端口
            refreshPorts();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::parseAndApplyQuickConfig(const QString &configText){
    QString cleanText = configText.trimmed();
    if(cleanText.isEmpty()){
        showStatusMessage("配置文本为空");
        return;
    }

    // 支持逗号或中文逗号分隔
    QStringList parts = cleanText.split(QRegularExpression("[,，]"));

    if(parts.size() < 4){
        QMessageBox::warning(this, "配置错误",
            "配置格式错误！\n正确格式：波特率,校验,数据位,停止位\n例如：9600,N,8,1");
        return;
    }

    // 解析各个参数
    QString baudRateStr = parts[0].trimmed();
    QString parityStr = parts[1].trimmed().toUpper();
    QString dataBitsStr = parts[2].trimmed();
    QString stopBitsStr = parts[3].trimmed();

    // 验证并设置波特率
    bool ok;
    int baudRate = baudRateStr.toInt(&ok);
    if(!ok || baudRate <= 0 || baudRate > 10000000){
        QMessageBox::warning(this, "配置错误",
            QString("波特率格式错误！\n输入值：%1\n支持范围：1-10000000").arg(baudRateStr));
        return;
    }

    // 验证并设置校验位
    QString parityText;
    if(parityStr == "N" || parityStr == "NONE"){
        parityText = "N(无)";
    } else if(parityStr == "E" || parityStr == "EVEN"){
        parityText = "E(奇校验)";
    } else if(parityStr == "O" || parityStr == "ODD"){
        parityText = "O(偶校验)";
    } else {
        QMessageBox::warning(this, "配置错误",
            "校验位格式错误！\n支持：N/NONE(无), E/EVEN(偶校验), O/ODD(奇校验)");
        return;
    }

    // 验证并设置数据位
    int dataBits = dataBitsStr.toInt(&ok);
    if(!ok || (dataBits < 5 || dataBits > 8)){
        QMessageBox::warning(this, "配置错误", "数据位必须是5-8之间的整数！");
        return;
    }

    // 验证并设置停止位
    int stopBits = stopBitsStr.toInt(&ok);
    if(!ok || (stopBits != 1 && stopBits != 2)){
        QMessageBox::warning(this, "配置错误", "停止位必须是1或2！");
        return;
    }

    // 应用配置到界面
    // 设置波特率
    ui->baudRate->setCurrentText(QString::number(baudRate));

    // 设置校验位
    int parityIndex = ui->parity->findText(parityText);
    if(parityIndex >= 0){
        ui->parity->setCurrentIndex(parityIndex);
    }

    // 设置数据位
    ui->dataBits->setCurrentText(QString::number(dataBits));

    // 设置停止位
    ui->stopBits->setCurrentText(QString::number(stopBits));

    showStatusMessage(QString("配置已应用：%1,%2,%3,%4")
                     .arg(baudRate).arg(parityStr).arg(dataBits).arg(stopBits));
}

void MainWindow::sendHexCommand(const QString &hexCommand){
    if(!this->serialPort->isOpen()){
        QMessageBox::warning(this, "警告", "串口未打开！");
        showStatusMessage("串口未打开");
        return;
    }

    QString cleanHex = hexCommand.trimmed();
    if(cleanHex.isEmpty()){
        return;
    }

    if(!validateHexInput(QString(cleanHex).remove(' '))){
        QMessageBox::warning(this, "错误", "十六进制格式错误！");
        return;
    }

    QByteArray data = QByteArray::fromHex(cleanHex.toLatin1());
    sendDataToPort(data, cleanHex, true);
}

void MainWindow::sendTextCommand(const QString &textCommand){
    if(!this->serialPort->isOpen()){
        QMessageBox::warning(this, "警告", "串口未打开！");
        showStatusMessage("串口未打开");
        return;
    }

    QString cleanText = textCommand.trimmed();
    if(cleanText.isEmpty()){
        return;
    }

    // 简化：直接使用UTF-8编码
    QByteArray data = cleanText.toUtf8();
    sendDataToPort(data, cleanText, false);
}

void MainWindow::sendDataToPort(const QByteArray &data, const QString &displayText, bool isHex){
    if(!this->serialPort->isOpen()){
        return;
    }

    qint64 bytesWritten = this->serialPort->write(data);
    if(bytesWritten > 0){
        sendCount += bytesWritten;
        updateStatistics();
        showStatusMessage(QString("发送成功：%1 字节").arg(bytesWritten));

        if(!isPauseSendLog){
            QString displayMsg;
            if(isHex){
                displayMsg = data.toHex(' ').toUpper();
            } else {
                displayMsg = displayText;
            }

            QString logEntry;
            if(isTimestampDisplay){
                logEntry = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " " + displayMsg + "\n";
            } else {
                logEntry = displayMsg + "\n";
            }
            ui->comLog_1->insertPlainText(logEntry);
        }
    } else {
        QString errorMsg = QString("数据发送失败！\n错误信息：%1").arg(this->serialPort->errorString());
        QMessageBox::warning(this, "错误", errorMsg);
        showStatusMessage("发送失败");
    }
}
//初始化串口
bool MainWindow::initSerialPort(){
    // 检查串口名称是否为空或无效
    QString portName = ui->portName->currentText();
    if(portName.isEmpty() || portName == "无可用端口"){
        QMessageBox::warning(this, "警告", "请选择有效的串口！");
        return false;
    }

    // 如果串口已经打开，先关闭
    if(this->serialPort->isOpen()){
        this->serialPort->close();
    }

    // 设置串口参数
    this->serialPort->setPortName(portName);

    // 尝试打开串口
    if (!this->serialPort->open(QIODevice::ReadWrite)){
        QString errorMsg = QString("串口 %1 打开失败！\n错误信息：%2")
                          .arg(portName)
                          .arg(this->serialPort->errorString());
        QMessageBox::warning(this, "错误", errorMsg);
        return false;
    }

    // 设置波特率
    int baudRate = ui->baudRate->currentText().toInt();
    if(!this->serialPort->setBaudRate(baudRate)){
        QMessageBox::warning(this, "错误", "设置波特率失败！");
        this->serialPort->close();
        return false;
    }

    // 设置数据位
    int dataBits = ui->dataBits->currentText().toInt();
    QSerialPort::DataBits dataBitsEnum;
    switch(dataBits){
        case 5: dataBitsEnum = QSerialPort::Data5; break;
        case 6: dataBitsEnum = QSerialPort::Data6; break;
        case 7: dataBitsEnum = QSerialPort::Data7; break;
        case 8: dataBitsEnum = QSerialPort::Data8; break;
        default: dataBitsEnum = QSerialPort::Data8; break;
    }
    this->serialPort->setDataBits(dataBitsEnum);

    // 设置停止位
    int stopBits = ui->stopBits->currentText().toInt();
    QSerialPort::StopBits stopBitsEnum = (stopBits == 2) ? QSerialPort::TwoStop : QSerialPort::OneStop;
    this->serialPort->setStopBits(stopBitsEnum);

    // 设置校验位
    QString parity = ui->parity->currentText();
    QSerialPort::Parity parityEnum;
    if(parity == "EvenParity"){
        parityEnum = QSerialPort::EvenParity;
    }else if(parity == "OddParity"){
        parityEnum = QSerialPort::OddParity;
    }else{
        parityEnum = QSerialPort::NoParity;
    }
    this->serialPort->setParity(parityEnum);

    // 设置流控制
    this->serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // 清空缓冲区
    this->serialPort->clear();

    return true;
}
//向串口发送信息
void MainWindow::sendMsg(const QString &msg){
    if(!this->serialPort->isOpen()){
        QMessageBox::warning(this, "警告", "串口未打开！");
        showStatusMessage("串口未打开");
        return;
    }

    QString cleanMsg = msg.trimmed();
    if(cleanMsg.isEmpty()){
        QMessageBox::information(this, "提示", "发送内容不能为空！");
        return;
    }

    QByteArray data;

    if(ui->checkBox_5->isChecked()){ // 16进制发送
        if(!validateHexInput(cleanMsg)){
            QMessageBox::warning(this, "错误", "十六进制格式错误！\n请输入有效的十六进制字符（0-9, A-F），且长度为偶数。");
            return;
        }
        data = QByteArray::fromHex(cleanMsg.toLatin1());
    } else {
        // 简化：直接使用UTF-8编码
        data = cleanMsg.toUtf8();
    }

    // 添加回车换行
    if(ui->checkBox_4->isChecked()){
        QString endChars = ui->lineEdit->text().trimmed();
        if(!endChars.isEmpty()){
            if(!validateHexInput(endChars)){
                QMessageBox::warning(this, "错误", "回车字符格式错误！\n请输入有效的十六进制字符。");
                return;
            }
            data.append(QByteArray::fromHex(endChars.toLatin1()));
        } else {
            // 如果没有设置自定义回车字符，使用默认的回车换行
            data.append(QByteArray::fromHex("0D0A")); // CR LF
        }
    }

    qint64 bytesWritten = this->serialPort->write(data);
    if(bytesWritten > 0){
        sendCount += bytesWritten;
        updateStatistics();
        showStatusMessage(QString("发送成功：%1 字节").arg(bytesWritten));

        if(!isPauseSendLog){
            QString displayMsg;
            if(isHexDisplay){
                displayMsg = data.toHex(' ').toUpper();
            } else {
                displayMsg = cleanMsg;
            }

            QString logEntry;
            if(isTimestampDisplay){
                logEntry = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " " + displayMsg + "\n";
            } else {
                logEntry = displayMsg + "\n";
            }
            ui->comLog_1->insertPlainText(logEntry);
        }
    } else {
        QString errorMsg = QString("数据发送失败！\n错误信息：%1").arg(this->serialPort->errorString());
        QMessageBox::warning(this, "错误", errorMsg);
        showStatusMessage("发送失败");
    }
}
//接受来自串口的信息
void MainWindow::recvMsg(){
    QByteArray newData = this->serialPort->readAll();
    if(newData.isEmpty()) return;

    receiveCount += newData.size();
    updateStatistics();
    showStatusMessage(QString("接收：%1 字节").arg(newData.size()));

    if (!isPauseReceiveLog) {
        // 实时显示模式：立即显示接收到的数据，不使用缓存
        displayCompleteMessage(newData);
    }
}

void MainWindow::displayCompleteMessage(const QByteArray &message){
    // 使用UTF-8解码消息
    QString displayMsg = QString::fromUtf8(message);

    // 移除回车符，避免显示问题，但保留换行符
    displayMsg.remove(QChar('\r'));

    // 实时显示：不强制添加换行符，保持原始格式
    QString logEntry;
    if (isTimestampDisplay) {
        // 只在数据开始时添加时间戳，避免每个字节都有时间戳
        static QDateTime lastTimestamp;
        QDateTime currentTime = QDateTime::currentDateTime();

        // 如果距离上次时间戳超过1秒，或者数据以换行符开始，添加新时间戳
        if (lastTimestamp.isNull() ||
            lastTimestamp.msecsTo(currentTime) > 1000 ||
            displayMsg.startsWith('\n')) {

            if (!displayMsg.startsWith('\n') && !ui->comLog_2->toPlainText().endsWith('\n')) {
                logEntry = "\n" + currentTime.toString("yyyy-MM-dd hh:mm:ss") + " " + displayMsg;
            } else {
                logEntry = currentTime.toString("yyyy-MM-dd hh:mm:ss") + " " + displayMsg;
            }
            lastTimestamp = currentTime;
        } else {
            logEntry = displayMsg;
        }
    } else {
        logEntry = displayMsg;
    }

    ui->comLog_2->insertPlainText(logEntry);

    // 自动滚动到底部
    ui->comLog_2->moveCursor(QTextCursor::End);
}

// 缓存处理函数已移除，改为实时显示

void MainWindow::onSerialError(QSerialPort::SerialPortError error){
    if(error == QSerialPort::NoError){
        return;
    }

    QString errorMsg;
    switch(error){
        case QSerialPort::DeviceNotFoundError:
            errorMsg = "设备未找到";
            break;
        case QSerialPort::PermissionError:
            errorMsg = "权限错误，设备可能被其他程序占用";
            break;
        case QSerialPort::OpenError:
            errorMsg = "设备打开失败";
            break;
        case QSerialPort::WriteError:
            errorMsg = "数据写入失败";
            break;
        case QSerialPort::ReadError:
            errorMsg = "数据读取失败";
            break;
        case QSerialPort::ResourceError:
            errorMsg = "设备资源不可用，设备可能被意外移除";
            break;
        case QSerialPort::UnsupportedOperationError:
            errorMsg = "不支持的操作";
            break;
        case QSerialPort::TimeoutError:
            errorMsg = "操作超时";
            break;
        default:
            errorMsg = QString("未知错误 (错误代码: %1)").arg(error);
            break;
    }

    QMessageBox::critical(this, "串口错误",
                         QString("串口通信发生错误：\n%1\n\n串口将被关闭。").arg(errorMsg));

    // 关闭串口并更新UI状态
    if(serialPort->isOpen()){
        serialPort->close();
    }
    ui->btnOpenPort->setEnabled(true);
    ui->btnClosePort->setEnabled(false);
    ui->btnSend->setEnabled(false);
}

void MainWindow::onAutoSendTimeout(){
    if(!autoSendData.isEmpty() && serialPort && serialPort->isOpen()){
        sendMsg(autoSendData);
    } else if(autoSendData.isEmpty()){
        // 如果数据为空，停止自动发送
        autoSendTimer->stop();
        ui->checkBox_autoSend->setChecked(false);
        showStatusMessage("自动发送已停止：发送数据为空");
    }
}

// =====================================================================================
// TableWidget右键菜单功能实现
void MainWindow::onTableContextMenu(const QPoint &pos){
    // 获取点击位置的行列
    int row = ui->tableWidget->rowAt(pos.y());
    int col = ui->tableWidget->columnAt(pos.x());

    // 如果点击在有效区域外，不显示菜单
    if(row < 0 || col < 0) return;

    // 设置当前选中的单元格
    ui->tableWidget->setCurrentCell(row, col);

    QMenu contextMenu(this);

    QAction *editAction = contextMenu.addAction("编辑");
    QAction *deleteAction = contextMenu.addAction("删除");

    connect(editAction, &QAction::triggered, this, &MainWindow::onEditButtonData);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteButtonData);

    contextMenu.exec(ui->tableWidget->mapToGlobal(pos));
}

void MainWindow::onEditButtonData(){
    int row = ui->tableWidget->currentRow();
    int col = ui->tableWidget->currentColumn();

    if(row < 0 || col < 0) return;

    // 获取当前按键数据
    ButtonData currentData = buttonDatabase->getButtonData(row, col);

    // 创建编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑按键");
    dialog.setModal(true);
    dialog.resize(400, 200);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 备注输入
    QLabel *remarkLabel = new QLabel("按键备注:");
    QLineEdit *remarkEdit = new QLineEdit(currentData.remark);
    layout->addWidget(remarkLabel);
    layout->addWidget(remarkEdit);

    // 数据类型选择
    QLabel *typeLabel = new QLabel("指令类型:");
    QCheckBox *hexCheckBox = new QCheckBox("16进制指令");
    hexCheckBox->setChecked(currentData.isHexCommand);
    layout->addWidget(typeLabel);
    layout->addWidget(hexCheckBox);

    // 指令输入
    QLabel *commandLabel = new QLabel("按键指令:");
    QLineEdit *commandEdit = new QLineEdit(currentData.command);
    layout->addWidget(commandLabel);
    layout->addWidget(commandEdit);

    // 根据类型更新标签文本
    auto updateCommandLabel = [commandLabel, hexCheckBox]() {
        if (hexCheckBox->isChecked()) {
            commandLabel->setText("按键指令 (16进制):");
        } else {
            commandLabel->setText("按键指令 (字符):");
        }
    };

    connect(hexCheckBox, &QCheckBox::toggled, updateCommandLabel);
    updateCommandLabel(); // 初始化标签

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("确定");
    QPushButton *cancelButton = new QPushButton("取消");
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, [&](){
        QString remark = remarkEdit->text().trimmed();
        QString command = commandEdit->text().trimmed();
        bool isHexCommand = hexCheckBox->isChecked();

        // 验证是否至少填写了备注或指令
        if(remark.isEmpty() && command.isEmpty()){
            QMessageBox::warning(&dialog, "输入错误", "请至少填写备注或指令中的一项！");
            return;
        }

        // 验证指令格式
        if(!command.isEmpty() && isHexCommand && !validateHexInput(QString(command).remove(' '))){
            QMessageBox::warning(&dialog, "格式错误", "请输入有效的十六进制指令！");
            return;
        }

        dialog.accept();
    });
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if(dialog.exec() == QDialog::Accepted){
        QString remark = remarkEdit->text().trimmed();
        QString command = commandEdit->text().trimmed();
        bool isHexCommand = hexCheckBox->isChecked();

        // 保存到数据库
        buttonDatabase->setButtonData(row, col, remark, command, isHexCommand);

        // 更新统计
        updateStatistics();

        // 更新界面显示
        QTableWidgetItem *item = ui->tableWidget->item(row, col);
        if(!item){
            item = new QTableWidgetItem();
            ui->tableWidget->setItem(row, col, item);
        }

        // 显示备注在单元格中
        item->setText(remark);
        item->setTextAlignment(Qt::AlignCenter);

        // 根据是否有指令设置背景色（适配黑色主题）
        if(!command.isEmpty()){
            // 有指令的按键使用浅蓝色背景
            item->setBackground(QBrush(QColor(173, 216, 230))); // 浅蓝色
            item->setForeground(QBrush(QColor(0, 0, 0))); // 黑色文字
        } else {
            // 无指令的按键使用浅绿色背景
            item->setBackground(QBrush(QColor(144, 238, 144))); // 浅绿色
            item->setForeground(QBrush(QColor(0, 0, 0))); // 黑色文字
        }
    }
}

void MainWindow::onDeleteButtonData(){
    int row = ui->tableWidget->currentRow();
    int col = ui->tableWidget->currentColumn();

    if(row < 0 || col < 0) return;

    int ret = QMessageBox::question(this, "确认删除",
                                   "确定要删除这个按键的数据吗?",
                                   QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes){
        // 从数据库删除
        buttonDatabase->removeButtonData(row, col);

        // 更新统计
        updateStatistics();

        // 更新界面
        QTableWidgetItem *item = ui->tableWidget->item(row, col);
        if(item){
            item->setText("");
            // 重置为默认样式
            item->setBackground(QBrush());
            item->setForeground(QBrush());
        }
    }
}

void MainWindow::onOpenSerialPort(){
    if(initSerialPort()){
        ui->btnOpenPort->setEnabled(false);
        ui->btnClosePort->setEnabled(true);
        ui->btnSend->setEnabled(true);
        showStatusMessage("串口已打开");
    }
}

void MainWindow::onCloseSerialPort(){
    if(serialPort->isOpen()){
        serialPort->close();
    }
    ui->btnOpenPort->setEnabled(true);
    ui->btnClosePort->setEnabled(false);
    ui->btnSend->setEnabled(false);
    showStatusMessage("串口已关闭");
}

// =====================================================================================
// TableWidget相关功能实现
void MainWindow::setupTableWidget(){
    // 从数据库获取表格大小
    QPair<int, int> tableSize = buttonDatabase->getTableSize();
    ui->tableWidget->setRowCount(tableSize.first);
    ui->tableWidget->setColumnCount(tableSize.second);

    // 禁用双击编辑
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置右键菜单
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::onTableContextMenu);

    // 设置表头
    QStringList headers;
    for(int i = 0; i < ui->tableWidget->columnCount(); i++){
        headers << QString("按键%1").arg(i+1);
    }
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setVisible(true);

    // 隐藏水平滚动条
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置自适应列宽
    adjustTableColumnWidths();

    // 加载保存的按键数据
    loadButtonsFromDatabase();
}

void MainWindow::adjustTableColumnWidths(){
    if(!ui->tableWidget){
        return;
    }

    int columnCount = ui->tableWidget->columnCount();
    if(columnCount == 0){
        return;
    }

    // 获取表格的总宽度
    int totalWidth = ui->tableWidget->width();

    // 减去边距和滚动条等占用的空间
    // 垂直滚动条宽度约20px，边距约10px，表格边框约2px
    int reservedWidth = 32;

    // 计算可用宽度
    int availableWidth = totalWidth - reservedWidth;

    // 计算平均列宽
    int columnWidth = availableWidth / columnCount;

    // 确保列宽不会太小
    if(columnWidth < 80){
        columnWidth = 80;
    }

    // 设置每列宽度
    for(int i = 0; i < columnCount; i++){
        ui->tableWidget->setColumnWidth(i, columnWidth);
    }
}

void MainWindow::loadButtonsFromDatabase(){
    QMap<QString, ButtonData> allButtons = buttonDatabase->getAllButtons();

    for(auto it = allButtons.begin(); it != allButtons.end(); ++it){
        const ButtonData &data = it.value();

        if(data.isValid && data.row >= 0 && data.col >= 0 &&
           data.row < ui->tableWidget->rowCount() && data.col < ui->tableWidget->columnCount()){

            QTableWidgetItem *item = ui->tableWidget->item(data.row, data.col);
            if(!item){
                item = new QTableWidgetItem();
                ui->tableWidget->setItem(data.row, data.col, item);
            }

            // 显示备注
            item->setText(data.remark);
            item->setTextAlignment(Qt::AlignCenter);

            // 根据是否有指令设置背景色（适配黑色主题）
            if(!data.command.isEmpty()){
                // 有指令的按键使用浅蓝色背景
                item->setBackground(QBrush(QColor(173, 216, 230))); // 浅蓝色
                item->setForeground(QBrush(QColor(0, 0, 0))); // 黑色文字
            } else {
                // 无指令的按键使用浅绿色背景
                item->setBackground(QBrush(QColor(144, 238, 144))); // 浅绿色
                item->setForeground(QBrush(QColor(0, 0, 0))); // 黑色文字
            }
        }
    }

    // 更新按键数统计
    updateStatistics();
}

void MainWindow::onTableCellClicked(int row, int column){
    // 获取按键数据
    ButtonData data = buttonDatabase->getButtonData(row, column);

    // 如果有关联的指令，立即发送
    if(data.isValid && !data.command.isEmpty()){
        // 检查串口状态
        if(!this->serialPort->isOpen()){
            QMessageBox::warning(this, "警告", "串口未打开！");
            showStatusMessage("串口未打开");
            return;
        }

        // 直接发送，绕过所有中间函数
        QByteArray sendData;
        QString displayCommand;

        if(data.isHexCommand) {
            // 十六进制命令
            QString cleanHex = data.command.trimmed();
            if(!cleanHex.isEmpty() && validateHexInput(QString(cleanHex).remove(' '))) {
                sendData = QByteArray::fromHex(cleanHex.toLatin1());
                displayCommand = cleanHex;
            }
        } else {
            // 文本命令
            QString cleanText = data.command.trimmed();
            if(!cleanText.isEmpty()) {
                sendData = cleanText.toUtf8();
                displayCommand = cleanText;
            }
        }

        // 立即写入串口
        if(!sendData.isEmpty()) {
            qint64 bytesWritten = this->serialPort->write(sendData);
            if(bytesWritten > 0) {
                sendCount += bytesWritten;
                updateStatistics();
                showStatusMessage(QString("按键发送成功：%1 字节 [%2]").arg(bytesWritten).arg(displayCommand));

                // 记录发送日志
                if(!isPauseSendLog) {
                    QString logMsg = data.isHexCommand ? sendData.toHex(' ').toUpper() : displayCommand;
                    QString logEntry = isTimestampDisplay ?
                        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " " + logMsg + "\n" :
                        logMsg + "\n";
                    ui->comLog_1->insertPlainText(logEntry);
                }
            } else {
                showStatusMessage("按键发送失败");
                QMessageBox::warning(this, "错误", QString("按键发送失败！\n错误：%1").arg(this->serialPort->errorString()));
            }
        }
    }
}

void MainWindow::onAddRowClicked(){
    addTableRow();
    saveAllConfigs();
}

void MainWindow::onAddColumnClicked(){
    addTableColumn();
    saveAllConfigs();
}

void MainWindow::onRemoveRowClicked(){
    removeTableRow();
    saveAllConfigs();
}

void MainWindow::addTableRow(){
    int currentRows = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(currentRows + 1);
}

void MainWindow::addTableColumn(){
    int currentCols = ui->tableWidget->columnCount();
    ui->tableWidget->setColumnCount(currentCols + 1);

    // 设置新列的表头
    QString headerText = QString("按键%1").arg(currentCols + 1);
    ui->tableWidget->setHorizontalHeaderItem(currentCols, new QTableWidgetItem(headerText));

    // 重新调整所有列的宽度
    adjustTableColumnWidths();
}

void MainWindow::removeTableRow(){
    if(!ui->tableWidget || !buttonDatabase){
        return;
    }

    int currentRows = ui->tableWidget->rowCount();
    if(currentRows <= 2){
        // QMessageBox::information(this, "提示", "至少需要保留2行！");
        return;
    }

    // 从最后一行开始检查，如果有使用的按键则停止删除
    bool deletedAnyRow = false;
    for(int row = currentRows - 1; row >= 0; row--){
        bool hasUsedButton = false;

        // 检查这一行是否有使用的按键
        int colCount = ui->tableWidget->columnCount();
        for(int col = 0; col < colCount; col++){
            ButtonData data = buttonDatabase->getButtonData(row, col);
            if(data.isValid && (!data.remark.isEmpty() || !data.command.isEmpty())){
                hasUsedButton = true;
                break;
            }
        }

        if(hasUsedButton){
            if(row == currentRows - 1 && !deletedAnyRow){
                QMessageBox::information(this, "提示", "最后一行有使用的按键，无法删除！");
            } else if(deletedAnyRow){
                QMessageBox::information(this, "提示",
                    QString("第%1行有使用的按键，已删除到此行为止。").arg(row + 1));
            }
            break;
        } else {
            // 这一行没有使用的按键，可以删除
            // 先删除数据库中这一行的所有按键数据
            for(int col = 0; col < colCount; col++){
                buttonDatabase->removeButtonData(row, col);
            }

            // 删除表格行
            ui->tableWidget->removeRow(row);
            currentRows--;
            deletedAnyRow = true;

            // 只删除一行就停止
            break;
        }
    }

    if(deletedAnyRow){
        updateStatistics();
        showStatusMessage("已删除一行");
    }
}

void MainWindow::saveAllConfigs(){
    // 保存串口配置
    SerialPortConfig config;
    config.portName = ui->portName->currentText();
    config.baudRate = ui->baudRate->currentText().toInt();
    config.dataBits = ui->dataBits->currentText().toInt();
    config.stopBits = ui->stopBits->currentText().toInt();
    config.parity = ui->parity->currentText();
    config.timestampDisplay = ui->checkBox_1->isChecked();
    config.hexDisplay = ui->checkBox_2->isChecked();
    config.hexSend = ui->checkBox_5->isChecked();
    config.autoSendEnter = ui->checkBox_4->isChecked();
    config.enterChars = ui->lineEdit->text();
    // 移除编码配置，固定使用UTF-8

    buttonDatabase->setSerialConfig(config);

    // 保存表格大小
    buttonDatabase->setTableSize(ui->tableWidget->rowCount(), ui->tableWidget->columnCount());

    // 窗口几何信息保存已移除
}

void MainWindow::loadAllConfigs(){
    // 加载串口配置
    SerialPortConfig config = buttonDatabase->getSerialConfig();
    applySerialPortConfig(config);

    // 窗口几何信息加载已移除
}

void MainWindow::applySerialPortConfig(const SerialPortConfig &config){
    // 应用串口参数
    if (!config.portName.isEmpty()) {
        int index = ui->portName->findText(config.portName);
        if (index >= 0) {
            ui->portName->setCurrentIndex(index);
        }
    }

    ui->baudRate->setCurrentText(QString::number(config.baudRate));
    ui->dataBits->setCurrentText(QString::number(config.dataBits));
    ui->stopBits->setCurrentText(QString::number(config.stopBits));
    ui->parity->setCurrentText(config.parity);

    // 应用显示选项
    ui->checkBox_1->setChecked(config.timestampDisplay);
    ui->checkBox_2->setChecked(config.hexDisplay);
    ui->checkBox_5->setChecked(config.hexSend);
    ui->checkBox_4->setChecked(config.autoSendEnter);
    ui->lineEdit->setText(config.enterChars);

    // 移除编码选择设置，固定使用UTF-8

    // 更新内部状态
    isTimestampDisplay = config.timestampDisplay;
    isHexDisplay = config.hexDisplay;
}

void MainWindow::updateStatistics(){
    ui->label_6->setText(QString("发送数：%1").arg(sendCount));
    ui->label_7->setText(QString("接收数：%1").arg(receiveCount));

    // 统计自定义按键数
    int buttonCount = 0;
    QMap<QString, ButtonData> allButtons = buttonDatabase->getAllButtons();
    for(auto it = allButtons.begin(); it != allButtons.end(); ++it){
        if(it.value().isValid && (!it.value().remark.isEmpty() || !it.value().command.isEmpty())){
            buttonCount++;
        }
    }
    ui->label_8->setText(QString("按键数：%1").arg(buttonCount));
}

bool MainWindow::validateHexInput(const QString &input){
    if(input.isEmpty()) return false;

    QString cleanInput = input;
    cleanInput.remove(' ').remove('\n').remove('\r').remove('\t');

    // 检查是否只包含十六进制字符
    QRegularExpression hexPattern("^[0-9A-Fa-f]+$");
    if(!hexPattern.match(cleanInput).hasMatch()){
        return false;
    }

    // 检查长度是否为偶数（每个字节需要2个十六进制字符）
    return (cleanInput.length() % 2 == 0);
}

void MainWindow::showStatusMessage(const QString &message, int timeout){
    // 如果有状态栏，显示消息
    if(statusBar()){
        statusBar()->showMessage(message, timeout);
    }
}

// =====================================================================================
// 日志管理功能实现
void MainWindow::onClearSendLogClicked(){
    ui->comLog_1->clear();
    sendCount = 0;
    ui->label_6->setText("发送数：0");
}

void MainWindow::onSaveSendLogClicked(){
    QString fileName = QFileDialog::getSaveFileName(this,
        "保存发送日志",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/send_log.txt",
        "文本文件 (*.txt)");

    if(!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QTextStream out(&file);
            out << ui->comLog_1->toPlainText();
            file.close();
            QMessageBox::information(this, "提示", "发送日志保存成功！");
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件！");
        }
    }
}

void MainWindow::onPauseSendLogClicked(){
    isPauseSendLog = !isPauseSendLog;
    if(isPauseSendLog){
        ui->pushButton_3->setText("继续显示");
    } else {
        ui->pushButton_3->setText("暂停显示");
    }
}

void MainWindow::onClearReceiveLogClicked(){
    ui->comLog_2->clear();
    receiveCount = 0;
    ui->label_7->setText("接收数：0");
}

void MainWindow::onSaveReceiveLogClicked(){
    QString fileName = QFileDialog::getSaveFileName(this,
        "保存接收日志",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/receive_log.txt",
        "文本文件 (*.txt)");

    if(!fileName.isEmpty()){
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QTextStream out(&file);
            out << ui->comLog_2->toPlainText();
            file.close();
            QMessageBox::information(this, "提示", "接收日志保存成功！");
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件！");
        }
    }
}

void MainWindow::onPauseReceiveLogClicked(){
    isPauseReceiveLog = !isPauseReceiveLog;
    if(isPauseReceiveLog){
        ui->pushButton_6->setText("继续显示");
    } else {
        ui->pushButton_6->setText("暂停显示");
    }
}

void MainWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);

    // 延迟调整列宽，确保UI已经完全更新
    QTimer::singleShot(10, this, [this](){
        adjustTableColumnWidths();
    });
}

// 编码处理函数已删除，统一使用UTF-8

// =====================================================================================
// 按键立即发送机制已集成到onTableCellClicked函数中
