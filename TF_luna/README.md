# TF-Luna 配置准备

## 本目录文件

- `proposed_hex_commands.txt`：基于当前房间假设生成的拟定十六进制命令。
- `configure_tf_luna.py`：Windows/macOS 通用配置工具；默认只打印命令，加入 `--apply` 才会写入传感器。
- `README.md`：CP2102 接线、参数含义、官方 GUI 能力和到货测试流程。

## 当前拟定参数，不是最终现场值

房间长度约 4.2 m，空房间背景墙预计为 410–420 cm。这里暂时按空房最低值 410 cm，并预留 40 cm 背景差值：

```text
Mode       = 1
Dist       = 370 cm
Zone       = 20 cm
Delay1     = 0 ms
Delay2     = 300 ms
Amp 阈值   = 100
Dummy Dist = 500 cm
```

触发与释放：

```text
距离 < 370 cm
→ TF-Luna Pin 6 输出 HIGH
→ Nano D2 连续 HIGH 100 ms
→ 控制器确认 presence

距离处于 370–390 cm
→ 保持此前状态

距离 > 390 cm 持续 300 ms
→ TF-Luna Pin 6 输出 LOW
→ 控制器随后使用原有 90 秒 RUN 保持
```

40 cm 是“相对最低空房背景值的触发余量”，不是 Zone。若把 Dist=370、Zone=40，释放点会变成 410 cm，可能刚好碰到空房最低读数而无法稳定释放。

## 到货后先测量

使用官方 GUI 分别记录：

1. 门关闭、无人。
2. 门打开、无人。
3. 门正在摆动、无人。
4. 人刚进入门口。
5. 人进入约 20 cm。
6. 人进入约 40 cm。

记录最低空房读数 `E_min` 和人刚进入时的读数 `P_entry`。初始计算：

```text
Dist = E_min - 40 cm
Zone 建议先试 10–20 cm
并保证 Dist + Zone 明显小于 E_min
```

如果 `P_entry >= Dist`，40 cm余量与“刚进门立即触发”不能同时满足，需要缩小余量或调整激光束方向。TF-Luna 是单点测距，只判断光束内最近反射距离，不能区分门和人。

## CP2102 配置接线

```text
CP2102 5V/VBUS -> TF-Luna Pin 1 +5V
CP2102 TXD     -> TF-Luna Pin 2 RXD
CP2102 RXD     <- TF-Luna Pin 3 TXD
CP2102 GND     -> TF-Luna Pin 4 GND
TF-Luna Pin 5  -> 悬空（UART 模式）
TF-Luna Pin 6  -> 配置时不接
```

TF-Luna 供电范围是 3.7–5.2 V，串口信号是 3.3 V LVTTL。不要用 CP2102 的 3.3 V电源脚给 TF-Luna 供电。TF-Luna 峰值电流可达 150 mA；不确定 CP2102 板的 5V/VBUS 能力时，使用独立稳定5V电源并共地。

## 官方 GUI 能做什么

北醒官网下载的 `Benewake LiDAR GUI Viewer` 就是 TF 系列官方 Windows 上位机。官方说明确认：

- 可以选择产品、COM 和波特率并连接；
- 可以查看实时距离和 Strength；
- 可以设置帧率、开关串口输出；
- `SAVE SETTING` 保存当前已修改的设置；
- `RESTORE DEFAULT SETTINGS` 恢复出厂值；
- `CUSTOM COMMAND` 向传感器发送协议命令。

官方 GUI 说明明确写到 `SAVE SETTING` 可保存界面中修改过的帧率和输出设置，但没有明确保证这个按钮会替自定义的 on/off 五参数完成持久化。它不会自动生成 Dist、Zone、Delay1、Delay2。当前截图版本只有三个 Param 输入框，官方 GUI 文档也没有证明它提供完整的 TF-Luna on/off 五参数表单。因此：

1. 优先用 GUI 测量实际距离和读取固件版本。
2. 如果 `CUSTOM COMMAND` 可以直接输入并发送完整 HEX 帧，可以发送本目录命令；写完 `0x3B` 后再明确发送保存帧 `5A 04 11 6F`，不要只依赖 GUI 的保存按钮。
3. 重新上电后发送读取帧 `5A 05 3F 3B D9`，确认参数仍然存在。
4. 如果命令框不能手动输入完整帧，使用本目录 Python 工具或 CoolTerm HEX Send。

官方资料：

- [TF-Luna 产品下载页](https://en.benewake.com/DataDownload/index_pid_20_lcid_21.html)
- [TF-Luna 用户手册与串口协议](https://en.benewake.com/uploadfiles/2025/04/20250430174515390.pdf)
- [北醒 BW_TFDS GUI 官方说明](https://en.benewake.com/uploadfiles/2025/04/20250430175630717.pdf)

## 使用 Python 工具

安装 pySerial：

Windows：

```powershell
py -m pip install pyserial
```

macOS：

```bash
python3 -m pip install pyserial
```

只检查拟定帧，不连接、不写入：

```powershell
python configure_tf_luna.py
```

Windows 实际写入示例：

```powershell
python configure_tf_luna.py --port COM5 --apply
```

macOS 实际写入示例：

```bash
python3 configure_tf_luna.py --port /dev/cu.SLAB_USBtoUART --apply
```

现场测量后可以覆盖参数：

```powershell
python configure_tf_luna.py --port COM5 --distance 365 --zone 15 --delay-in 0 --delay-out 300 --apply
```

脚本发送有效校验和。TF-Luna 默认可以忽略下行校验，但发送正确校验和可兼容已经启用校验检查的设备。

## 正式安装

配置、保存和断电复查完成后，移除 CP2102，只保留：

```text
TF-Luna Pin 1 +5V         -> 稳定5V
TF-Luna Pin 4 GND         -> Nano GND
TF-Luna Pin 6 Digital OUT -> Nano D2
Nano D2                   -> 约10kΩ下拉 -> Nano GND
```

Nano 固件采用 `SENSOR_ACTIVE_HIGH = true`，必须与 Mode=1 保持一致。Uno 装置仍只通过继电器接收 RUN/RESET，因此 TF-Luna 参数变化不要求修改装置代码。
