# Becoming Ripples Corridor Toolbox

## 当前文件与作用

1. `arduino/controllers/presence_group_controller_2devices_sr602/presence_group_controller_2devices_sr602.ino`：SR602 两装置控制器；SR602 OUT 接控制器 D2。
2. `arduino/controllers/presence_group_controller_2devices_tfluna/presence_group_controller_2devices_tfluna.ino`：TF-Luna Digital OUT 两装置控制器；TF-Luna 预配置后以 5V、GND、Digital OUT 三线运行，OUT 接控制器 D2。
3. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器版本，没有被两装置版覆盖。
4. `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`：新增的 D13 状态灯装置端版本；IDLE 熄灭、RUN 常亮、AUTOHOME/RESET 非阻塞快闪；同目录 `README.md` 包含无装置硬件时的四按钮临时接线与操作方法。
5. `arduino/devices/190cmBar_device/190cmBar_device.ino`：保留不变的基础装置端固件，接收 D2 RUN/IDLE 和 D3 AUTOHOME/RESET 信号。
6. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`：无外部 RUN/RESET 的自主运行装置版；保留原始随机运动和按轮数自动重启逻辑，只移除了 LED 控制路径。
7. `arduino/tests/`：现场排查和短时间验证用的临时测试程序，不是当前正式上传文件。

## 中文

这个文件夹是 Becoming Ripples 六台装置联动控制更新的工作工具箱。

当前内容：

1. `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`：带 D13 状态反馈的 190cmBar 装置端联动版固件。
2. `arduino/devices/190cmBar_device/190cmBar_device.ino`：保留的无状态灯基础装置端联动固件。
3. `arduino/controllers/presence_group_controller_2devices_sr602/presence_group_controller_2devices_sr602.ino`：SR602 两装置控制器。
4. `arduino/controllers/presence_group_controller_2devices_tfluna/presence_group_controller_2devices_tfluna.ino`：TF-Luna 三线 Digital OUT 两装置控制器。
5. `arduino/controllers/presence_group_controller/presence_group_controller.ino`：保留的旧三装置控制器程序。
6. `arduino/tests/`：现场排查用的临时测试 sketch。
7. `arduino/libraries/HLK_LD2410_config/`：LD2410 串口调参库参考。
8. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`：不包含外部 RUN/RESET 的自主随机运行、无 LED 版本。

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
3. `arduino/controllers/presence_group_controller_2devices_sr602/presence_group_controller_2devices_sr602.ino`: two-device SR602 controller.
4. `arduino/controllers/presence_group_controller_2devices_tfluna/presence_group_controller_2devices_tfluna.ino`: two-device TF-Luna three-wire Digital OUT controller.
5. `arduino/controllers/presence_group_controller/presence_group_controller.ino`: retained older three-device controller sketch.
6. `arduino/tests/`: temporary on-site diagnostic sketches.
7. `arduino/libraries/HLK_LD2410_config/`: reference LD2410 UART configuration library.
8. `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`: standalone random-motion version without external RUN/RESET inputs; the LED control path is removed.

Start from these documents:

- `index.md`
- `docs/development_requirements.md`
- `docs/communication_model.md`
- `docs/wiring_guide.md`
