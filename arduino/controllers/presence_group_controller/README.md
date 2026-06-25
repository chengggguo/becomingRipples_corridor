# Presence Group Controller Firmware / Presence 分组控制器固件

## 中文

这个文件夹用于保存每排 `Presence Group Controller` 的 Arduino sketch。

### 功能概述

这份代码运行在每排一块 Arduino Nano R3 上，用来把“房间里是否有人”转换成三台 190cmBar 装置可以理解的 `RUN/IDLE` 和 `AUTOHOME/RESET` 请求信号。

每块控制器读取自己这一侧的 LD2410C presence 传感器，并用六路继电器控制本排三台装置：三路用于 `RUN`，三路用于逐台发送 `AUTOHOME/RESET` 请求。同时，两块控制器通过 `D3` 上的一根 active-low Bus 共享本地 3 分钟 `RUN` 请求。这样无论观众先被哪一侧传感器检测到，两排控制器都会启动各自的三路 `RUN` 继电器，最终让六台装置一起运行。

每台本排装置都会单独累计自上次 reset 之后的 RUN 时长。某台装置累计 RUN 达到阈值后，控制器会等房间无人、RUN 保持期结束、并经过空闲延迟后，再一次只向一台装置发送 reset 请求，避免三台同时归零。继电器触点侧只像“外部按钮”一样短接每台装置自己的输入脚和 `GND`，因此装置之间不需要共地。

为减少瞬时误触发，传感器 OUT 必须连续保持 active 满 500ms，控制器才会刷新本地 3 分钟 `RUN` 保持期。

### 当前 Sketch

- `presence_group_controller.ino`

### 当前行为

- 在 `D2` 读取 LD2410C 数字 OUT。
- 最后一次检测到 presence 后，继电器继续保持 3 分钟。
- 用 `D5`、`D6`、`D7` 驱动三路 `RUN` 继电器。
- 用 `D8`、`D9`、`D10` 驱动三路 `AUTOHOME/RESET` 请求继电器。
- 支持低电平触发或高电平触发继电器模块。
- 通过继电器干接点向三台装置 Arduino 发送 `RUN/IDLE`。
- 通过 `D3` 上的 active-low Bus 与另一排控制器共享本地 3 分钟 `RUN` 请求。
- 传感器必须连续 active 满 500ms，才会被认为是真的 presence。
- 每台装置累计 RUN 15 分钟后，才会进入 reset 等待队列。
- reset 中如果重新检测到 presence，当前 2 秒 reset pulse 会先完成，然后恢复 RUN；未 reset 的装置会留到下一次 IDLE 窗口继续处理。

### 默认接线

更完整的现场接线说明见 `docs/wiring_guide.md`。

```text
LD2410C OUT -> Nano D2

Nano D3    -> other controller D3
Nano GND   -> other controller GND

Nano D5    -> Relay IN1
Nano D6    -> Relay IN2
Nano D7    -> Relay IN3
Nano D8    -> Relay IN4
Nano D9    -> Relay IN5
Nano D10   -> Relay IN6
Nano 5V    -> Relay VCC
Nano GND   -> Relay GND
```

继电器触点侧：

```text
Relay CH1 NO  -> Device 1 Arduino D2 RUN input
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2 RUN input
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2 RUN input
Relay CH3 COM -> Device 3 Arduino GND

Relay CH4 NO  -> Device 1 Arduino D3 AUTOHOME/RESET request input
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3 AUTOHOME/RESET request input
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3 AUTOHOME/RESET request input
Relay CH6 COM -> Device 3 Arduino GND
```

### 配置常量

