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
