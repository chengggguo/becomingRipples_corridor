# Wiring Guide / 接线指南

## 中文

本文档描述每一排 `Presence Group Controller` 的接线方式。每排控制器负责本排 3 台 190cmBar 装置。两排控制器之间通过一根 Bus 线共享 `RUN` 请求。

### 每排硬件

每一排需要以下硬件：

- 1 块 Arduino Nano R3，作为 presence group controller。
- 1 个 LD2410C 或类似 presence 传感器。
- 1 个 6 路继电器模块，或 2 个 4 路继电器模块并只使用其中 6 路。
- 3 台 190cmBar 装置。
- 5V 电源，用于 group controller、传感器和继电器模块控制侧。

### 分组控制器引脚

```text
Nano D2  <- LD2410C OUT
Nano D3  <-> other group controller D3 bus

Nano D5  -> Relay IN1  RUN for device 1
Nano D6  -> Relay IN2  RUN for device 2
Nano D7  -> Relay IN3  RUN for device 3

Nano D8  -> Relay IN4  AUTOHOME/RESET request for device 1
Nano D9  -> Relay IN5  AUTOHOME/RESET request for device 2
Nano D10 -> Relay IN6  AUTOHOME/RESET request for device 3
```

### 传感器接线

```text
LD2410C VCC -> Nano 5V
LD2410C GND -> Nano GND
LD2410C OUT -> Nano D2
```

当前代码默认 `SENSOR_ACTIVE_HIGH = true`。如果实测发现 OUT 极性相反，需要在代码里改成 `false`。

### 两块控制器之间的 Bus

```text
Controller A D3  -> Controller B D3
Controller A GND -> Controller B GND
```

`D3` 是 active-low Bus。任意一块控制器处于本地 3 分钟 `RUN` 保持期时，会把 Bus 拉低。另一块控制器读到 `D3 = LOW` 后，也会打开自己的三路 `RUN` 继电器。

这个 Bus 方案需要两块 group controller 共地。如果两块控制器之间的线很长，建议使用双绞线，并考虑在 Bus 线上加一个外部上拉电阻，例如 4.7kΩ 到 5V。

### 继电器模块控制侧

```text
Nano 5V  -> Relay VCC
Nano GND -> Relay GND

Nano D5  -> Relay IN1
Nano D6  -> Relay IN2
Nano D7  -> Relay IN3
Nano D8  -> Relay IN4
Nano D9  -> Relay IN5
Nano D10 -> Relay IN6
```

当前代码默认 `RELAY_ACTIVE_LOW = true`，适用于很多低电平触发继电器模块。若你的继电器模块是高电平触发，需要改成 `false`。

### 继电器触点侧到装置

继电器触点侧只作为每台装置自己的“外部按钮”。触点侧不要把三台装置的 GND 连接在一起。

RUN 继电器：

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

装置端 `D2` 使用 `INPUT_PULLUP`。继电器闭合时，`D2` 被拉到本机 `GND`，装置读到 `LOW = RUN`。

AUTOHOME/RESET 请求继电器：

```text
Relay CH4 NO  -> Device 1 Arduino D3
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3
Relay CH6 COM -> Device 3 Arduino GND
```

装置端 `D3` 使用 `INPUT_PULLUP`。继电器闭合时，`D3` 被拉到本机 `GND`，装置读到 `LOW = AUTOHOME/RESET request`。

### 推荐保护电阻

正式安装时，建议在每一路继电器 `NO` 到装置 Arduino 输入脚之间串联一个 1kΩ 保护电阻：

```text
Relay NO -> 1kΩ -> Device Arduino D2 or D3
Relay COM -> Device Arduino GND
```

这个 1kΩ 是串联保护电阻，不是上拉电阻。

### 供电和地线边界

- group controller、LD2410C、继电器模块控制侧需要共用同一个 5V/GND 系统。
- 两块 group controller 之间的 Bus 方案需要共地。
- 三台 190cmBar 装置之间不需要共地。
- group controller 和装置 Arduino 之间不需要共地，因为它们通过继电器触点侧隔离。

### Reset 时间

当前默认 reset 调度如下：

```text
last presence detected
-> keep RUN for 3 minutes
-> enter IDLE
-> wait 5 minutes
-> send reset request to one device for 2 seconds
-> wait 10 minutes
-> send reset request to the next device for 2 seconds
-> wait 10 minutes
-> send reset request to the third device for 2 seconds
```

如果中途检测到人，reset 调度会取消，所有 reset 继电器关闭，`RUN` 继电器恢复响应。

### 快速测试顺序

建议按以下顺序测试：

1. 单独测试一台装置的 `D2 RUN`：手动短接 `D2` 到 `GND`，确认装置运行。
2. 单独测试一台装置的 `D3 AUTOHOME/RESET`：手动短接 `D3` 到 `GND`，确认装置在 IDLE 时归零。
3. 用继电器测试一台装置的 `RUN` 和 `RESET`。
4. 接上本排三台装置。
5. 接上 LD2410C，确认 presence 能启动三台。
6. 接上两块 group controller 的 `D3/GND` Bus，确认任意一侧触发时六台都会运行。

