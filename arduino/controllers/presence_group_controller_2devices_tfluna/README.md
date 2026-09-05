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
Nano D2                    -> 约 10kΩ 下拉电阻 -> Nano GND
```

- Pin 2 RX、Pin 3 TX 在正式运行时不接 Arduino。
- Pin 5 不接地，保持 UART/on-off 使用的模式；接地会切换为 I2C 模式。
- 不要只按线材颜色判断引脚，按 TF-Luna 插头的实际 Pin 编号核对。
- Digital OUT 是信号线，必须与 Arduino 共地，否则 D2 没有可靠的电压参考。
- D2 使用外部约 10kΩ 下拉；TF-Luna 未接、掉电或信号线断开时，控制器会稳定读到 LOW，而不是悬空误触发。
- 拟定 HEX、CP2102 配置脚本和到货测试流程见项目根目录 `TF_luna/`。

## 控制器全部相关引脚

```text
D2  <- TF-Luna Pin 6 Digital OUT
D3  <-> 另一排控制器的 active-low 联动 Bus

D5  -> Relay CH1 -> 装置 A A1 RUN
D6  -> Relay CH2 -> 装置 A A2 RESET
D7  -> Relay CH3 -> 装置 B A1 RUN
D8  -> Relay CH4 -> 装置 B A2 RESET
D13 -> 本控制器 RUN 状态灯
```

特别注意：控制器侧 D2/D3 与装置侧 A1/A2 属于不同 Arduino：

- **传感器控制器 D2**：读取 TF-Luna Digital OUT。
- **传感器控制器 D3**：两块传感器控制器之间的联动 Bus。
- **装置 Arduino A1**：作为数字 RUN 输入，继电器闭合时被接到该装置 GND。
- **装置 Arduino A2**：作为数字 RESET/AUTOHOME 输入，继电器闭合时被接到该装置 GND。

## 继电器触点侧接法

```text
Relay CH1 NO  -> 装置 A Arduino A1 RUN
Relay CH1 COM -> 装置 A Arduino GND
Relay CH2 NO  -> 装置 A Arduino A2 RESET
Relay CH2 COM -> 装置 A Arduino GND

Relay CH3 NO  -> 装置 B Arduino A1 RUN
Relay CH3 COM -> 装置 B Arduino GND
Relay CH4 NO  -> 装置 B Arduino A2 RESET
Relay CH4 COM -> 装置 B Arduino GND
```

同一台装置对应的两个 COM 可以互联，共用该装置自己的 GND。NO 触点闭合后，相当于把装置 A1 或 A2 暂时接地。NC 不使用。

## 与 SR602 版相比调整了什么

- 传感器仍接控制器 D2，因此继电器和装置接线完全不变。
- SR602 版的 6.5 秒持续检测改为 100 ms 毛刺过滤。
- 不设置 Arduino 侧的固定开机等待；TF-Luna 上电后输出什么状态，控制器就读取什么状态。
- TF-Luna 的距离阈值、Zone、Delay1/Delay2 不在 Arduino 代码里设置，而由上位机预先写入传感器。
- 当前本机 RUN 保持时间为 9 秒；第二台装置在持续 RUN 满 10 秒后启动。若 TF-Luna 的 HIGH 持续存在，保持计时会不断刷新，两台装置都可进入 RUN；若触发很短并在 10 秒前结束，第二台可能不会启动。
- 累计运行 30 分钟后排队复位，以及 D3 Bus 逻辑保持不变。

`presenceDebounceMs = 100` 不是人体停留时间。D2 必须连续保持有效电平 100 ms 才算触发，用于过滤插拔、电源或长线附近干扰产生的短促脉冲；如果电平中途恢复，计时立即清零。真正的距离判断、Zone 和 Delay1/Delay2 仍在 TF-Luna 内完成。

这 100 ms 不是 TF-Luna 的强制要求。确认现场信号稳定后可改成 `0UL`；保留它的代价是 RUN 最多晚约 100 ms 开始。

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
