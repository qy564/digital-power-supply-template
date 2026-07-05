import serial
import time
import matplotlib.pyplot as plt
from collections import deque
import argparse

"""
数字电源串口调参工具

用法:
    python serial_monitor.py --port COM3 --baud 115200

功能:
    - 实时显示输出电压、电流、占空比
    - 串口绘图(实时波形)
    - 发送调参指令

串口协议格式(MCU发送):
    Vout: %.2f, Iout: %.2f, Duty: %.3f

调参指令(PC发送):
    KP <value>  : 设置 Kp
    KI <value>  : 设置 Ki
    VREF <value>: 设置目标电压
    RESET       : 重置积分器
    DUMP        : 打印当前参数
"""

class PowerSupplyMonitor:
    def __init__(self, port, baudrate=115200, plot=True):
        self.serial = serial.Serial(port, baudrate, timeout=0.1)
        self.plot_enabled = plot
        
        # 数据缓冲区
        self.max_points = 500
        self.time_data = deque(maxlen=self.max_points)
        self.vout_data = deque(maxlen=self.max_points)
        self.iout_data = deque(maxlen=self.max_points)
        self.duty_data = deque(maxlen=self.max_points)
        
        self.start_time = time.time()
        
        # 实时参数
        self.current_params = {
            'vout': 0.0,
            'iout': 0.0,
            'duty': 0.0
        }
        
        if self.plot_enabled:
            self._setup_plot()
    
    def _setup_plot(self):
        """初始化实时图表"""
        plt.ion()
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(10, 6))
        self.fig.canvas.manager.set_window_title('数字电源调试面板')
        
        self.line_vout, = self.ax1.plot([], [], 'b-', label='Vout (V)')
        self.line_duty, = self.ax1.plot([], [], 'r-', label='Duty (x10)')
        self.ax1.set_ylabel('电压')
        self.ax1.legend()
        self.ax1.grid(True)
        
        self.line_iout, = self.ax2.plot([], [], 'g-', label='Iout (A)')
        self.ax2.set_xlabel('时间(s)')
        self.ax2.set_ylabel('电流')
        self.ax2.legend()
        self.ax2.grid(True)
    
    def parse_line(self, line):
        """解析 MCU 串口数据"""
        try:
            import re
            # 匹配格式: Vout: 12.05, Iout: 1.23, Duty: 0.456
            match = re.search(
                r'Vout:\s*([\d.]+).*?Iout:\s*([\d.]+).*?Duty:\s*([\d.]+)',
                line
            )
            if match:
                self.current_params['vout'] = float(match.group(1))
                self.current_params['iout'] = float(match.group(2))
                self.current_params['duty'] = float(match.group(3))
                return True
        except:
            pass
        return False
    
    def update_plot(self):
        """更新实时图表"""
        elapsed = time.time() - self.start_time
        self.time_data.append(elapsed)
        self.vout_data.append(self.current_params['vout'])
        self.iout_data.append(self.current_params['iout'])
        self.duty_data.append(self.current_params['duty'] * 10)  # 放大显示
        
        if not self.plot_enabled:
            return
        
        self.line_vout.set_data(list(self.time_data), list(self.vout_data))
        self.line_duty.set_data(list(self.time_data), list(self.duty_data))
        self.line_iout.set_data(list(self.time_data), list(self.iout_data))
        
        # 自动调整坐标轴
        for ax in [self.ax1, self.ax2]:
            ax.relim()
            ax.autoscale_view()
        
        plt.pause(0.01)
    
    def send_command(self, cmd):
        """发送调参指令到 MCU"""
        self.serial.write((cmd + '\r\n').encode())
        print(f"[发送] {cmd}")
    
    def run(self):
        """主循环"""
        print(f"连接成功: {self.serial.port} @ {self.serial.baudrate}")
        print("=" * 50)
        print('指令帮助:')
        print('  KP 0.5    -> 设置比例系数')
        print('  KI 0.01   -> 设置积分系数')
        print('  VREF 12.0 -> 设置目标电压')
        print('  RESET     -> 重置积分器')
        print('  DUMP      -> 打印当前参数')
        print('  Q         -> 退出')
        print("=" * 50)
        
        try:
            import msvcrt
            
            while True:
                # 读取串口数据
                if self.serial.in_waiting:
                    line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        if self.parse_line(line):
                            self.update_plot()
                            # 终端显示
                            p = self.current_params
                            print(f"\rVout={p['vout']:.2f}V  Iout={p['iout']:.2f}A  Duty={p['duty']:.3f}  ", end='')
                
                # 检查键盘输入
                if msvcrt.kbhit():
                    cmd = msvcrt.getche().decode()
                    if cmd == 'Q' or cmd == 'q':
                        break
                
                time.sleep(0.01)
        except KeyboardInterrupt:
            pass
        finally:
            self.serial.close()
            if self.plot_enabled:
                plt.ioff()
                plt.close()
            print("\n已断开连接")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='数字电源串口调参工具')
    parser.add_argument('--port', '-p', default='COM3', help='串口号')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='波特率')
    parser.add_argument('--no-plot', action='store_true', help='禁用实时图表')
    
    args = parser.parse_args()
    
    monitor = PowerSupplyMonitor(args.port, args.baud, not args.no_plot)
    monitor.run()