## English

This document describes the wiring for each row-level `Presence Group Controller`. Each controller handles 3 local 190cmBar devices. The two row controllers share `RUN` requests through one bus line.

### Per Row Hardware

Each row needs:

- 1 Arduino Nano R3 as the presence group controller.
- 1 LD2410C or similar presence sensor.
- 1 six-channel relay module, or 2 four-channel relay modules using only 6 channels.
- 3 190cmBar devices.
- A 5V supply for the group controller, sensor, and relay module control side.

### Group Controller Pins

```text
Nano D2  <- LD2410C OUT
Nano D3  <-> other group controller D3 bus

Nano D5  -> Relay IN1  RUN for device 1
Nano D6  -> Relay IN2  RUN for device 2
Nano D7  -> Relay IN3  RUN for device 3

Nano D8  -> Relay IN4  AUTOHOME/RESET request for device 1
Nano D9  -> Relay IN5  AUTOHOME/RESET request for device 2
Nano D10 -> Relay IN6  AUTOHOME/RESET request for device 3
```

### Sensor Wiring

```text
LD2410C VCC -> Nano 5V
LD2410C GND -> Nano GND
LD2410C OUT -> Nano D2
```

The current code assumes `SENSOR_ACTIVE_HIGH = true`. If testing shows the OUT polarity is opposite, change it to `false` in the code.

### Bus Between Two Controllers

```text
Controller A D3  -> Controller B D3
Controller A GND -> Controller B GND
```

`D3` is an active-low bus. When either controller is inside its local 3-minute `RUN` hold window, it pulls the bus LOW. The other controller reads `D3 = LOW` and turns on its own three `RUN` relays.

This bus option requires the two group controllers to share GND. If the wire between controllers is long, use twisted pair if possible and consider adding an external pull-up resistor, such as 4.7kΩ to 5V, on the bus line.

### Relay Module Control Side

```text
Nano 5V  -> Relay VCC
Nano GND -> Relay GND

Nano D5  -> Relay IN1
Nano D6  -> Relay IN2
Nano D7  -> Relay IN3
Nano D8  -> Relay IN4
Nano D9  -> Relay IN5
Nano D10 -> Relay IN6
```

The current code assumes `RELAY_ACTIVE_LOW = true`, which matches many active-low relay modules. If your relay module is active-high, change it to `false`.

### Relay Contact Side to Devices

The relay contact side acts only as an "external button" for each device. Do not use the contact side to tie the three device grounds together.

RUN relays:

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

Device-side `D2` uses `INPUT_PULLUP`. When the relay closes, `D2` is pulled to the device's own `GND`, so the device reads `LOW = RUN`.

AUTOHOME/RESET request relays:

```text
Relay CH4 NO  -> Device 1 Arduino D3
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3
Relay CH6 COM -> Device 3 Arduino GND
```

Device-side `D3` uses `INPUT_PULLUP`. When the relay closes, `D3` is pulled to the device's own `GND`, so the device reads `LOW = AUTOHOME/RESET request`.

### Recommended Protection Resistors

For final installation, add a 1kΩ series protection resistor between each relay `NO` contact and the device Arduino input pin:

```text
Relay NO -> 1kΩ -> Device Arduino D2 or D3
Relay COM -> Device Arduino GND
```

This 1kΩ resistor is a series protection resistor, not a pull-up resistor.

### Power and Ground Boundaries

- The group controller, LD2410C, and relay module control side should share the same 5V/GND system.
- The bus between the two group controllers requires shared GND.
- The three 190cmBar devices do not need to share GND with each other.
- The group controller and device Arduinos do not need to share GND because the relay contact side provides isolation.

### Reset Timing

Current default reset schedule:

```text
last presence detected
-> keep RUN for 3 minutes
-> enter IDLE
-> wait 5 minutes
-> send reset request to one device for 2 seconds
-> wait 10 minutes
-> send reset request to the next device for 2 seconds
-> wait 10 minutes
-> send reset request to the third device for 2 seconds
```

If presence is detected during the reset sequence, reset scheduling is cancelled, all reset relays turn off, and the `RUN` relays resume response.

### Quick Test Order

Recommended test order:

1. Test one device's `D2 RUN`: manually short `D2` to `GND` and confirm the device runs.
2. Test one device's `D3 AUTOHOME/RESET`: manually short `D3` to `GND` and confirm the device homes in IDLE.
3. Test one device's `RUN` and `RESET` through relays.
4. Connect all three local devices.
5. Connect the LD2410C and confirm presence starts all three devices.
6. Connect the `D3/GND` bus between the two group controllers and confirm either side can start all six devices.
