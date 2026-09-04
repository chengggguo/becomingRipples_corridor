# Presence Group Controller — Two-device TF-Luna Digital OUT Version

## 文件与作用

- `presence_group_controller_2devices_tfluna.ino`：TF-Luna 双装置控制器正式版本。TF-Luna 预先通过 USB/串口上位机配置，实际安装时只读取 Digital OUT。
- `README.md`：本版本的传感器配置、三线接法、控制器引脚、继电器触点接法和参数说明。

本版本不通过 Arduino 读取距离数据，不使用 TX/RX，也不使用 I2C。距离阈值、回差区间和进入/离开延迟由 TF-Luna 自己判断；Arduino 只接收“范围内/范围外”的数字状态。

## TF-Luna 预先配置

用 USB 转串口和 TF-Luna 上位机完成下列设置，并把设置保存到 TF-Luna：

1. 启用 on/off mode。
2. 设置目标距离 `Dist`。
3. 设置回差 `Zone`，避免目标位于阈值附近时输出反复跳变。
4. 设置 `Delay1` 和 `Delay2`，分别控制进入与离开状态需要持续多久才切换输出。
5. 设置为“检测到近距离目标时 Pin 6 输出 HIGH”。
6. 按官方说明设置 Amp 相关阈值，避免无有效目标时距离值为 0 引起误触发。

参考：[Benewake TF-Luna 官方用户手册](https://en.benewake.com/uploadfiles/2025/04/20250430174515390.pdf)。

代码中的：

```cpp
const bool SENSOR_ACTIVE_HIGH = true;
```

必须与 TF-Luna 保存的输出极性一致。如果你在上位机中配置成“检测到目标时输出 LOW”，就把它改为 `false`。

## 正式运行的三线接法

这里的 Digital OUT 指 TF-Luna 的 **Pin 6 multiplexing output**，不是 UART TX。

```text
TF-Luna Pin 1  +5V          -> 稳定的 5V 电源
TF-Luna Pin 4  GND          -> 传感器控制 Arduino GND
TF-Luna Pin 6  Digital OUT  -> 传感器控制 Arduino D2
```

- Pin 2 RX、Pin 3 TX 在正式运行时不接 Arduino。
- Pin 5 不接地，保持 UART/on-off 使用的模式；接地会切换为 I2C 模式。
- 不要只按线材颜色判断引脚，按 TF-Luna 插头的实际 Pin 编号核对。
- Digital OUT 是信号线，必须与 Arduino 共地，否则 D2 没有可靠的电压参考。

## 控制器全部相关引脚

```text
D2  <- TF-Luna Pin 6 Digital OUT
D3  <-> 另一排控制器的 active-low 联动 Bus

D5  -> Relay CH1 -> 装置 A D2 RUN
D6  -> Relay CH2 -> 装置 A D3 RESET
D7  -> Relay CH3 -> 装置 B D2 RUN
D8  -> Relay CH4 -> 装置 B D3 RESET
D13 -> 本控制器 RUN 状态灯
```

特别注意：控制器侧和装置侧都使用了 D2/D3，但它们不是同一块 Arduino：

- **传感器控制器 D2**：读取 TF-Luna Digital OUT。
- **传感器控制器 D3**：两块传感器控制器之间的联动 Bus。
- **装置 Arduino D2**：RUN 输入，继电器闭合时被接到该装置 GND。
- **装置 Arduino D3**：RESET/AUTOHOME 输入，继电器闭合时被接到该装置 GND。

## 继电器触点侧接法

```text
Relay CH1 NO  -> 装置 A Arduino D2 RUN
Relay CH1 COM -> 装置 A Arduino GND
Relay CH2 NO  -> 装置 A Arduino D3 RESET
Relay CH2 COM -> 装置 A Arduino GND

Relay CH3 NO  -> 装置 B Arduino D2 RUN
Relay CH3 COM -> 装置 B Arduino GND
Relay CH4 NO  -> 装置 B Arduino D3 RESET
Relay CH4 COM -> 装置 B Arduino GND
```

同一台装置对应的两个 COM 可以互联，共用该装置自己的 GND。NO 触点闭合后，相当于把装置 D2 或 D3 暂时接地。NC 不使用。

## 与 SR602 版相比调整了什么

- 传感器仍接控制器 D2，因此继电器和装置接线完全不变。
- SR602 版的 6.5 秒持续检测改为 100 ms 毛刺过滤。
- 开机忽略传感器时间由 4 秒改为 1 秒。
- TF-Luna 的距离阈值、Zone、Delay1/Delay2 不在 Arduino 代码里设置，而由上位机预先写入传感器。
- 90 秒 RUN 保持、第二台延迟 10 秒启动、累计运行 30 分钟后排队复位，以及 D3 Bus 逻辑均保持不变。

`presenceDebounceMs = 100` 不是人体停留时间；它只过滤很短的电气毛刺。真正的检测距离与进入/离开确认时间应在 TF-Luna 中设定，避免传感器和 Arduino 两边重复增加长延迟。

## 上传前配置

两排控制器分别设置：

```cpp
const char ROW_ID = 'A';
// 或
const char ROW_ID = 'B';
```

当前继电器模块按高电平吸合配置：

```cpp
const bool RELAY_ACTIVE_LOW = false;
```

如果实际继电器板是低电平吸合，必须改为 `true`。

## 更新历史

### 2026-09-04

- 从 SR602 双装置版本派生 TF-Luna Digital OUT 独立版本。
- 明确采用三线正式接法：5V、GND、Pin 6 Digital OUT。
- 去除 I2C 距离读取，让 TF-Luna 自身承担距离阈值、回差和延迟判断。
- 保留 D5–D8 的 RUN/RESET 继电器映射及原有运行、联动、复位调度。
