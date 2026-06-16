# Change Log / 变更记录

这个文件记录工具箱和 Arduino sketches 的有意变更。
This file records intentional changes to the toolbox and Arduino sketches.

## 2026-06-16

- 创建工作工具箱结构。
  Created the working toolbox structure.
- 添加需求文档和通信模型文档。
  Added requirement and communication model documents.
- 将当前文件夹初始化为 Git 仓库。
  Initialized this folder as a Git repository.
- 添加 `arduino/190cmBar_device/190cmBar_device.ino`，作为原始装置 sketch 的工作副本。
  Added `arduino/190cmBar_device/190cmBar_device.ino` as a working copy of the original device sketch.
- 从工作版装置 sketch 中移除 D13 LED 继电器路径。
  Removed the D13 LED relay path from the working device sketch.
- 添加第一版 Arduino Nano R3 presence group controller sketch，包含 LD2410C 输入、三路继电器输出和 active-low 控制器间 Bus。
  Added the first Arduino Nano R3 presence group controller sketch with LD2410C input, three relay outputs, and active-low cross-controller bus.
- 将项目说明文档改为中英双语。
  Rewrote project documentation as bilingual Chinese/English docs.
- 将 presence group controller 的 Bus 语义从瞬时 `localPresence` 改为本地 3 分钟 `localRunRequest`。
  Changed the presence group controller bus semantics from instant `localPresence` to local 3-minute `localRunRequest`.
