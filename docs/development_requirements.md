# Development Requirements / 开发需求

## 中文

### 已确认交付内容

本项目有两份代码交付物：

1. 基于原始 `190cmBar_code.ino` 修改的装置端固件。
2. 用于每排 `Presence Group Controller` 的新 Arduino 固件。

### 装置端固件目标

原始 190cmBar 装置现在会持续随机运动。修改后，它应该改为由外部信号触发运行。

装置端目标包括：

- 添加 `triggerPin = 2`。
- 将 `triggerPin` 设置为 `INPUT_PULLUP`。
- `D2 = LOW` 表示 `RUN`。
- `D2 = HIGH` 表示 `IDLE`。
- 开机后运行 `autoHome()`，然后移动到随机待机点。
- 第一次收到 `RUN` 时，先随机延迟，再在当前位置舵机动作一次，然后进入原有随机运动逻辑。
- 当 `RUN` 变为 `IDLE` 时，不急停；完成当前阻塞动作后，移动到随机待机点，舵机回初始角度，并等待下一次触发。
- 在 IDLE 状态下响应 `AUTOHOME/RESET` 请求。
- 禁用旧的 LED 随机路径。
- 修复局部 `rebootThreshold` 遮蔽问题。
- 建议禁用原来的 10 到 20 轮自动软件重启逻辑，由分组控制器在无人时逐台安排 reset/autohome。

### 分组控制器目标

每排有一个 group controller Arduino。总共有两个 group controller，每个都连接自己的 LD2410C 或类似 presence 传感器。

每个 group controller 应该完成以下工作：

- 通过数字 OUT 引脚读取本地 presence 传感器。
- 传感器 OUT 必须连续 active 满 500ms，才认为本地 presence 成立。
- 通过两个控制器之间的 Bus 共享本地 presence 状态。
- 将房间 presence 判断为 `local presence OR remote presence`。
- 在最后一次房间级 presence 之后，让本排三台装置继续 `RUN` 3 分钟。
- 用 `D5`、`D6`、`D7` 分别控制三路 `RUN` 继电器。
- 用 `D8`、`D9`、`D10` 分别控制三路 `AUTOHOME/RESET` 请求继电器。
- 在房间进入 IDLE 后等待一段时间，再逐台发送 reset 请求；每个无人周期每台最多一次。
- 用常量配置继电器触发极性。

### V1 重要范围

第一版不需要六台装置精确同步。

第一版可以保留现有阻塞式运动结构。更快响应停止信号的需求，可以之后再重构为非阻塞状态机。

第一版需要两个 group controller 共享房间级 presence，所以任意一排检测到人，都可以触发六台装置运行。

## English

### Confirmed Deliverables

There are two code deliverables:

1. Modified device firmware based on the original `190cmBar_code.ino`.
2. New Arduino firmware for each row-level `Presence Group Controller`.

### Device Firmware Target

The original 190cmBar device currently runs random movement continuously. It should become externally triggered.

Device-side targets:

- Add `triggerPin = 2`.
- Configure `triggerPin` as `INPUT_PULLUP`.
- Treat `D2 = LOW` as `RUN`.
- Treat `D2 = HIGH` as `IDLE`.
- On boot, run `autoHome()`, then move to a random standby position.
- When `RUN` first arrives, wait a short random delay, swing the servo once at the current position, then enter the original random movement behavior.
- When `RUN` becomes `IDLE`, do not emergency stop; finish the current blocking action, move to a random standby position, return the servo to its initial angle, and wait for the next trigger.
- Respond to `AUTOHOME/RESET` requests while in IDLE.
- Disable the old LED random path.
- Fix the local `rebootThreshold` shadowing bug.
- Disable the old automatic software reboot after 10 to 20 cycles, and let the group controller schedule reset/autohome one device at a time while the room is empty.

### Presence Group Controller Target

Each row has one group controller Arduino. There are two group controllers total, and each one has its own LD2410C or similar presence sensor.

Each group controller should:

- Read its local presence sensor using a digital OUT pin.
- Require the sensor OUT signal to stay active continuously for 500ms before accepting local presence.
- Share its local presence state with the other group controller through a cross-controller bus.
- Treat room presence as `local presence OR remote presence`.
- Keep its three downstream devices in `RUN` for 3 minutes after the last room-level presence.
- Control three `RUN` relay channels separately, using `D5`, `D6`, and `D7`.
- Control three `AUTOHOME/RESET` request relay channels separately, using `D8`, `D9`, and `D10`.
- After the room enters IDLE, wait for a configurable idle delay and send reset requests one device at a time; each device receives at most one request per idle period.
- Allow relay active polarity to be configured with a constant.

### Important V1 Scope

The first version does not need precise synchronization between the six devices.

The first version can keep the existing blocking movement style. Faster interruption can be a later non-blocking state-machine refactor.

The first version does need the two group controllers to share room-level presence, so either row can trigger all six devices.
