import socket
import threading
import time
import re
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# --- 網路設定 ---
# 監聽所有介面，Port 必須跟 STM32 C 語言裡面的 RemotePORT 一致
HOST = '0.0.0.0' 
PORT = 8002

# --- 資料儲存 (設定畫面上最多顯示 100 筆資料) ---
MAX_LEN = 100
x_data = deque([0]*MAX_LEN, maxlen=MAX_LEN)
y_data = deque([0]*MAX_LEN, maxlen=MAX_LEN)
z_data = deque([0]*MAX_LEN, maxlen=MAX_LEN)

# 震動事件旗標
motion_event_flag = False

def tcp_server():
    global motion_event_flag
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print(f"[*] 等待 STM32 連線 (Port: {PORT})...")
        
        conn, addr = s.accept()
        with conn:
            print(f"[+] STM32 已連線！來源 IP: {addr}")
            # ❌ 刪掉這行，不再主動觸發
            # conn.sendall(b"a")
            
            buffer = ""
            while True:
                try:
                    data = conn.recv(1024)
                    if not data:
                        break
                    
                    buffer += data.decode('utf-8', errors='ignore')
                    
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        
                        if "EVENT" in line:
                            print("⚠️ 偵測到劇烈晃動！")
                            motion_event_flag = True
                            
                        elif "ACC:" in line:
                            match = re.search(r"X=(-?\d+),\s*Y=(-?\d+),\s*Z=(-?\d+)", line)
                            if match:
                                x_data.append(int(match.group(1)))
                                y_data.append(int(match.group(2)))
                                z_data.append(int(match.group(3)))
                            # ❌ 刪掉這行，不再一問一答
                            # conn.sendall(b"a")
                                
                except Exception as e:
                    print(f"發生錯誤: {e}")
                    break

# 啟動 TCP Server 執行緒 (放在背景跑，才不會卡住畫圖畫面)
thread = threading.Thread(target=tcp_server, daemon=True)
thread.start()

# --- GUI 繪圖設定 ---
fig, ax = plt.subplots(figsize=(10, 6))
fig.canvas.manager.set_window_title('STM32 即時感測器監控系統')

# 設定三條折線
line_x, = ax.plot(x_data, label='X Axis', color='#ff4c4c', linewidth=2)
line_y, = ax.plot(y_data, label='Y Axis', color='#4caf50', linewidth=2)
line_z, = ax.plot(z_data, label='Z Axis', color='#2196f3', linewidth=2)

# 設定圖表外觀
ax.set_ylim(-2000, 2000) # 加速度計的數值範圍，可依實際情況調整
ax.set_title("STM32 LSM6DSL Real-time Accelerometer", fontsize=14)
ax.set_xlabel("Time (frames)")
ax.set_ylabel("Acceleration (Raw Data)")
ax.legend(loc="upper right")
ax.grid(True, linestyle='--', alpha=0.6)

# --- 動態更新圖表的函式 ---
def update_plot(frame):
    global motion_event_flag
    
    # 更新折線資料
    line_x.set_ydata(x_data)
    line_y.set_ydata(y_data)
    line_z.set_ydata(z_data)
    
    # 如果觸發警告，將背景閃爍成紅色！
    if motion_event_flag:
        ax.set_facecolor('#ffe6e6') # 淺紅色警告背景
        motion_event_flag = False   # 閃爍一次後關閉
    else:
        ax.set_facecolor('#ffffff') # 恢復白色背景
        
    return line_x, line_y, line_z

# 啟動動畫 (每 50 毫秒更新一次畫面)
ani = animation.FuncAnimation(fig, update_plot, interval=50, blit=False, cache_frame_data=False)

print("開始繪製圖表...")
plt.show()