- `SENSOR_ACTIVE_HIGH`：如果传感器 OUT 极性相反，修改这个常量。
- `RELAY_ACTIVE_LOW`：当前默认 `false`，适用于现场测试过的高电平触发继电器模块；如果换成低电平触发模块，改为 `true`。
- `presenceDebounceMs`：传感器必须连续 active 的时间，默认 500ms。
- `holdTimeMs`：默认是 `180000UL`，也就是 3 分钟。
- `runTimeBeforeResetMs`：每台装置自上次 reset 后累计 RUN 多久才需要再次 reset，默认 15 分钟。
- `resetIdleDelayMs`：房间进入 IDLE 后，等待多久才开始逐台发送 reset 请求，默认 5 分钟。
- `resetPulseMs`：每个 reset 请求继电器保持吸合的时间，默认 2 秒。
- `resetBetweenDevicesMs`：同一排两台装置 reset 请求之间的间隔，默认 10 分钟。
- `resetStartOffsetMs`：给某一排增加额外 reset 起始延迟，用来错开两排的 reset 时间。

### A/B 两排 Offset 配置

如果两排上传同一份代码，并且 `resetStartOffsetMs` 都是 `0UL`，两排会在同一时间各自 reset 一台装置。为了错峰，建议上传前这样配置：

```cpp
// Row A controller
const unsigned long resetStartOffsetMs = 0UL;

// Row B controller
const unsigned long resetStartOffsetMs = 300000UL;
```

这样在存在待 reset 装置时，Row A 会在房间进入 IDLE 后 5 分钟开始处理第一台；Row B 会在房间进入 IDLE 后 10 分钟开始处理第一台。之后两排都继续按 10 分钟间隔处理下一台。

### Bus 语义

`D3 = LOW` 表示某一块控制器正处于自己的本地 3 分钟保持期内。

每块控制器只把自己的本地保持状态发送到 Bus，不会把 remote Bus 状态再次发送回去，这样可以避免两个控制器互相续命导致永远不停止。

### Reset 请求调度

每台装置都会单独累计自上次 reset 之后的 RUN 时长。只有某台装置累计 RUN 达到 `runTimeBeforeResetMs` 时，它才会进入 reset 等待队列。

当 `roomRunActive` 为 false，也就是本地和远端都不再请求 `RUN` 时，控制器会启动空闲计时。默认等待 5 分钟后，它会向待 reset 队列中的装置逐台发送 reset 请求：每次只打开一路 reset 继电器，保持 2 秒，然后等待 10 分钟再处理下一台，让上一台装置有足够时间完成 `autoHome()` 和回到待机点。

如果 reset pulse 正在发送时重新检测到观众，当前 2 秒 pulse 会先完成，然后 `RUN` 继电器恢复正常响应。已经排队但还没有 reset 的装置会保留在队列里，等下一次 IDLE 窗口继续处理。某台装置在中断期间又累计到 reset 阈值时，会进入下一轮队列，避免刚 reset 过的装置立即插队重复 reset。

## English

This folder is for the Arduino sketch for each row-level `Presence Group Controller`.

### Overview

This sketch runs on one Arduino Nano R3 per row. Its job is to convert room-level presence into `RUN/IDLE` and `AUTOHOME/RESET` request signals that three local 190cmBar devices can understand.

Each controller reads the LD2410C presence sensor on its side and drives six relay channels for the three devices in that row: three for `RUN`, and three for one-at-a-time `AUTOHOME/RESET` requests. At the same time, the two controllers share their local 3-minute `RUN` requests through an active-low bus on `D3`. This means that whichever sensor detects the visitor first, both row controllers activate their own `RUN` relay channels, so all six devices run together.

Each local device tracks its own accumulated RUN time since its last reset. After a device reaches the reset threshold, the controller waits until the room is empty, the RUN hold window has ended, and the idle delay has passed. It then sends reset requests one device at a time, avoiding three devices homing at the same time. On the relay contact side, each relay behaves like an isolated external button that shorts one device's own input pin to its own `GND`, so the device Arduinos do not need to share ground with each other.

To reduce momentary false triggers, the sensor OUT signal must stay active continuously for 500ms before the controller refreshes the local 3-minute `RUN` hold window.

### Current Sketch

- `presence_group_controller.ino`

### Current Behavior

