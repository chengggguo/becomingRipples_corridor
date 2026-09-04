# 190cmBar Device — Status LED Version

## 文件

`190cmBar_device_status_led.ino` 是装置侧 Arduino Uno 的正式联动代码，接收外部 RUN 和 RESET 信号，控制两台步进电机、舵机、左右 Hall 传感器，并通过 D13 显示状态。传感器控制器使用 Nano，两块 Arduino 的引脚编号不要混淆。

## 接线

装置端的 RUN 和 RESET 都是低电平有效，代码使用 `INPUT_PULLUP`：

```text
装置 Arduino A1 = RUN
装置 Arduino A2 = RESET / AUTOHOME
```

继电器触点接法：

```text
RUN 继电器 NO  -> 装置 Arduino A1
RUN 继电器 COM -> 装置 Arduino GND

RESET 继电器 NO  -> 装置 Arduino A2
RESET 继电器 COM -> 装置 Arduino GND
```

继电器吸合时，会把对应输入引脚接到装置自身的 GND。同一台装置的 RUN、RESET 两路 COM 可以互联并共用该装置 GND，NC 不接。

当前其他相关引脚：

```text
A1        = RUN 数字输入（LOW 有效，INPUT_PULLUP）
A2        = RESET 数字输入（LOW 有效，INPUT_PULLUP）
D5 / D4   = 左步进电机 STEP / DIR
D6 / D7   = 右步进电机 STEP / DIR
D8 / D9   = 左右驱动器 ENABLE
D10       = 舵机
D11 / D12 = 左右 Hall 传感器
D13       = 状态灯
```

A1、A2 在 Arduino Uno 上可以作为数字输入使用。D2、D3 在这个正式状态灯版本中不再用于 RUN/RESET；D11、D12、D13 已分别用于 Hall 和状态灯。

## 运行逻辑

1. 开机后先执行 AUTOHOME，通过 D11、D12 两个 Hall 信号让左右两边分别到头并建立机械原点。
2. 两边归零完成后，装置立即移动到绘画范围内的一个随机待机位置；不会停在原点等待。
3. 到达随机待机位置后才进入 IDLE，等待外部 RUN 信号。
4. A1 被继电器接地后，装置立即进入 RUN，舵机就在当前随机待机位置先摆动一次。
5. RUN 持续期间，每轮先随机等待 1–10 秒，然后移动到绘画范围内的另一个随机位置；如果 RUN 仍有效，舵机摆动一次，再开始下一轮。
6. A1 断开、恢复 HIGH 后，当前正在执行的动作不会被强行中断。该轮结束后退出 RUN，再移动到一个新的随机待机位置。
7. A2 被继电器接地时提出 RESET 请求。IDLE 时立即执行 AUTOHOME；如果正在 RUN，则先记录请求，待 RUN 结束后再执行。每次 AUTOHOME 完成后，也会先移动到新的随机待机位置，再进入 IDLE。

## D13 状态

```text
熄灭 = IDLE
常亮 = RUN
快闪 = 正在 AUTOHOME / RESET
```
