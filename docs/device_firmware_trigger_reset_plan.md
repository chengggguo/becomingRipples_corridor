# Device Firmware Trigger + Reset Implementation / 装置端触发与 Reset 实现说明

## 中文

本文档描述 `190cmBar_device.ino` 当前已经实现的 `RUN/IDLE` 与 `AUTOHOME/RESET` 控制逻辑。

### 目标

装置端 Arduino 不再上电后持续随机运行，而是由每排的 Presence Group Controller 通过继电器控制。控制器负责判断房间是否有人，并向每台装置提供两个隔离触点信号：

- `RUN/IDLE`：决定装置是否进入运动状态。
- `AUTOHOME/RESET`：在无人休息期逐台请求装置重新归零。

装置端继续保留阻塞式机械动作。也就是说，`STOP` 和 `RESET` 都不会急停正在运行的步进电机或舵机，而是在当前动作边界处理。

### 装置端引脚

```cpp
const int triggerPin = 2;       // LOW = RUN, HIGH = IDLE
const int resetRequestPin = 3;  // LOW = AUTOHOME/RESET request
```

两者都使用内部上拉：

```cpp
pinMode(triggerPin, INPUT_PULLUP);
pinMode(resetRequestPin, INPUT_PULLUP);
```

继电器触点侧接法：

```text
RUN relay NO    -> Device Arduino D2
RUN relay COM   -> Device Arduino GND

RESET relay NO  -> Device Arduino D3
RESET relay COM -> Device Arduino GND
```

### 开机状态

装置开机后按以下顺序执行：

1. 配置步进电机、舵机、Hall 传感器、`D2` 和 `D3`。
2. 舵机回到休息角度。
3. 执行第一次 `autoHome()`。因为第一次开机时坐标未知，所以跳过 HomePoint 预移动，直接执行安全反向和 Hall 寻零。
4. 初始化随机种子。
5. 启用 8 秒 watchdog。
6. 移动到一个随机待机点。
7. 进入 IDLE 监听。

这样六台装置在上电归零后不会都停在同一个 Home 点，而是各自停在随机待机位置。

### RUN 状态机

装置在 IDLE 中持续读取 `D2`。当 `D2 = LOW` 时进入 RUN：

```text
IDLE
-> D2 LOW
-> 在当前随机待机点 swingServo()
-> 进入循环
```

进入循环后，每一轮动作如下：

```text
随机等待 1 到 10 秒
-> 读取 D2
-> 如果 D2 HIGH，结束本轮并回到 IDLE 处理
-> 如果 D2 LOW，移动到下一个随机点
-> 读取 D2 和 D3
-> 如果 D2 LOW，swingServo()
-> 下一轮
```

这个顺序避免了“一轮结束后立刻又推一次布料”的重复触发。每次舵机动作之间至少隔着一次随机等待和一次随机移动。

### STOP / IDLE 行为

如果 `D2` 变回 `HIGH`，装置不会急停当前动作。当前动作结束后，下一次检查到 IDLE 时执行：

1. 移动到一个新的随机待机点。
2. 舵机回到休息角度。
3. `wasRunning = false`。
4. 继续监听 `D2` 和 `D3`。

### RESET 行为

`D3 = LOW` 会通过 `sampleResetRequest()` 记为 `pendingReset = true`。这个采样会发生在：

- 主循环顶部。
- 随机延迟期间。
- `moveToPositionSynced()` 的步进电机运动循环里。
- 舵机动作的短延迟期间。

但是实际 `autoHome()` 只在 IDLE 中执行。进入 IDLE 后，如果存在 pending reset 或当前 `D3 = LOW`，装置会执行：

```text
如果坐标已知，先移动到 HomePointX/HomePointY
-> 左右电机按原安全方向各自移动约 3000 steps
-> 左右电机分别寻找 Hall 触发点
-> 重新校准当前位置为 HomePoint
-> 移动到随机待机点
-> 舵机回到休息角度
-> 清除 pendingReset
```

