# Presence Group Controller Firmware / Presence 分组控制器固件

这个文件夹用于保存新的 row-level Arduino sketch。
This folder is for the new row-level Arduino sketch.

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
- 通过 `D3` 上的 active-low Bus 与另一排控制器共享本地 presence。
  Share local presence with the other row controller through an active-low bus on D3.

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
