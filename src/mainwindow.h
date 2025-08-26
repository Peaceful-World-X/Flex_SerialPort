#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QStatusBar>
#include <QTextCursor>
#include <QPoint>
#include <QQueue>
#include "configmanager.h"
#include "buttondatabase.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void findFreePorts();
    void refreshPorts();
    bool initSerialPort();
    void sendMsg(const QString &msg);
    void sendHexCommand(const QString &hexCommand);
    void sendTextCommand(const QString &textCommand);
    void sendDataToPort(const QByteArray &data, const QString &displayText, bool isHex);
    void setupTableWidget();
    void adjustTableColumnWidths();
    void loadButtonsFromDatabase();
    void addTableRow();
    void addTableColumn();
    void removeTableRow();
    void saveAllConfigs();
    void loadAllConfigs();
    void applySerialPortConfig(const SerialPortConfig &config);
    void updateStatistics();
    bool validateHexInput(const QString &input);
    void showStatusMessage(const QString &message, int timeout = 3000);
    void parseAndApplyQuickConfig(const QString &configText);
    // 编码处理方法已删除，统一使用UTF-8
    void processReceiveBuffer();
    void displayCompleteMessage(const QByteArray &message);
    bool eventFilter(QObject *obj, QEvent *event);
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void recvMsg();
    void onSerialError(QSerialPort::SerialPortError error);
    void onTableCellClicked(int row, int column);
    void onAddRowClicked();
    void onAddColumnClicked();
    void onRemoveRowClicked();
    void onClearSendLogClicked();
    void onSaveSendLogClicked();
    void onPauseSendLogClicked();
    void onClearReceiveLogClicked();
    void onSaveReceiveLogClicked();
    void onPauseReceiveLogClicked();
    void onAutoSendTimeout();
    void onTableContextMenu(const QPoint &pos);
    void onEditButtonData();
    void onDeleteButtonData();
    void onOpenSerialPort();
    void onCloseSerialPort();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    ConfigManager *configManager;
    ButtonDatabase *buttonDatabase;

    // 数据统计
    int sendCount;
    int receiveCount;

    // 显示控制
    bool isPauseSendLog;
    bool isPauseReceiveLog;
    bool isHexDisplay;
    bool isTimestampDisplay;

    // 自动发送
    QTimer *autoSendTimer;
    QString autoSendData;

    // 接收数据缓存
    QByteArray receiveBuffer;

    // 发送队列和防抖机制
    struct SendRequest {
        QString command;
        bool isHexCommand;
        QString displayText;

        SendRequest(const QString &cmd, bool isHex, const QString &display = "")
            : command(cmd), isHexCommand(isHex), displayText(display) {}
    };

    QQueue<SendRequest> sendQueue;
    QTimer *sendQueueTimer;
    QTimer *debounceTimer;
    static const int SEND_INTERVAL_MS = 50;  // 发送间隔50ms
    static const int DEBOUNCE_DELAY_MS = 10; // 防抖延迟10ms

    void processSendQueue();
    void enqueueSendRequest(const QString &command, bool isHexCommand, const QString &displayText = "");
};

#endif // MAINWINDOW_H
