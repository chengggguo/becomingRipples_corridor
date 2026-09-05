# Becoming Ripples Corridor

本仓库保存《洋·洋》作品中的人体传感器、双装置联动、190cmBar 装置本体以及 TF-Luna 配置工具。本文只说明各文件负责什么；接线和运行细节见对应目录内的 README。

## 当前正式代码

### 传感器控制器：二选一

- `arduino/controllers/presence_group_controller_2devices_tfluna/presence_group_controller_2devices_tfluna.ino`
  - Arduino Nano 双装置控制器，读取 TF-Luna Pin 6 Digital OUT。
  - Nano D5/D6 控制装置 A 的 RUN/RESET，D7/D8 控制装置 B 的 RUN/RESET。

- `arduino/controllers/presence_group_controller_2devices_sr602/presence_group_controller_2devices_sr602.ino`
  - Arduino Nano 双装置控制器，读取 SR602 数字输出。
  - 继电器输出和双装置调度与 TF-Luna 版本对应，传感器确认时间不同。

实际安装根据传感器选择其中一个，不要同时上传。

### 装置控制器

- `arduino/devices/190cmBar_device_status_led/190cmBar_device_status_led.ino`
  - 当前正式的 Arduino Uno 装置固件。
  - A1 是低电平有效的 RUN 输入，A2 是低电平有效的 RESET/AUTOHOME 输入。
  - D13 显示 IDLE、RUN 和 AUTOHOME 状态。
  - 控制步进电机、舵机和 D11/D12 Hall 传感器。

## TF-Luna 测距和配置工具

- `TF_luna/Start_TF_Luna_Wizard_Windows.bat`
  - Windows 双击启动入口，检查 Python 后打开 TF-Luna 向导。

- `TF_luna/Start_TF_Luna_Wizard_macOS.command`
  - macOS 双击启动入口，检查 Python 后打开同一套向导。

- `TF_luna/configure_tf_luna.py`
  - Windows/macOS 共用核心程序。
  - 支持真机测距、自动找端口、四项参数输入、固定 Mode 1、HEX 生成、写入、保存和断电验证。
  - 支持完全不访问串口的 DEMO 模式。

- `TF_luna/proposed_hex_commands.txt`
  - 当前拟定参数对应的 TF-Luna 十六进制命令，供人工核对。

TF-Luna 工具的完整流程见 `TF_luna/README.md`。

## 保留版本

- `arduino/controllers/presence_group_controller/presence_group_controller.ino`
  - 保留的旧三装置 Presence Group Controller。

- `arduino/devices/190cmBar_device/190cmBar_device.ino`
  - 保留的无 D13 状态灯装置联动版，使用 D2 RUN 和 D3 RESET。

- `arduino/legacy/190cmBar_device_no_led_only/190cmBar_device_no_led_only.ino`
  - 不包含外部 RUN/RESET 的自主随机运行版本。

## 临时测试代码

以下文件用于现场排查，不是正式安装固件：

- `arduino/tests/sensor_pin_test/sensor_pin_test.ino`
  - 检查控制器 D2 数字输入和 D13 指示灯。

- `arduino/tests/sensor_serial_read_test/sensor_serial_read_test.ino`
  - 在 Serial Monitor 显示 LD2410 数字 OUT，并用 D13 显示 presence。

- `arduino/tests/presence_group_controller_no_hold_test/presence_group_controller_no_hold_test.ino`
  - 去掉长时间 RUN 保持，用于观察即时传感器响应。

- `arduino/tests/presence_group_controller_no_bus_relay_test/presence_group_controller_no_bus_relay_test.ino`
  - 关闭 D3 联动 Bus，单独测试本地传感器和继电器。

- `arduino/tests/presence_group_controller_runtime_reset_test/presence_group_controller_runtime_reset_test.ino`
  - 使用缩短时间测试单台装置累计运行和 RESET 队列。

- `arduino/tests/presence_group_controller_runtime_bus_test/presence_group_controller_runtime_bus_test.ino`
  - 使用缩短时间同时测试累计运行、RESET 队列和 D3 Bus。

## 参考代码与详细文档

- `arduino/libraries/HLK_LD2410_config/`
  - LD2410 UART 配置参考库，不是当前 TF-Luna 正式方案。

- `arduino/README.md`
  - Arduino 目录代码索引。

- `docs/`
  - 旧三装置系统的设计、接线、通信和更新记录；当前双装置接线以两个双装置控制器目录及正式装置目录内 README 为准。
