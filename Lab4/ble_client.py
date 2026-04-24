#!/usr/bin/env python3
import struct
import time
import threading
import queue
from bluepy import btle

ACC_SERVICE_UUID = "00000000-0001-11e1-9ab4-0002a5d5c51b"
ACC_DATA_UUID    = "00e00000-0001-11e1-ac36-0002a5d5c51b"
ACC_FREQ_UUID    = "00e00001-0001-11e1-ac36-0002a5d5c51b"

freq_queue = queue.Queue()

class AccDelegate(btle.DefaultDelegate):
    def __init__(self):
        super().__init__()
        self.count = 0

    def handleNotification(self, cHandle, data):
        if len(data) == 6:
            x, y, z = struct.unpack('<hhh', bytes(data))
            self.count += 1
            print(f"[{self.count:5d}] X={x:6d}  Y={y:6d}  Z={z:6d} mg")

def input_thread():
    time.sleep(1.0)
    print("\n----------------------------------------")
    print("初始頻率: 10 Hz")
    print("可在下方輸入新頻率")
    print("----------------------------------------")
    while True:
        try:
            val = input("\n輸入採樣頻率 (1-100 Hz，q 離開): ")
            if val.lower() == 'q':
                freq_queue.put(None)
                break
            freq = int(val)
            if 1 <= freq <= 100:
                freq_queue.put(freq)
            else:
                print("請輸入 1~100 之間的數字")
        except ValueError:
            print("無效輸入")

def main():
    print("掃描 BLE 裝置（5秒）...")
    scanner = btle.Scanner()
    devices = scanner.scan(5)

    target_addr = None
    for dev in devices:
        for (adtype, desc, value) in dev.getScanData():
            if value == "STM32-ACC":
                print(f"找到 STM32-ACC，MAC: {dev.addr}")
                target_addr = dev.addr
                break

    if target_addr is None:
        target_addr = "d8:94:7e:7b:0b:6a"
        print(f"用固定 MAC: {target_addr}")

    print("連線中...")
    p = btle.Peripheral(target_addr, btle.ADDR_TYPE_RANDOM, iface=0)
    p.setDelegate(AccDelegate())
    print("連線成功！")

    svc = p.getServiceByUUID(ACC_SERVICE_UUID)
    data_char = svc.getCharacteristics(ACC_DATA_UUID)[0]
    freq_char = svc.getCharacteristics(ACC_FREQ_UUID)[0]

    cccd_handle = data_char.getHandle() + 1
    p.writeCharacteristic(cccd_handle, b'\x01\x00', withResponse=True)
    print("Notify 啟用！")

    freq_char.write(struct.pack('<H', 10), withResponse=True)

    # 輸入在背景，寫入在主執行緒
    t = threading.Thread(target=input_thread, daemon=True)
    t.start()

    try:
        while True:
            # 檢查是否有新頻率要寫
            try:
                freq = freq_queue.get_nowait()
                if freq is None:
                    break
                freq_char.write(struct.pack('<H', freq), withResponse=True)
                print(f"已傳送頻率: {freq} Hz")
            except queue.Empty:
                pass

            p.waitForNotifications(0.1)

    except KeyboardInterrupt:
        print("\n中斷，斷線...")
    finally:
        try:
            p.disconnect()
        except:
            pass
        print("已斷線")

if __name__ == "__main__":
    main()
