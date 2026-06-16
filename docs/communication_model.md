# Communication Model / 通信模型

## V1 Model / V1 模型

房间里有两个 row-level group controller Arduino。
The room has two row-level group controller Arduinos.

每个 group controller 包含以下部分。
Each group controller has:

- 一个本地 LD2410C 或类似 presence 传感器。
  One local LD2410C or similar presence sensor.
- 三路下游继电器输出。
  Three downstream relay channels.
- 一条连接到另一块 group controller 的 Bus。
  One cross-controller bus link to the other group controller.

核心行为如下。
The key behavior is:

```text
local presence OR remote presence
-> room RUN
-> both row controllers turn on their relay channels
-> all six 190cmBar devices run
```

## Local Row Path / 本排信号路径

每一排内部的触发路径如下。
Inside each row, the trigger path is:

```text
LD2410C OUT
-> row-level group Arduino
-> three relay channels
-> three device Arduinos' D2 trigger pins
```

## Row-Level Relay Control / 本排继电器控制

每块 row-level Arduino 直接读取自己的本地 presence 传感器。
Each row-level Arduino directly reads its local presence sensor.

然后它控制三路继电器输入。
It then controls three relay inputs:

```text
Group Arduino D5 -> Relay IN1
Group Arduino D6 -> Relay IN2
Group Arduino D7 -> Relay IN3
```

继电器触点侧连接到对应装置。
The relay contact side connects to each corresponding device:

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

继电器闭合时，装置自己的 `D2` 被拉到本机 `GND`，所以装置读到 `LOW` 并运行。
When the relay closes, D2 is pulled to the device's own GND, so the device reads `LOW` and runs.

## Cross-Controller Bus / 控制器之间的 Bus

两块 group controller Arduino 必须共享房间级 presence 状态。
The two group controller Arduinos must communicate room-level presence.

如果任意一块控制器检测到 presence，它会进入本地 3 分钟保持期，并在保持期内通过 Bus 告诉另一块控制器继续 `RUN`。两块控制器都会打开各自的三路继电器，所以六台装置都会运行。
If either controller sees presence, it enters its local 3-minute hold window and tells the other controller to keep `RUN` active through the bus. Both controllers then keep their own three relays active, so all six devices run.

当前 controller 程序使用 `D3` 上的简单 active-low digital bus。`D3 = LOW` 表示本地 3 分钟保持期仍有效。更多细节见 `docs/bus_options.md`。
The current controller sketch uses the simple active-low digital bus on D3. `D3 = LOW` means the local 3-minute hold window is still active. See `docs/bus_options.md`.

## Device Communication Boundary / 装置通信边界

group controller 不会和三台 190cmBar 装置 Arduino 交换串口消息。
The group controller does not exchange serial messages with the three 190cmBar device Arduinos.

它只通过继电器干接点给每台装置一个隔离的 `RUN/IDLE` 信号。
It gives each device an isolated `RUN/IDLE` signal through a relay dry contact.