这样即使 reset 请求发生在 RUN 动作中，也会被记住，但不会打断正在发生的机械动作。

### Watchdog

当前 watchdog 是：

```cpp
wdt_enable(WDTO_8S);
```

随机等待最长为 10 秒，超过 watchdog 的 8 秒窗口，所以代码使用 `delayWithWatchdog()` 把长延迟拆成约 50ms 的短段，并在每段中刷新 watchdog、采样 `D3`。

`autoHome()` 和 `moveToPositionSynced()` 的长循环中也会刷新 watchdog。`moveToPositionSynced()` 保留 30 秒运动超时保护；如果超时，会调用 `softwareReboot()` 作为异常恢复。当前 `softwareReboot()` 会通过 watchdog 触发重启。

`autoHome()` 现在也有自己的超时保护：

- 安全反向移动默认 15 秒超时。
- 左侧 Hall 寻零默认 20 秒超时。
- 右侧 Hall 寻零默认 20 秒超时。

如果 `autoHome()` 超时，代码会关闭步进电机使能，等待 10 秒，然后用 watchdog 触发重启。这样无人运行时会自动重新尝试开机归零。这个策略适合“尽量自恢复”的现场运行，但如果 Hall 传感器或机械结构持续故障，装置会周期性重启并再次尝试归零，因此现场测试时需要确认超时时间不会让机构长时间顶住硬限位。

### 已移除的旧逻辑

原始代码中“每 10 到 20 轮自动软件重启”的逻辑已经移除。

移除原因：

- 现在归零由 Presence Group Controller 在无人时逐台安排。
- 旧自动重启可能在观众仍在场时触发。
- 旧逻辑中的 `delay(10000)` 与 8 秒 watchdog 存在冲突。

### 和 Presence Group Controller 的关系

Presence Group Controller 在检测到无人后会先继续保持 `RUN` 3 分钟。进入 IDLE 后，它会先等待 5 分钟，再给某一台装置发送 2 秒 reset 请求。之后每隔 10 分钟才给下一台装置发送 reset 请求。

装置端不需要知道自己是第几台，只需要响应本机的 `D2` 和 `D3`。逐台错峰 reset 的调度由 Presence Group Controller 负责。

## English

This document describes the currently implemented `RUN/IDLE` and `AUTOHOME/RESET` behavior in `190cmBar_device.ino`.

### Goal

The device Arduino no longer runs random motion continuously after power-on. Instead, each row-level Presence Group Controller drives the devices through relay contacts. The controller decides whether the room is occupied and provides two isolated contact signals to each device:

- `RUN/IDLE`: decides whether the device enters motion.
- `AUTOHOME/RESET`: requests one-at-a-time homing during empty-room idle periods.

The device firmware keeps blocking mechanical actions. In other words, `STOP` and `RESET` do not emergency-stop an active stepper or servo action; they are handled at action boundaries.

### Device Pins

```cpp
const int triggerPin = 2;       // LOW = RUN, HIGH = IDLE
const int resetRequestPin = 3;  // LOW = AUTOHOME/RESET request
```

Both pins use internal pull-ups:

```cpp
pinMode(triggerPin, INPUT_PULLUP);
pinMode(resetRequestPin, INPUT_PULLUP);
```

Relay contact wiring:

```text
RUN relay NO    -> Device Arduino D2
RUN relay COM   -> Device Arduino GND

RESET relay NO  -> Device Arduino D3
RESET relay COM -> Device Arduino GND
```

### Startup State

After power-on, the device runs this sequence:

1. Configure steppers, servo, Hall sensors, `D2`, and `D3`.
2. Move the servo to its rest angle.
3. Run the first `autoHome()`. Because the coordinate position is unknown on first boot, the HomePoint pre-move is skipped and the firmware directly performs the safety reverse and Hall seeking steps.
4. Initialize the random seed.
5. Enable the 8-second watchdog.
6. Move to a random standby point.
7. Enter IDLE listening.

