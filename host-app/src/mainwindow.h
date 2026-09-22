// 主窗口：整合数据源(RS485 Modbus / OneNet) + 看板 + OTA + 控制
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "modbusmaster.h"
#include "onenetclient.h"
#include "dashboard.h"
#include "otapanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnectSerial();
    void onData(const GuardianFrame &f);
    void onError(const QString &msg);

private:
    void buildToolbar();

    ModbusMaster *m_modbus;
    OneNetClient *m_onenet;
    Dashboard    *m_dash;
    OtaPanel     *m_ota;

    QComboBox   *m_cmbPort;
    QLineEdit   *m_editBaud;
    QComboBox   *m_cmbSlave;
    QPushButton *m_btnConnect;
    QPushButton *m_btnClearAlarm;
    QPushButton *m_btnSos;
    QLabel      *m_status;
};

#endif // MAINWINDOW_H
