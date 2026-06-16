# 190cmBar Device Firmware / 190cmBar 装置端固件

## 中文

这个文件夹用于保存原始 `190cmBar_code.ino` 的修改版。

当前 sketch：

- `190cmBar_device.ino`

当前变更：

- 从原始 `190cmBar_code.ino` 复制而来。
- 移除了旧的 D13 LED 继电器路径和随机 LED 运动分支。

计划行为：

- 开机并运行 `autoHome()`。
- 移动到一个随机待机点。
- 等待 `D2 = LOW`。
- 第一次收到 `RUN` 时，随机延迟并在当前位置舵机动作一次。
- 当 `RUN` 保持有效时，继续原始随机运动逻辑。
- 当进入 `IDLE` 后，完成当前动作并回到随机待机状态。

## English

This folder is for the modified version of the original `190cmBar_code.ino`.

Current sketch:

- `190cmBar_device.ino`

Current changes:

- Copied from the original `190cmBar_code.ino`.
- Removed the old D13 LED relay path and random LED movement branch.

Planned behavior:

- Boot and run `autoHome()`.
- Move to a random standby point.
- Wait for `D2 = LOW`.
- On first `RUN`, delay randomly and swing the servo once.
- Continue original random movement while `RUN` remains active.
- After `IDLE`, finish the current action and return to random standby.
