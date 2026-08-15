# Becoming Ripples Corridor Toolbox

## 当前文件与作用

1. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`：当前新增的两装置传感器/分组控制器版本；D5/D6 控制装置 A，D7/D8 控制装置 B。
2. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器版本，没有被两装置版覆盖。
3. `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`：新增的 D13 状态灯装置端版本；IDLE 熄灭、RUN 常亮、AUTOHOME/RESET 非阻塞快闪。
4. `arduino/devices/190cmBar_device/190cmBar_device.ino`：保留不变的基础装置端固件，接收 D2 RUN/IDLE 和 D3 AUTOHOME/RESET 信号。
5. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`：无外部 RUN/RESET 的自主运行装置版；保留原始随机运动和按轮数自动重启逻辑，只移除了 LED 控制路径。
6. `arduino/tests/`：现场排查和短时间验证用的临时测试程序，不是当前正式上传文件。

## 中文

这个文件夹是 Becoming Ripples 六台装置联动控制更新的工作工具箱。

当前内容：

1. `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`：带 D13 状态反馈的 190cmBar 装置端联动版固件。
2. `arduino/devices/190cmBar_device/190cmBar_device.ino`：保留的无状态灯基础装置端联动固件。
3. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`：当前两装置 `Presence Group Controller` 程序。
4. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器程序。
5. `arduino/tests/`：现场排查用的临时测试 sketch。
6. `arduino/libraries/HLK_LD2410_config/`：LD2410 串口调参库参考。
7. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`：不包含外部 RUN/RESET 的自主随机运行、无 LED 版本。

建议从这些文档开始：

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`

## English

This folder is the working toolbox for the six-device Becoming Ripples control update.

Current contents:

1. `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`: linked device firmware with D13 status feedback.
2. `arduino/devices/190cmBar_device/190cmBar_device.ino`: retained base linked device firmware without status feedback.
3. `arduino/controllers/presence_group_controller_2devices/presence_group_controller_2devices.ino`: current two-device Presence Group Controller sketch.
4. `arduino/controllers/presence_group_controller/presence_group_controller.ino`: retained older three-device controller sketch.
5. `arduino/tests/`: temporary on-site diagnostic sketches.
6. `arduino/libraries/HLK_LD2410_config/`: reference LD2410 UART configuration library.
7. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`: standalone random-motion version without external RUN/RESET inputs; the LED control path is removed.

Start from these documents:

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`
