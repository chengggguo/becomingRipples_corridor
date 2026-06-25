# 190cmBar Device Firmware / 190cmBar 装置端固件

## 中文

这个文件夹保存每台 190cmBar 装置 Arduino 的修改版固件。当前主文件是：

- `190cmBar_device.ino`

这版代码已经从原始的“上电后持续随机运行”改为“由 Presence Group Controller 通过继电器控制运行和归零”。LED 继电器相关逻辑已经移除，装置端现在只响应两个外部输入：`D2` 的 `RUN/IDLE` 信号，以及 `D3` 的 `AUTOHOME/RESET` 请求。

### 引脚

```text
D2  <- RUN input, LOW = RUN, HIGH = IDLE
D3  <- AUTOHOME/RESET request input, LOW = request
D4  -> left stepper DIR
D5  -> left stepper STEP
D6  -> right stepper STEP
D7  -> right stepper DIR
D8  -> left stepper enable
D9  -> right stepper enable
D10 -> servo
D11 <- left Hall sensor
D12 <- right Hall sensor
```

`D2` 和 `D3` 都使用 `INPUT_PULLUP`。继电器闭合时，把对应输入脚拉到装置 Arduino 自己的 `GND`。

### 开机行为

装置上电后会先执行 `autoHome()`，完成左右步进电机归零。第一次开机时坐标还未知，所以会跳过回 HomePoint 的预移动，直接执行安全反向和 Hall 寻零。归零完成后，装置会移动到画布范围内的一个随机待机点，然后保持舵机在休息角度，等待 `D2 = LOW` 的 `RUN` 信号。

### RUN 行为

当 `D2` 从 `IDLE` 进入 `RUN` 时，装置会先在当前随机待机点执行一次 `swingServo()`。之后每一轮动作按这个顺序执行：

1. 随机等待 1 到 10 秒。
2. 如果 `D2` 已经回到 `HIGH`，这一轮直接结束。
3. 如果 `D2` 仍然是 `LOW`，移动到下一个随机位置。
4. 移动完成后再次读取 `D2` 和 `D3`。
5. 如果 `D2` 仍然是 `LOW`，执行一次 `swingServo()`。

这意味着 `STOP` 不会急停当前移动；如果停止信号发生在移动过程中，装置会完成当前移动，然后不再执行下一次舵机动作，并回到随机待机状态。

### RESET 行为

`D3 = LOW` 会被记录为一次 pending reset 请求。装置不会在 RUN 动作中立即归零，而是在进入 IDLE 后执行：

1. 如果当前位置坐标已知，先移动回 `HomePointX/HomePointY`，也就是画面顶部中心。
2. 左右电机按原安全方向各自移动约 3000 steps，离开 Hall 触发区。
3. 左右电机分别寻找 Hall 触发点，并重新校准坐标。
4. 移动到一个新的随机待机点。
5. 舵机回到休息角度。
6. 继续等待下一次 `RUN`。

这样 reset 可以由 Presence Group Controller 在无人时逐台触发，不会和观众在场时的 RUN 动作直接冲突。

### Watchdog 和旧重启逻辑

当前 watchdog 仍然使用 `WDTO_8S`。所有 1 到 10 秒随机等待都通过 `delayWithWatchdog()` 分段延迟并刷新 watchdog，因此不会因为等待超过 8 秒而误复位。

原始代码中“每 10 到 20 轮自动软件重启”的逻辑已经移除。现在的日常归零由 Presence Group Controller 的逐台 reset 请求负责。`softwareReboot()` 仍然保留，只用于 `moveToPositionSynced()` 运动超时后的异常恢复；它现在通过 watchdog 触发重启。

`autoHome()` 现在有超时保护：安全反向移动默认 15 秒超时，单侧 Hall 寻零默认 20 秒超时。如果超时，代码会关闭步进电机使能，等待 10 秒，然后用 watchdog 触发重启。重启后会重新从开机流程开始尝试归零。

### 已知限制

