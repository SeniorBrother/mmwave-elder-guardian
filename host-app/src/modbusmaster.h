// Modbus RTU 主站封装（Qt SerialBus）—— 经 USB-RS485 轮询设备寄存器
#ifndef MODBUSMASTER_H
#define MODBUSMASTER_H

#include <QObject>
#include <QModbusRtuSerialMaster>
#include <QModbusDataUnit>
#include <QTimer>

// 一次轮询回传的完整监测数据（与固件寄存器映射对应）
struct GuardianFrame {
    int  heartRate   = 0;
    int  respRate    = 0;
    int  people      = 0;
    int  fall        = 0;
    float bodyTemp   = 0.0f;   // 寄存器 x10 还原
    float envTemp    = 0.0f;   // 寄存器 x10 还原
    int  envHumi     = 0;
    int  co          = 0;
    int  alarmStatus = 0;
    int  fwVersion   = 0;
};

class ModbusMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMaster(QObject *parent = nullptr);
    ~ModbusMaster();

    // 打开串口，如 portName="COM5" 或 "/dev/ttyUSB0"
    bool connectPort(const QString &portName, int baud = 9600);
    void disconnectPort();
    void setSlaveId(int id);
    void startPolling(int intervalMs = 1000);
    void stopPolling();

public slots:
    void clearAlarm();      // 写线圈0：远程解除报警
    void triggerSos();      // 写线圈1：触发SOS测试

signals:
    void dataUpdated(const GuardianFrame &frame);
    void errorOccurred(const QString &msg);
    void connectedChanged(bool online);

private slots:
    void onPollTimeout();
    void onReadReady();

private:
    void pollInputRegisters();

    QModbusRtuSerialMaster *m_modbus = nullptr;
    QModbusReply *m_reply = nullptr;
    QTimer m_timer;
    int m_slaveId = 1;
};

#endif // MODBUSMASTER_H
