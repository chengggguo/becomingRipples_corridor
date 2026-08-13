# Presence Group Controller — Two-device Version

## 文件与作用

- `presence_group_controller_2devices.ino`：当前两装置版 Presence Group Controller 固件。读取 LD2410C 传感器，控制装置 A、B 的 RUN 和 RESET 继电器，并保留两块控制器之间的 D3 Bus 联动。
- `README.md`：说明这个版本的文件用途、引脚、继电器接线、主要运行逻辑和更新记录。

这是由原三装置正式控制器派生的新版本。旧文件 `arduino/controllers/presence_group_controller/presence_group_controller.ino` 保留不变；需要控制两台装置时，应上传本目录中的 `presence_group_controller_2devices.ino`。

## 控制器与继电器引脚

```text
D2  <- LD2410C 数字 OUT
D3  <-> 另一排控制器的 active-low Bus

D5  -> Relay CH1 -> 装置 A D2 RUN
D6  -> Relay CH2 -> 装置 A D3 AUTOHOME/RESET
D7  -> Relay CH3 -> 装置 B D2 RUN
D8  -> Relay CH4 -> 装置 B D3 AUTOHOME/RESET
```

每台装置使用两路独立继电器。对应两路继电器的 COM 可以共同连接该装置自身的 GND。

继电器触点侧接法：

```text
Relay CH1 NO  -> 装置 A Arduino D2
Relay CH1 COM -> 装置 A Arduino GND
Relay CH2 NO  -> 装置 A Arduino D3
Relay CH2 COM -> 装置 A Arduino GND

Relay CH3 NO  -> 装置 B Arduino D2
Relay CH3 COM -> 装置 B Arduino GND
Relay CH4 NO  -> 装置 B Arduino D3
Relay CH4 COM -> 装置 B Arduino GND
```

同一台装置的两个 COM 可以互联，共用该装置自己的 GND；不同装置的 GND 不需要通过继电器触点侧互联。

## 主要运行逻辑

本版本保留原正式控制器的 presence debounce、三分钟 RUN hold、D3 控制器间 Bus、累计运行时间以及逐台 reset 调度逻辑，只把本排受控装置数量从三台改为两台。

- LD2410C 的数字 OUT 接控制器 D2，连续 active 500ms 后确认 presence。
- presence 消失后继续保持 RUN 三分钟。
- 装置 A、B 分别累计自上次 reset 后的 RUN 时间。
- 单台装置累计 RUN 15 分钟后进入 reset 队列。
- 控制器只在房间 IDLE 时逐台发送两秒 reset pulse。
- 两块控制器可以通过 D3 active-low Bus 共享 RUN 请求。

## 上传前配置

上传两排控制器前，分别把 `ROW_ID` 设置为 `'A'` 和 `'B'`。

当前继电器模块按高电平吸合配置：

```cpp
const bool RELAY_ACTIVE_LOW = false;
```

## 更新历史

### 2026-08-13

- 从保留的三装置正式版派生两装置版本，不覆盖旧文件。
- 将 `deviceCount` 从 3 改为 2。
- 固定映射为：D5 = 装置 A RUN、D6 = 装置 A RESET、D7 = 装置 B RUN、D8 = 装置 B RESET。
- D9 和 D10 在本版本中不再用于装置继电器。
