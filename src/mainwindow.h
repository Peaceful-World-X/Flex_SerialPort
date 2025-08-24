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
    QByteArray encodeText(const QString &text, const QString &encoding);
    QString decodeText(const QByteArray &data, const QString &encoding);
    QString autoDetectAndDecode(const QByteArray &data);
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
};

#endif // MAINWINDOW_H
