// Modbus RTU 主站实现（Qt SerialBus）
#include "modbusmaster.h"
#include <QModbusResponse>

// 输入寄存器地址（与固件 modbus_rtu.h / docs/modbus-map.md 一致）
static const int REG_START = 0x0000;
static const int REG_COUNT = 10;

ModbusMaster::ModbusMaster(QObject *parent) : QObject(parent)
{
    m_modbus = new QModbusRtuSerialMaster(this);
    connect(&m_timer, &QTimer::timeout, this, &ModbusMaster::onPollTimeout);
}

ModbusMaster::~ModbusMaster()
{
    disconnectPort();
}

bool ModbusMaster::connectPort(const QString &portName, int baud)
{
    if (!m_modbus) return false;
    if (m_modbus->state() != QModbusDevice::ConnectedState) {
        m_modbus->setConnectionParameter(
            QModbusDevice::SerialPortNameParameter, portName);
        m_modbus->setConnectionParameter(
            QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
        m_modbus->setConnectionParameter(
            QModbusDevice::SerialBaudRateParameter, baud);
        m_modbus->setConnectionParameter(
            QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_modbus->setConnectionParameter(
            QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
        m_modbus->setTimeout(500);

        if (!m_modbus->connectDevice()) {
            emit errorOccurred(m_modbus->errorString());
            return false;
        }
    }
    emit connectedChanged(true);
    return true;
}

void ModbusMaster::disconnectPort()
{
    m_timer.stop();
    if (m_modbus && m_modbus->state() == QModbusDevice::ConnectedState) {
        m_modbus->disconnectDevice();
        emit connectedChanged(false);
    }
}

void ModbusMaster::setSlaveId(int id) { m_slaveId = id; }

void ModbusMaster::startPolling(int intervalMs)
{
    m_timer.start(intervalMs);
    onPollTimeout();   // 立即拉一次
}

void ModbusMaster::stopPolling() { m_timer.stop(); }

void ModbusMaster::onPollTimeout()
{
    if (!m_modbus || m_modbus->state() != QModbusDevice::ConnectedState) return;
    if (m_reply && m_reply->isRunning()) return;   // 上一次还没回
    pollInputRegisters();
}

void ModbusMaster::pollInputRegisters()
{
    // 读输入寄存器 0x04, 起始0, 数量10
    QModbusDataUnit unit(QModbusDataUnit::InputRegisters, REG_START, REG_COUNT);
    if (auto *reply = m_modbus->sendReadRequest(unit, m_slaveId)) {
        if (reply->isFinished()) {
            reply->deleteLater();
            return;
        }
        m_reply = reply;
        connect(reply, &QModbusReply::finished, this, &ModbusMaster::onReadReady);
    } else {
        emit errorOccurred(m_modbus->errorString());
    }
}

void ModbusMaster::onReadReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        GuardianFrame f;
        // value(i) 对应寄存器 REG_START+i
        if (unit.valueCount() >= 10) {
            f.heartRate   = unit.value(0);
            f.respRate    = unit.value(1);
            f.people      = unit.value(2);
            f.fall        = unit.value(3);
            f.bodyTemp    = unit.value(4) / 10.0f;   // x10 还原
            f.envTemp     = unit.value(5) / 10.0f;
            f.envHumi     = unit.value(6);
            f.co          = unit.value(7);
            f.alarmStatus = unit.value(8);
            f.fwVersion   = unit.value(9);
            emit dataUpdated(f);
        }
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        emit errorOccurred(QString("Modbus 异常响应 0x%1")
                           .arg(reply->rawResult().exceptionCode(), 2, 16, QChar('0')));
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
    m_reply = nullptr;
}

// 写线圈 0x05：ON=0xFF00
static void writeCoil(QModbusRtuSerialMaster *mb, int slaveId, int addr, bool on)
{
    QModbusDataUnit unit(QModbusDataUnit::Coils, addr, 1);
    unit.setValue(0, on ? 0xFF00 : 0x0000);
    if (auto *r = mb->sendWriteRequest(unit, slaveId)) {
        if (!r->isFinished())
            QObject::connect(r, &QModbusReply::finished, r, &QObject::deleteLater);
        else
            r->deleteLater();
    }
}

void ModbusMaster::clearAlarm()
{
    if (m_modbus && m_modbus->state() == QModbusDevice::ConnectedState)
        writeCoil(m_modbus, m_slaveId, 0x0000, true);
}

void ModbusMaster::triggerSos()
{
    if (m_modbus && m_modbus->state() == QModbusDevice::ConnectedState)
        writeCoil(m_modbus, m_slaveId, 0x0001, true);
}
