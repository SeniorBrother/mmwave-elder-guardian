#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Modbus RTU 主站测试脚本 —— 经 USB-RS485 轮询监护设备，验证固件从站栈。

依赖: pip install pyserial

用法:
  python modbus_test.py COM5            # Windows
  python modbus_test.py /dev/ttyUSB0     # Linux/WSL
  python modbus_test.py COM5 --slave 1 --baud 9600

读输入寄存器 0x04 起始0 数量10，解析出心率/呼吸/体温等，并演示写线圈解除报警。
"""
import sys
import time
import argparse

try:
    import serial
except ImportError:
    print("请先安装 pyserial:  pip install pyserial")
    sys.exit(1)


def crc16(data: bytes) -> int:
    """Modbus CRC16, 多项式 0xA001"""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def build_read_input(slave, start, count):
    pdu = bytes([slave, 0x04, (start >> 8) & 0xFF, start & 0xFF,
                 (count >> 8) & 0xFF, count & 0xFF])
    crc = crc16(pdu)
    return pdu + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def build_write_coil(slave, addr, on):
    val = 0xFF00 if on else 0x0000
    pdu = bytes([slave, 0x05, (addr >> 8) & 0xFF, addr & 0xFF,
                 (val >> 8) & 0xFF, val & 0xFF])
    crc = crc16(pdu)
    return pdu + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def parse_input(resp: bytes):
    """解析 0x04 响应 -> dict"""
    if len(resp) < 5 or resp[1] != 0x04:
        return None
    byte_count = resp[2]
    data = resp[3:3 + byte_count]
    regs = [int.from_bytes(data[i:i + 2], "big") for i in range(0, byte_count, 2)]
    return {
        "心率":      regs[0],
        "呼吸":      regs[1],
        "存在":      "有人" if regs[2] else "无人",
        "摔倒":      regs[3],
        "体温(℃)":  regs[4] / 10.0,
        "室温(℃)":  regs[5] / 10.0,
        "湿度":      regs[6],
        "CO(%)":     regs[7],
        "报警位图":  bin(regs[8]),
        "固件版本":  hex(regs[9]),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("--slave", type=int, default=1)
    ap.add_argument("--baud", type=int, default=9600)
    ap.add_argument("--once", action="store_true", help="只读一次")
    ap.add_argument("--clear-alarm", action="store_true", help="发送解除报警线圈")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.5)
    print(f"打开 {args.port} @ {args.baud}, 从站 {args.slave}")

    if args.clear_alarm:
        ser.write(build_write_coil(args.slave, 0x0000, True))
        time.sleep(0.2)
        ser.reset_input_buffer()
        print("已发送解除报警线圈")

    try:
        while True:
            ser.reset_input_buffer()
            ser.write(build_read_input(args.slave, 0, 10))
            time.sleep(0.15)
            resp = ser.read(64)
            if not resp:
                print("无响应（检查 A/B 接线、从站地址、波特率）")
            else:
                parsed = parse_input(resp)
                if parsed:
                    print(parsed)
                else:
                    print("异常响应:", resp.hex(" "))
            if args.once:
                break
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()


if __name__ == "__main__":
    main()
