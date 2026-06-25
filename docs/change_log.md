# Change Log / 变更记录

## 中文

这个文件记录工具箱和 Arduino sketches 的有意变更。

### 2026-06-16

- 创建工作工具箱结构。
- 添加需求文档和通信模型文档。
- 将当前文件夹初始化为 Git 仓库。
- 添加 `arduino/190cmBar_device/190cmBar_device.ino`，作为原始装置 sketch 的工作副本。
- 从工作版装置 sketch 中移除 D13 LED 继电器路径。
- 添加第一版 Arduino Nano R3 presence group controller sketch，包含 LD2410C 输入、三路继电器输出和 active-low 控制器间 Bus。
- 将项目说明文档改为中英双语。
- 将 presence group controller 的 Bus 语义从瞬时 `localPresence` 改为本地 3 分钟 `localRunRequest`。
- 将 presence group controller 扩展为 6 路继电器：3 路 `RUN`，3 路逐台 `AUTOHOME/RESET` 请求。
- 新增装置端 `RUN/IDLE` 与 `AUTOHOME/RESET` 具体修改方案文档。
- 将同排两台装置 reset 请求之间的默认间隔从 1 分钟改为 10 分钟。
- 新增 `docs/wiring_guide.md`，作为现场接线指南。
- 将所有 Markdown 文档重排为“完整中文在前，完整英文在后”的结构。
- 为 presence group controller 增加 500ms 本地传感器 active debounce，减少瞬时误触发。
- 将 `190cmBar_device.ino` 改为由 `D2 RUN/IDLE` 和 `D3 AUTOHOME/RESET` 控制的状态机。
- 移除装置端旧的 10 到 20 轮自动软件重启逻辑，改为由 presence group controller 在 IDLE 中逐台 reset。
- 为装置端随机等待、舵机动作和步进移动加入 watchdog-safe 延迟与 reset 请求记忆。
- 更新装置端 README 和触发/reset 文档，使其描述当前实现而不是未来计划。
- 更新 `autoHome()`：坐标已知时先回到 HomePoint，再安全反向和 Hall 寻零，并为 homing 增加超时后 watchdog 重启。
- 将装置端异常 `softwareReboot()` 改为通过 watchdog 触发重启。
- 为两排 Presence Group Controller 增加 A/B `resetStartOffsetMs` 错峰配置说明。

### 2026-06-25

- 重排 Arduino 代码目录：正式装置固件放入 `arduino/devices/`，正式控制器固件放入 `arduino/controllers/`，临时测试 sketch 放入 `arduino/tests/`。
- 将 LD2410 串口调参库移入 `arduino/libraries/HLK_LD2410_config/`。
- 将 No LED 单机分叉移入 `arduino/legacy/`。
- 将正式 Presence Group Controller 的 reset 调度改为“每台装置累计 RUN 时长达标后再排队 reset”，并保留 D3 Bus 联动。
- 将正式 Presence Group Controller 的继电器默认极性改为高电平触发，以匹配现场测试的继电器模块。

## English

This file records intentional changes to the toolbox and Arduino sketches.

### 2026-06-16

- Created the working toolbox structure.
- Added requirement and communication model documents.
- Initialized this folder as a Git repository.
- Added `arduino/190cmBar_device/190cmBar_device.ino` as a working copy of the original device sketch.
- Removed the D13 LED relay path from the working device sketch.
- Added the first Arduino Nano R3 presence group controller sketch with LD2410C input, three relay outputs, and active-low cross-controller bus.
- Rewrote project documentation as bilingual Chinese/English docs.
- Changed the presence group controller bus semantics from instant `localPresence` to local 3-minute `localRunRequest`.
- Expanded the presence group controller to six relays: three `RUN` relays and three one-at-a-time `AUTOHOME/RESET` request relays.
- Added a concrete device-side `RUN/IDLE` and `AUTOHOME/RESET` modification plan document.
- Changed the default gap between reset requests for devices in the same row from 1 minute to 10 minutes.
- Added `docs/wiring_guide.md` as an on-site wiring guide.
- Reorganized all Markdown documents so the full Chinese version appears first and the full English version appears after it.
- Added a 500ms local sensor active debounce to the presence group controller to reduce momentary false triggers.
- Changed `190cmBar_device.ino` into a state machine controlled by `D2 RUN/IDLE` and `D3 AUTOHOME/RESET`.
- Removed the old device-side automatic software reboot after 10 to 20 rounds; routine reset is now requested one device at a time by the presence group controller during IDLE.
- Added watchdog-safe delays and reset-request latching during device-side random waits, servo actions, and stepper movement.
- Updated the device README and trigger/reset document so they describe the current implementation instead of a future plan.
- Updated `autoHome()`: when the coordinate is known, move back to HomePoint before safety reverse and Hall seeking, and add homing timeout recovery through watchdog reboot.
- Changed device-side abnormal `softwareReboot()` to trigger reboot through the watchdog.
- Added A/B `resetStartOffsetMs` staggered-reset setup notes for the two Presence Group Controllers.

### 2026-06-25

- Reorganized Arduino code folders: production device firmware is under `arduino/devices/`, production controller firmware is under `arduino/controllers/`, and temporary diagnostic sketches are under `arduino/tests/`.
- Moved the LD2410 UART configuration library into `arduino/libraries/HLK_LD2410_config/`.
- Moved the standalone no-LED branch into `arduino/legacy/`.
- Changed the production Presence Group Controller reset scheduler so each device is queued for reset only after its own accumulated RUN time reaches the threshold, while keeping D3 bus linking.
- Changed the production Presence Group Controller default relay polarity to active-high to match the on-site relay module test.
