# Presence Group Controller Firmware / Presence 分组控制器固件

这个文件夹用于保存新的 row-level Arduino sketch。
This folder is for the new row-level Arduino sketch.

## Overview / 功能概述

这份代码运行在每排一块 Arduino Nano R3 上，用来把“房间里是否有人”转换成三台 190cmBar 装置可以理解的 `RUN/IDLE` 触发信号。每块控制器读取自己这一侧的 LD2410C presence 传感器，并用三路继电器分别控制本排三台装置；同时，两块控制器通过 `D3` 上的一根 active-low Bus 共享本地 3 分钟 `RUN` 请求。这样无论观众先被哪一侧传感器检测到，两排控制器都会启动各自的三路继电器，最终让六台装置一起运行。继电器触点侧只像“外部按钮”一样短接每台装置自己的 `D2` 和 `GND`，因此装置之间不需要共地。

This sketch runs on one Arduino Nano R3 per row. Its job is to convert room-level presence into `RUN/IDLE` trigger signals that three local 190cmBar devices can understand. Each controller reads the LD2410C presence sensor on its side and drives three relay channels for the three devices in that row. At the same time, the two controllers share their local 3-minute `RUN` requests through an active-low bus on `D3`. This means that whichever sensor detects the visitor first, both row controllers activate their own relay channels, so all six devices run together. On the relay contact side, each relay behaves like an isolated external button that shorts one device's own `D2` to its own `GND`, so the device Arduinos do not need to share ground with each other.

## Current Sketch / 当前 Sketch

- `presence_group_controller.ino`

## Planned Behavior / 计划行为

- 在 `D2` 读取 LD2410C 数字 OUT。
  Read LD2410C digital OUT on D2.
- 最后一次检测到 presence 后，继电器继续保持 3 分钟。
  Keep relays active for 3 minutes after last presence.
- 用 `D5`、`D6`、`D7` 驱动三路继电器。
  Drive relay channels on D5, D6, and D7.
- 支持低电平触发或高电平触发继电器模块。
  Support active-low or active-high relay modules.
- 通过继电器干接点向三台装置 Arduino 发送 `RUN/IDLE`。
  Send `RUN/IDLE` to three device Arduinos through relay dry contacts.
- 通过 `D3` 上的 active-low Bus 与另一排控制器共享本地 3 分钟 `RUN` 请求。
  Share the local 3-minute `RUN` request with the other row controller through an active-low bus on D3.

## Default Wiring / 默认接线

```text
LD2410C OUT -> Nano D2

Nano D3    -> other controller D3
Nano GND   -> other controller GND

Nano D5    -> Relay IN1
Nano D6    -> Relay IN2
Nano D7    -> Relay IN3
Nano 5V    -> Relay VCC
Nano GND   -> Relay GND
```

继电器触点侧。
Relay contact side:

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

## Config Constants / 配置常量

- `SENSOR_ACTIVE_HIGH`：如果传感器 OUT 极性相反，修改这个常量。
  `SENSOR_ACTIVE_HIGH`: change if the sensor OUT polarity is opposite.
- `RELAY_ACTIVE_LOW`：如果继电器模块是高电平触发，修改这个常量。
  `RELAY_ACTIVE_LOW`: change if the relay module is active-high.
- `holdTimeMs`：默认是 `180000UL`，也就是 3 分钟。
  `holdTimeMs`: default is `180000UL`, equal to 3 minutes.

## Bus Semantics / Bus 语义

`D3 = LOW` 表示某一块控制器正处于自己的本地 3 分钟保持期内。

`D3 = LOW` means one controller is currently inside its own local 3-minute hold window.

每块控制器只把自己的本地保持状态发送到 Bus，不会把 remote Bus 状态再次发送回去，这样可以避免两个控制器互相续命导致永远不停止。

Each controller only publishes its own local hold state to the bus. It does not re-publish the remote bus state, which avoids an infinite mutual hold between the two controllers.