This prevents all six devices from staying at the same Home point after startup and homing.

### RUN State Machine

While in IDLE, the device continuously reads `D2`. When `D2 = LOW`, it enters RUN:

```text
IDLE
-> D2 LOW
-> swingServo() at current random standby point
-> enter loop
```

After that, each motion cycle runs as follows:

```text
wait randomly from 1 to 10 seconds
-> read D2
-> if D2 HIGH, end this cycle and return to IDLE handling
-> if D2 LOW, move to the next random point
-> read D2 and D3
-> if D2 LOW, swingServo()
-> next cycle
```

This avoids the double-trigger problem where one completed cycle would immediately cause another cloth push. Each servo action is separated by at least one random wait and one random movement.

### STOP / IDLE Behavior

If `D2` returns to `HIGH`, the device does not emergency-stop the current action. After the current action ends, the next IDLE check runs:

1. Move to a new random standby point.
2. Return the servo to its rest angle.
3. Set `wasRunning = false`.
4. Continue listening to `D2` and `D3`.

### RESET Behavior

`D3 = LOW` is stored by `sampleResetRequest()` as `pendingReset = true`. Sampling happens during:

- The top of the main loop.
- Random delay periods.
- The `moveToPositionSynced()` stepper movement loop.
- The short delays inside the servo action.

However, the actual `autoHome()` only runs in IDLE. When the device reaches IDLE and either has a pending reset or currently reads `D3 = LOW`, it runs:

```text
if the coordinate is known, move to HomePointX/HomePointY first
-> move the left and right motors about 3000 steps in their existing safety-reverse directions
-> seek the left and right Hall trigger points
-> recalibrate the current position as HomePoint
-> move to random standby point
-> return servo to rest angle
-> clear pendingReset
```

This means a reset request that arrives during RUN is remembered, but it does not interrupt the active mechanical action.

### Watchdog

The current watchdog is:

```cpp
wdt_enable(WDTO_8S);
```

The random wait can be as long as 10 seconds, longer than the 8-second watchdog window. Therefore the code uses `delayWithWatchdog()` to split long waits into roughly 50ms chunks, refreshing the watchdog and sampling `D3` in each chunk.

The long loops in `autoHome()` and `moveToPositionSynced()` also refresh the watchdog. `moveToPositionSynced()` keeps its 30-second movement timeout; on timeout, it calls `softwareReboot()` as abnormal recovery. The current `softwareReboot()` triggers reboot through the watchdog.

`autoHome()` now has its own timeout protection:

- The safety reverse move defaults to a 15-second timeout.
- The left Hall seek defaults to a 20-second timeout.
- The right Hall seek defaults to a 20-second timeout.

If `autoHome()` times out, the firmware disables the stepper enables, waits 10 seconds, and then uses the watchdog to reboot. This lets the device automatically retry startup homing during unattended operation. This is useful for self-recovery, but if a Hall sensor or mechanical part remains faulty, the device will periodically reboot and try homing again, so on-site testing should confirm that the timeout is short enough to avoid pushing against a hard mechanical limit for too long.

### Removed Old Logic

The original "automatic software reboot every 10 to 20 rounds" logic has been removed.

Reasons:

- Homing is now scheduled one device at a time by the Presence Group Controller while the room is empty.
- The old automatic reboot could trigger while visitors are still present.
- The old `delay(10000)` conflicted with the 8-second watchdog.

### Relationship to the Presence Group Controller

After the Presence Group Controller sees no presence, it keeps `RUN` active for 3 more minutes. After entering IDLE, it waits 5 minutes, then sends a 2-second reset request to one device. It then waits 10 minutes before sending a reset request to the next device.

The device firmware does not need to know which device number it is. It only responds to its own `D2` and `D3`. The one-at-a-time staggered reset schedule is handled by the Presence Group Controller.
