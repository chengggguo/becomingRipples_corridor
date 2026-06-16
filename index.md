# Index / 索引

## 中文

### 文档

- `docs/development_requirements.md`
  当前已对齐的开发需求。
- `docs/communication_model.md`
  传感器、分组控制器、继电器和装置 Arduino 之间的硬件与信号模型。
- `docs/bus_options.md`
  两个控制器之间共享房间 presence 状态的 Bus 方案。
- `docs/source_files.md`
  原始文件和目标输出文件说明。
- `docs/device_firmware_trigger_reset_plan.md`
  装置端 `RUN/IDLE` 与 `AUTOHOME/RESET` 当前实现说明。
- `docs/wiring_guide.md`
  现场接线指南。
- `docs/change_log.md`
  项目变更记录。

### Arduino 目标代码

- `arduino/190cmBar_device/`
  每台原始 190cmBar 装置的修改版固件。
- `arduino/presence_group_controller/`
  每排 `Presence Group Controller` 的新固件。

## English

### Documents

- `docs/development_requirements.md`
  Current aligned development requirements.
- `docs/communication_model.md`
  Hardware and signal model between sensors, group controllers, relays, and device Arduinos.
- `docs/bus_options.md`
  Bus options for sharing room-level presence between the two controllers.
- `docs/source_files.md`
  Original files and generated target files.
- `docs/device_firmware_trigger_reset_plan.md`
  Current device-side `RUN/IDLE` and `AUTOHOME/RESET` implementation.
- `docs/wiring_guide.md`
  On-site wiring guide.
- `docs/change_log.md`
  Project change log.

### Arduino Targets

- `arduino/190cmBar_device/`
  Modified firmware for each original 190cmBar device.
- `arduino/presence_group_controller/`
  New firmware for each row-level `Presence Group Controller`.