- Read LD2410C digital OUT on D2.
- Keep relays active for 3 minutes after last presence.
- Drive `RUN` relay channels on D5, D6, and D7.
- Drive `AUTOHOME/RESET` request relay channels on D8, D9, and D10.
- Support active-low or active-high relay modules.
- Send `RUN/IDLE` to three device Arduinos through relay dry contacts.
- Share the local 3-minute `RUN` request with the other row controller through an active-low bus on D3.
- Require the sensor to stay active continuously for 500ms before accepting it as real presence.
- Queue a device for reset only after that device has accumulated 15 minutes of RUN time since its own last reset.
- If presence returns during a reset pulse, finish the current 2-second pulse first, then resume RUN; unfinished queued devices are kept for the next IDLE window.

### Default Wiring

For a fuller on-site wiring guide, see `docs/wiring_guide.md`.

```text
LD2410C OUT -> Nano D2

Nano D3    -> other controller D3
Nano GND   -> other controller GND

Nano D5    -> Relay IN1
Nano D6    -> Relay IN2
Nano D7    -> Relay IN3
Nano D8    -> Relay IN4
Nano D9    -> Relay IN5
Nano D10   -> Relay IN6
Nano 5V    -> Relay VCC
Nano GND   -> Relay GND
```

Relay contact side:

```text
Relay CH1 NO  -> Device 1 Arduino D2 RUN input
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2 RUN input
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2 RUN input
Relay CH3 COM -> Device 3 Arduino GND

Relay CH4 NO  -> Device 1 Arduino D3 AUTOHOME/RESET request input
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3 AUTOHOME/RESET request input
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3 AUTOHOME/RESET request input
Relay CH6 COM -> Device 3 Arduino GND
```

### Config Constants

- `SENSOR_ACTIVE_HIGH`: change if the sensor OUT polarity is opposite.
- `RELAY_ACTIVE_LOW`: currently defaults to `false` for the on-site active-high relay module; change it to `true` for active-low relay modules.
- `presenceDebounceMs`: how long the sensor must stay continuously active. The default is 500ms.
- `holdTimeMs`: default is `180000UL`, equal to 3 minutes.
- `runTimeBeforeResetMs`: how much RUN time each device must accumulate since its own last reset before it is queued for reset. The default is 15 minutes.
- `resetIdleDelayMs`: how long to wait after the room enters IDLE before one-at-a-time reset requests begin. The default is 5 minutes.
- `resetPulseMs`: how long each reset request relay stays active. The default is 2 seconds.
- `resetBetweenDevicesMs`: the gap between reset requests for devices in the same row. The default is 10 minutes.
- `resetStartOffsetMs`: an extra reset start delay for one row, useful for staggering reset timing between the two rows.

### A/B Row Offset Setup

If both rows are uploaded with the same code and both use `resetStartOffsetMs = 0UL`, the two rows will each reset one device at the same time. To stagger the rows, configure this before uploading:

```cpp
// Row A controller
const unsigned long resetStartOffsetMs = 0UL;

// Row B controller
const unsigned long resetStartOffsetMs = 300000UL;
```

With this setup, when there are devices queued for reset, Row A starts handling the first queued device 5 minutes after the room enters IDLE. Row B starts handling the first queued device 10 minutes after the room enters IDLE. Both rows then continue with the same 10-minute gap before the next local device.

### Bus Semantics

`D3 = LOW` means one controller is currently inside its own local 3-minute hold window.

Each controller only publishes its own local hold state to the bus. It does not re-publish the remote bus state, which avoids an infinite mutual hold between the two controllers.

### Reset Request Scheduler

Each local device separately accumulates RUN time since its own last reset. A device is queued for reset only after its accumulated RUN time reaches `runTimeBeforeResetMs`.

When `roomRunActive` is false, meaning neither the local nor remote side is requesting `RUN`, the controller starts an idle timer. After the default 5-minute delay, it sends reset requests to queued local devices one at a time: one reset relay turns on for 2 seconds, then the controller waits 10 minutes before handling the next device, giving the previous device enough time to finish `autoHome()` and return to standby.

If presence returns during an active reset pulse, the current 2-second pulse finishes first, then the `RUN` relays resume normal response. Devices that were queued but not yet reset remain queued for the next IDLE window. If a device reaches the runtime threshold again during an interruption, it is added to the next-pass queue so a just-reset device does not immediately jump ahead of unfinished devices.
