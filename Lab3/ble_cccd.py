from bluepy.btle import Peripheral, UUID, DefaultDelegate, Scanner
import time

# 處理 Notification/Indication 的 class
class NotifyDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        print(f"收到通知！Handle: {cHandle}, 資料: {data}")

# 掃描裝置
print("掃描中...")
scanner = Scanner()
devices = scanner.scan(10.0)

n = 0
addr = []
for dev in devices:
    print(f"{n}: Device {dev.addr} ({dev.addrType}), RSSI={dev.rssi} dB")
    for (adtype, desc, value) in dev.getScanData():
        print(f"  {desc} = {value}")
    addr.append(dev.addr)
    n += 1

# 選擇裝置
number = int(input("輸入裝置編號: "))
print(f"連線到: {addr[number]}")

# 連線
dev = Peripheral(addr[number], 'random')
dev.setDelegate(NotifyDelegate())

try:
    # 列出所有 Service
    print("\n=== Services ===")
    for svc in dev.services:
        print(f"Service: {svc.uuid}")

    # 找到 fff0 service
    testService = dev.getServiceByUUID(UUID(0xfff0))

    # 列出所有 Characteristic
    print("\n=== Characteristics ===")
    for ch in testService.getCharacteristics():
        print(f"Char UUID: {ch.uuid}, Properties: {ch.propertiesToString()}")

    # 找到 fff1
    ch = testService.getCharacteristics(UUID(0xfff1))[0]
    print(f"\nfff1 Properties: {ch.propertiesToString()}")

    # 讀取值
    if ch.supportsRead():
        print(f"讀取值: {ch.read()}")

    # ✅ 正確方式：透過 descriptor UUID 找 CCCD handle
    cccd_uuid = UUID(0x2902)
    descs = ch.getDescriptors(forUUID=cccd_uuid)
    if descs:
        cccd_handle = descs[0].handle
        print(f"\nCCCD handle: {cccd_handle}")
        dev.writeCharacteristic(cccd_handle, b'\x02\x00', withResponse=True)
        print("CCCD 設定成功！值 = 0x0002 (Indication 已啟用)")
    else:
        print("找不到 CCCD descriptor！")
        raise Exception("No CCCD found")

    # 等待 Indication
    print("\n等待 Indication（100秒）...")
    timeout = time.time() + 100
    while time.time() < timeout:
        if dev.waitForNotifications(1.0):
            print("收到 Indication！")
            break
        print("等待中...")

finally:
    dev.disconnect()
    print("已斷線")
