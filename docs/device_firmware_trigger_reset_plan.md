# Device Firmware Trigger + Reset Plan / 装置端触发与 Reset 修改方案

本文档描述 `190cmBar_device.ino` 的下一步修改方案。当前还不是代码实现，而是给装置端修改用的具体设计。
This document describes the next modification plan for `190cmBar_device.ino`. It is a concrete design for the device firmware change, not the implementation yet.

## Goals / 目标

装置端 Arduino 需要从“上电后持续随机运行”改成“由分组控制器控制运行和归零”。
The device Arduino should change from "run random motion continuously after power-on" to "run and home under the group controller's signals".

目标行为如下。
Target behavior:

1. 上电后执行 `autoHome()`。
   Run `autoHome()` after power-on.
2. 归零完成后移动到一个随机待机点。
   Move to a random standby point after homing.
3. 等待 `RUN` 信号。
   Wait for the `RUN` signal.
4. 第一次收到 `RUN` 时，随机等待 1 到 10 秒。
   When `RUN` first arrives, wait for a random delay from 1 to 10 seconds.
5. 随机延迟后，先在当前位置执行一次 `swingServo()`。
   After the random delay, run `swingServo()` once at the current position.
6. 之后循环执行：随机小延迟、移动到随机位置、推布料。
   Then repeat: short random delay, move to a random position, push the cloth.
7. 收到 `IDLE/STOP` 后不急停，完成当前动作，移动到随机待机点并等待。
   After `IDLE/STOP`, do not emergency stop; finish the current action, move to a random standby point, and wait.
8. 在 IDLE 状态下收到 `AUTOHOME/RESET` 请求时，执行 `autoHome()`，然后移动到随机待机点。
   When an `AUTOHOME/RESET` request arrives in IDLE, run `autoHome()`, then move to a random standby point.

## Pins / 引脚

装置端建议使用以下输入。
Recommended device-side inputs:

```cpp
const int triggerPin = 2;       // LOW = RUN, HIGH = IDLE
const int resetRequestPin = 3;  // LOW = AUTOHOME/RESET request
```

两者都使用内部上拉。
Both pins should use internal pull-ups.

```cpp
pinMode(triggerPin, INPUT_PULLUP);
pinMode(resetRequestPin, INPUT_PULLUP);
```

继电器触点侧接法。
Relay contact wiring:

```text
RUN relay NO   -> Device Arduino D2
RUN relay COM  -> Device Arduino GND

RESET relay NO -> Device Arduino D3
RESET relay COM -> Device Arduino GND
```

## New Helpers / 新增辅助函数

建议新增以下函数。
Recommended new helper functions:

```cpp
bool readRunSignal() {
  return digitalRead(triggerPin) == LOW;
}

bool readResetRequest() {
  return digitalRead(resetRequestPin) == LOW;
}

void delayWithWatchdog(unsigned long durationMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    wdt_reset();
    delay(50);
  }
}

void moveToRandomStandbyPosition() {
  float randX = random(topleftYX[1], topleftYX[1] + pageWidth);
  float randY = random(topleftYX[0], topleftYX[0] + pageHeight);
  moveToPositionSynced(randX, randY);
}
```

`delayWithWatchdog()` 很重要，因为随机 1 到 10 秒延迟可能超过当前 8 秒 watchdog 时间。
`delayWithWatchdog()` is important because the random 1-to-10-second delay can exceed the current 8-second watchdog timeout.

## Main Loop Shape / 主循环结构

建议主循环改成这个结构。
The main loop should follow this shape:

```cpp
void loop() {
  bool shouldRun = readRunSignal();

  if (shouldRun) {
    if (!wasRunning) {
      delayWithWatchdog(random(1000, 10001));
      swingServo();
      wasRunning = true;
    }

    runOneRandomCycle();
  } else {
    if (wasRunning) {
      moveToRandomStandbyPosition();
      myServo.write(19);
      wasRunning = false;
    }

    if (readResetRequest()) {
      autoHome();
      moveToRandomStandbyPosition();
      myServo.write(19);
    }

    wdt_reset();
    delay(100);
  }
}
```

其中 `runOneRandomCycle()` 可以替代现在的 `randomlyTriggerFunctions()`。
`runOneRandomCycle()` can replace the current `randomlyTriggerFunctions()`.

```cpp
void runOneRandomCycle() {
  delayWithWatchdog(random(3000, 8001));
  moveToRandomPositionAndSwing();
}
```

随机等待范围建议先做成常量，方便现场调试。
The random delay ranges should be constants so they are easy to tune on site.

## Startup Behavior / 开机行为

`setup()` 中 `autoHome()` 后建议立即移动到随机待机点。
After `autoHome()` in `setup()`, the device should immediately move to a random standby point.

```cpp
autoHome();
randomSeed(analogRead(0));
moveToRandomStandbyPosition();
wasRunning = false;
```

这样六台装置开机后不会都停在同一个 Home 点。
This prevents all six devices from staying at the same Home point after startup.

## Periodic Reset / 原周期性重启

建议下一版禁用原来的 10 到 20 轮自动软件重启逻辑。
The next version should disable the original automatic software reboot after 10 to 20 cycles.

原因如下。
Reasons:

- 现在 reset/autohome 由分组控制器在无人时逐台安排。
  Reset/autohome is now scheduled one device at a time by the group controller when the room is empty.
- 原来的自动重启可能在观众还在场时突然触发，影响作品行为。
  The old automatic reboot may trigger while visitors are present, interrupting the work's behavior.
- 当前 `delay(10000)` 与 8 秒 watchdog 有冲突。
  The current `delay(10000)` conflicts with the 8-second watchdog timeout.

watchdog 建议保留，只用于程序卡死时自动恢复。
The watchdog should stay enabled and only act as recovery if the program hangs.

## Known Limitation / 已知限制

本方案继续使用阻塞式动作。
This plan keeps the blocking motion style.

这意味着 `STOP` 和 `AUTOHOME/RESET` 不会打断当前移动或舵机动作，而是在当前动作完成后被处理。
This means `STOP` and `AUTOHOME/RESET` will not interrupt the current movement or servo action; they are handled after the current action completes.

这是目前可接受的，因为作品不需要急停。
This is acceptable for now because the installation does not require emergency stop behavior.
