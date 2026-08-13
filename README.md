# Becoming Ripples Corridor Toolbox

## 当前文件与作用

1. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`：当前新增的两装置传感器/分组控制器版本；D5/D6 控制装置 A，D7/D8 控制装置 B。
2. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器版本，没有被两装置版覆盖。
3. `arduino/devices/190cmBar_device/190cmBar_device.ino`：装置端固件，接收 D2 RUN/IDLE 和 D3 AUTOHOME/RESET 信号。
4. `arduino/tests/`：现场排查和短时间验证用的临时测试程序，不是当前正式上传文件。

## 中文

这个文件夹是 Becoming Ripples 六台装置联动控制更新的工作工具箱。

当前内容：

1. `arduino/devices/190cmBar_device/190cmBar_device.ino`：190cmBar 装置端联动版固件。
2. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`：当前两装置 `Presence Group Controller` 程序。
3. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器程序。
4. `arduino/tests/`：现场排查用的临时测试 sketch。
5. `arduino/libraries/HLK_LD2410_config/`：LD2410 串口调参库参考。
6. `arduino/legacy/`：旧分叉或备用版本。

建议从这些文档开始：

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`

## English

This folder is the working toolbox for the six-device Becoming Ripples control update.

Current contents:

1. `arduino/devices/190cmBar_device/190cmBar_device.ino`: linked-control firmware for each 190cmBar device.
2. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`: current two-device Presence Group Controller sketch.
3. `arduino/controllers/presence_group_controller/presence_group_controller.ino`: retained older three-device controller sketch.
4. `arduino/tests/`: temporary on-site diagnostic sketches.
5. `arduino/libraries/HLK_LD2410_config/`: reference LD2410 UART configuration library.
6. `arduino/legacy/`: older branches or backup versions.

Start from these documents:

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`
