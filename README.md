# Becoming Ripples Corridor Toolbox

## 中文

这个文件夹是 Becoming Ripples 六台装置联动控制更新的工作工具箱。

当前内容：

1. `arduino/devices/190cmBar_device/190cmBar_device.ino`：六台 190cmBar 装置端联动版固件。
2. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：每排 `Presence Group Controller` 的正式 Arduino 程序。
3. `arduino/tests/`：现场排查用的临时测试 sketch。
4. `arduino/libraries/HLK_LD2410_config/`：LD2410 串口调参库参考。
5. `arduino/legacy/`：旧分叉或备用版本。

建议从这些文档开始：

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`

## English

This folder is the working toolbox for the six-device Becoming Ripples control update.

Current contents:

1. `arduino/devices/190cmBar_device/190cmBar_device.ino`: linked-control firmware for the six 190cmBar devices.
2. `arduino/controllers/presence_group_controller/presence_group_controller.ino`: production Arduino sketch for each row-level `Presence Group Controller`.
3. `arduino/tests/`: temporary on-site diagnostic sketches.
4. `arduino/libraries/HLK_LD2410_config/`: reference LD2410 UART configuration library.
5. `arduino/legacy/`: older branches or backup versions.

Start from these documents:

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`
