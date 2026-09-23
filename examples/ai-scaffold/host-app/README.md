# host-app —— QT5 PC 上位机看板

Qt5 桌面看板，双数据源（RS485 本地 Modbus + OneNet 云端），实时曲线、报警弹窗、远程解除报警、OTA 面板。

## 依赖

- Qt 5.12+（组件：Widgets / Charts / Network / SerialBus / SerialPort）
- CMake 3.16+
- Qt SerialBus 自带 Modbus RTU 主站（`QModbusRtuSerialMaster`），无需第三方 libmodbus。

## 构建

```bash
cd host-app
mkdir build && cd build
cmake ..            # Windows 需指定 Qt5 路径: -DCMAKE_PREFIX_PATH=/path/to/Qt5.x
cmake --build .
```

或用 Qt Creator 直接打开 `CMakeLists.txt`。

## 结构

```
src/
├── main.cpp            入口
├── mainwindow.*        主窗口：工具栏(串口/波特率/从站/连接/解除报警/SOS) + 看板 + OTA 面板
├── modbusmaster.*      Modbus RTU 主站：轮询输入寄存器、写线圈(解除报警/SOS)
├── onenetclient.*      OneNet RESTful API 轮询数据点，转成统一 GuardianFrame
├── dashboard.*         QtCharts 曲线 + 数字卡片 + 报警条
└── otapanel.*          选择 bin、填产品ID/设备名、下发升级、进度与版本显示
```

## 数据流

- `ModbusMaster` 经 USB-RS485 读设备输入寄存器（映射见 `docs/modbus-map.md`），
  解析出心率/呼吸/体温等，发 `dataUpdated(GuardianFrame)`。
- `OneNetClient` 从 OneNet 云端拉数据点，发同样的 `GuardianFrame`。
- 两路数据都汇入 `Dashboard::updateData()` 统一显示；报警位图非零则变红 + 蜂鸣。
- 工具栏「解除报警/SOS」通过 `ModbusMaster` 写线圈远程控制设备。

## 使用

1. 设备经 RS485 接到电脑 USB-RS485，选串口/波特率(默认9600)/从站地址(默认1)，点「连接RS485」。
2. 云端可选：在 `onenetclient` 配置产品ID/设备名/apiKey 后 `start()`。
3. OTA：切到 OTA 面板，选 bin、填产品ID/设备名，点上传下发（需对接 OneNet FOTA REST API）。

> 说明：`onenetclient.cpp` 的 API 地址与返回结构按 OneNet 实际版本微调；
> `otapanel` 的 `requestUpload` 信号处理需接 OneNet FOTA 上传接口。