这版代码保留阻塞式机械动作，优先保证行为简单和现场稳定。`STOP` 和 `AUTOHOME/RESET` 都不会中断正在执行的步进电机移动或舵机动作；它们会在当前动作边界被处理。

## English

This folder contains the modified Arduino firmware for each 190cmBar device. The current main file is:

- `190cmBar_device.ino`

This version changes the device from "continuous random motion after power-on" to "run and home under relay control from the Presence Group Controller". The LED relay logic has been removed. The device now responds to two external inputs only: `D2` for `RUN/IDLE`, and `D3` for `AUTOHOME/RESET` requests.

### Pins

```text
D2  <- RUN input, LOW = RUN, HIGH = IDLE
D3  <- AUTOHOME/RESET request input, LOW = request
D4  -> left stepper DIR
D5  -> left stepper STEP
D6  -> right stepper STEP
D7  -> right stepper DIR
D8  -> left stepper enable
D9  -> right stepper enable
D10 -> servo
D11 <- left Hall sensor
D12 <- right Hall sensor
```

Both `D2` and `D3` use `INPUT_PULLUP`. When a relay closes, it pulls the corresponding input pin to the device Arduino's own `GND`.

### Startup Behavior

After power-on, the device first runs `autoHome()` to home both stepper motors. On the first boot, the coordinate position is still unknown, so the firmware skips the HomePoint pre-move and directly performs the safety reverse and Hall seeking steps. After homing, it moves to a random standby point inside the drawing area, keeps the servo at its rest angle, and waits for `D2 = LOW`.

### RUN Behavior

When `D2` changes from `IDLE` to `RUN`, the device first runs `swingServo()` at the current random standby point. Each following motion cycle then runs in this order:

1. Wait randomly from 1 to 10 seconds.
2. If `D2` has returned to `HIGH`, end this cycle immediately.
3. If `D2` is still `LOW`, move to the next random position.
4. After movement completes, read `D2` and `D3` again.
5. If `D2` is still `LOW`, run `swingServo()` once.

This means `STOP` does not emergency-stop the current movement. If the stop signal arrives during movement, the device finishes that movement, skips the next servo swing, and returns to random standby.

### RESET Behavior

`D3 = LOW` is stored as a pending reset request. The device does not home immediately during RUN. After it reaches IDLE, it performs:

1. If the current coordinate is known, move back to `HomePointX/HomePointY`, the top-center point of the drawing area.
2. Move the left and right motors about 3000 steps in their existing safety-reverse directions, away from the Hall trigger area.
3. Seek the left and right Hall trigger points and recalibrate the coordinate system.
4. Move to a new random standby point.
5. Return the servo to its rest angle.
6. Wait for the next `RUN`.

This lets the Presence Group Controller reset devices one at a time while the room is empty, without directly conflicting with visitor-triggered RUN behavior.

### Watchdog and Old Reset Logic

The watchdog still uses `WDTO_8S`. All 1-to-10-second random waits go through `delayWithWatchdog()`, which breaks the wait into short chunks and refreshes the watchdog, so waits longer than 8 seconds do not cause accidental resets.

The original "automatic software reboot every 10 to 20 rounds" logic has been removed. Routine homing is now handled by one-at-a-time reset requests from the Presence Group Controller. `softwareReboot()` is still kept only as abnormal recovery after a `moveToPositionSynced()` movement timeout; it now triggers reboot through the watchdog.

`autoHome()` now has timeout protection: the safety reverse move defaults to a 15-second timeout, and each Hall seek defaults to a 20-second timeout. If a timeout happens, the firmware disables the stepper enables, waits 10 seconds, and then uses the watchdog to reboot. After reboot, the device starts from the normal startup sequence and attempts homing again.

### Known Limitation

This version keeps blocking mechanical actions because it is simpler and safer for installation testing. `STOP` and `AUTOHOME/RESET` do not interrupt an ongoing stepper movement or servo action; they are handled at action boundaries.
