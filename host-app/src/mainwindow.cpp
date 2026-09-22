// 主窗口实现
#include "mainwindow.h"
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QSerialPortInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("独居老人居家监护 - 上位机看板");

    m_modbus = new ModbusMaster(this);
    m_onenet = new OneNetClient(this);
    m_dash   = new Dashboard(this);
    m_ota    = new OtaPanel(this);

    // 左看板右OTA
    auto *split = new QSplitter(Qt::Horizontal, this);
    split->addWidget(m_dash);
    split->addWidget(m_ota);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    setCentralWidget(split);

    buildToolbar();
    statusBar()->showMessage("未连接");

    // 数据源 -> 看板
    connect(m_modbus, &ModbusMaster::dataUpdated, this, &MainWindow::onData);
    connect(m_onenet, &OneNetClient::dataUpdated, this, &MainWindow::onData);
    connect(m_modbus, &ModbusMaster::errorOccurred, this, &MainWindow::onError);
    connect(m_onenet, &OneNetClient::errorOccurred, this, &MainWindow::onError);

    // 控制按钮
    connect(m_btnClearAlarm, &QPushButton::clicked, m_modbus, &ModbusMaster::clearAlarm);
    connect(m_btnSos, &QPushButton::clicked, m_modbus, &ModbusMaster::triggerSos);
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectSerial);
}

void MainWindow::buildToolbar()
{
    auto *tb = addToolBar("main");
    tb->addWidget(new QLabel(" 串口: "));
    m_cmbPort = new QComboBox;
    for (const auto &info : QSerialPortInfo::availablePorts())
        m_cmbPort->addItem(info.portName());
    m_cmbPort->setMinimumWidth(120);
    tb->addWidget(m_cmbPort);

    tb->addWidget(new QLabel(" 波特率: "));
    m_editBaud = new QLineEdit("9600");
    m_editBaud->setFixedWidth(70);
    tb->addWidget(m_editBaud);

    tb->addWidget(new QLabel(" 从站: "));
    m_cmbSlave = new QComboBox;
    for (int i = 1; i <= 16; i++) m_cmbSlave->addItem(QString::number(i));
    tb->addWidget(m_cmbSlave);

    m_btnConnect = new QPushButton("连接RS485");
    tb->addWidget(m_btnConnect);
    tb->addSeparator();

    m_btnClearAlarm = new QPushButton("解除报警");
    m_btnSos = new QPushButton("SOS测试");
    tb->addWidget(m_btnClearAlarm);
    tb->addWidget(m_btnSos);
}

void MainWindow::onConnectSerial()
{
    if (m_cmbPort->currentText().isEmpty()) {
        QMessageBox::warning(this, "RS485", "请选择串口");
        return;
    }
    int baud = m_editBaud->text().toInt();
    m_modbus->setSlaveId(m_cmbSlave->currentText().toInt());
    if (m_modbus->connectPort(m_cmbPort->currentText(), baud)) {
        m_modbus->startPolling(1000);
        statusBar()->showMessage("RS485 已连接，轮询中...");
        m_btnConnect->setText("已连接");
    } else {
        statusBar()->showMessage("RS485 连接失败");
    }
}

void MainWindow::onData(const GuardianFrame &f)
{
    m_dash->updateData(f);
    m_ota->setDeviceVersion(f.fwVersion);
    m_status = nullptr;   // 预留
}

void MainWindow::onError(const QString &msg)
{
    statusBar()->showMessage("错误: " + msg, 3000);
}
