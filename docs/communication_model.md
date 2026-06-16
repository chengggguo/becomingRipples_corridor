# Communication Model / 通信模型

## 中文

### V1 模型

房间里有两个 row-level group controller Arduino。

每个 group controller 包含：

- 一个本地 LD2410C 或类似 presence 传感器。
- 六路下游继电器输出：三路 `RUN`，三路 `AUTOHOME/RESET` 请求。
- 一条连接到另一块 group controller 的 Bus。

核心行为如下：

```text
local presence OR remote presence
-> room RUN
-> both row controllers turn on their RUN relay channels
-> all six 190cmBar devices run
```

### 本排信号路径

每一排内部的 `RUN` 触发路径：

```text
LD2410C OUT
-> row-level group Arduino
-> three RUN relay channels
-> three device Arduinos' D2 trigger pins
```

每一排内部的 reset 请求路径：

```text
idle reset scheduler
-> three reset request relay channels
-> three device Arduinos' D3 reset request pins
```

### 本排继电器控制

每块 row-level Arduino 直接读取自己的本地 presence 传感器，然后控制六路继电器输入：

```text
Group Arduino D5  -> Relay IN1 RUN for device 1
Group Arduino D6  -> Relay IN2 RUN for device 2
Group Arduino D7  -> Relay IN3 RUN for device 3
Group Arduino D8  -> Relay IN4 RESET request for device 1
Group Arduino D9  -> Relay IN5 RESET request for device 2
Group Arduino D10 -> Relay IN6 RESET request for device 3
```

继电器触点侧连接到对应装置：

```text
Relay CH1 NO  -> Device 1 Arduino D2 RUN input
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2 RUN input
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2 RUN input
Relay CH3 COM -> Device 3 Arduino GND

Relay CH4 NO  -> Device 1 Arduino D3 RESET request input
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3 RESET request input
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3 RESET request input
Relay CH6 COM -> Device 3 Arduino GND
```

RUN 继电器闭合时，装置自己的 `D2` 被拉到本机 `GND`，所以装置读到 `LOW` 并运行。

Reset 请求继电器闭合时，装置自己的 `D3` 被拉到本机 `GND`，所以装置可以在安全的 IDLE 状态下执行 `autoHome()` 并回到随机待机点。

### 控制器之间的 Bus

两块 group controller Arduino 必须共享房间级 presence 状态。

如果任意一块控制器检测到 presence，它会进入本地 3 分钟保持期，并在保持期内通过 Bus 告诉另一块控制器继续 `RUN`。两块控制器都会打开各自的三路 `RUN` 继电器，所以六台装置都会运行。

当前 controller 程序使用 `D3` 上的简单 active-low digital bus。`D3 = LOW` 表示本地 3 分钟保持期仍有效。更多细节见 `docs/bus_options.md`。

### 装置通信边界

group controller 不会和三台 190cmBar 装置 Arduino 交换串口消息。

它只通过继电器干接点给每台装置隔离的 `RUN/IDLE` 和 `AUTOHOME/RESET` 请求信号。

## English

### V1 Model

The room has two row-level group controller Arduinos.

Each group controller has:

- One local LD2410C or similar presence sensor.
- Six downstream relay channels: three for `RUN`, and three for `AUTOHOME/RESET` requests.
- One cross-controller bus link to the other group controller.

The key behavior is:

```text
local presence OR remote presence
-> room RUN
-> both row controllers turn on their RUN relay channels
-> all six 190cmBar devices run
```

### Local Row Path

Inside each row, the `RUN` trigger path is:

```text
LD2410C OUT
-> row-level group Arduino
-> three RUN relay channels
-> three device Arduinos' D2 trigger pins
```

Inside each row, the reset request path is:

```text
idle reset scheduler
-> three reset request relay channels
-> three device Arduinos' D3 reset request pins
```

### Row-Level Relay Control

Each row-level Arduino directly reads its local presence sensor, then controls six relay inputs:

```text
Group Arduino D5  -> Relay IN1 RUN for device 1
Group Arduino D6  -> Relay IN2 RUN for device 2
Group Arduino D7  -> Relay IN3 RUN for device 3
Group Arduino D8  -> Relay IN4 RESET request for device 1
Group Arduino D9  -> Relay IN5 RESET request for device 2
Group Arduino D10 -> Relay IN6 RESET request for device 3
```

The relay contact side connects to each corresponding device:

```text
Relay CH1 NO  -> Device 1 Arduino D2 RUN input
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2 RUN input
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2 RUN input
Relay CH3 COM -> Device 3 Arduino GND

Relay CH4 NO  -> Device 1 Arduino D3 RESET request input
Relay CH4 COM -> Device 1 Arduino GND

Relay CH5 NO  -> Device 2 Arduino D3 RESET request input
Relay CH5 COM -> Device 2 Arduino GND

Relay CH6 NO  -> Device 3 Arduino D3 RESET request input
Relay CH6 COM -> Device 3 Arduino GND
```

When a RUN relay closes, the device's own `D2` is pulled to its own `GND`, so the device reads `LOW` and runs.

When a reset request relay closes, the device's own `D3` is pulled to its own `GND`, so the device can run `autoHome()` and return to a random standby point when it is safely in IDLE.

### Cross-Controller Bus

The two group controller Arduinos must communicate room-level presence.

If either controller sees presence, it enters its local 3-minute hold window and tells the other controller to keep `RUN` active through the bus. Both controllers then keep their own three `RUN` relays active, so all six devices run.

The current controller sketch uses the simple active-low digital bus on D3. `D3 = LOW` means the local 3-minute hold window is still active. See `docs/bus_options.md`.

### Device Communication Boundary

The group controller does not exchange serial messages with the three 190cmBar device Arduinos.

It gives each device isolated `RUN/IDLE` and `AUTOHOME/RESET` request signals through relay dry contacts.
