# TF-Luna 测距与配置向导

本目录提供一套 Windows/macOS 共用的 TF-Luna 测距、参数生成、写入和验证工具。日常使用从两个双击启动脚本之一进入；两个启动脚本调用同一份 Python 核心，所以两套系统上的计算和 TF-Luna 命令完全相同。

## 应该打开哪个文件

| 系统 | 双击文件 | 作用 |
|---|---|---|
| Windows | `Start_TF_Luna_Wizard_Windows.bat` | 打开命令行，检查 Python 后启动向导 |
| macOS | `Start_TF_Luna_Wizard_macOS.command` | 打开 Terminal，检查 Python 后启动向导 |
| 两个平台共用 | `configure_tf_luna.py` | 执行测距、计算 HEX、串口写入和验证的核心程序 |

其他文件：

- `proposed_hex_commands.txt`：当前拟定参数对应的十六进制命令，供人工核对。
- `tf_luna_measurements_日期时间.csv`：真机测距产生的原始数据。
- `tf_luna_demo_measurements_日期时间.csv`：DEMO 模拟数据，不能作为现场参数。

## 两个双击启动脚本

### Windows 启动脚本

文件：`Start_TF_Luna_Wizard_Windows.bat`

执行逻辑：

1. 把命令行切换为 UTF-8，避免中文乱码。
2. 自动进入脚本所在的 `TF_luna` 目录。
3. 优先寻找 Windows Python Launcher：`py -3`。
4. 检查 Python 是否为 3.8 或更高版本。
5. 如果没有 `py`，再尝试 `python`。
6. 找到合格的 Python 后运行 `configure_tf_luna.py --wizard`。
7. 向导结束后等待按键关闭，错误信息不会一闪而过。
8. 没有合格的 Python 时显示官方下载地址，不擅自安装 Python。

Windows 使用方法：直接双击 `Start_TF_Luna_Wizard_Windows.bat`。

### macOS 启动脚本

文件：`Start_TF_Luna_Wizard_macOS.command`

执行逻辑：

1. 自动进入脚本所在的 `TF_luna` 目录。
2. 检查系统能否找到 `python3`。
3. 检查 Python 是否为 3.8 或更高版本。
4. 找到合格的 Python 后运行同一份 `configure_tf_luna.py --wizard`。
5. 向导结束后等待按 Return 关闭 Terminal。
6. 没有合格的 Python 时显示官方 macOS 下载地址。

macOS 使用方法：双击 `Start_TF_Luna_Wizard_macOS.command`。第一次被系统阻止时，在 Finder 中右键选择“打开”。如果文件失去可执行权限，运行：

```bash
chmod +x Start_TF_Luna_Wizard_macOS.command
```

## 共用 Python 核心流程

两个启动脚本最终进入同一个 `configure_tf_luna.py`。启动后选择：

```text
1 = 真机模式：连接 CP2102 和 TF-Luna
2 = DEMO：无传感器演示，不打开串口
```

总流程：

```text
检查 Python/pySerial
→ 选择真机或 DEMO
→ 读取或模拟六个现场场景
→ 统计距离并生成 CSV
→ 给出 Dist 参考值
→ 固定 Mode=1，并依次询问四项可调参数
→ 转换成 TF-Luna HEX
→ 输入 WRITE 确认
→ 写入、保存、立即读回
→ 断电重启
→ 只读验证
```

真机模式需要 pySerial。缺少时向导会询问是否通过当前 Python 的 pip 为当前用户安装；拒绝会安全退出。

## DEMO 无硬件模式

DEMO 用于 TF-Luna 到货前熟悉整个流程。它与真机模式共用参数计算、HEX 构造、确认和解码代码，但串口层被替换为模拟层。

DEMO 保证：

- 不枚举或打开 COM/`/dev/cu.*`。
- 不需要 CP2102、TF-Luna 或 pySerial。
- 不向任何硬件发送字节。
- 所有模拟输出都带 `[DEMO]`。
- CSV 文件名带 `demo`。
- 模拟写入后生成模拟读回帧，并用正式解码逻辑核对参数。

双击任一启动脚本后选择 `2`。也可以直接运行：

Windows：

```powershell
py configure_tf_luna.py --demo
```

macOS：

```bash
python3 configure_tf_luna.py --demo
```

模拟距离只能演示流程，不能作为正式 Dist 或 Zone。

## 真机模式流程

### 1. 自动识别端口

向导枚举串口并优先检查 CP210x：

1. 被动监听 TF 系列默认的 `59 59` 测距帧。
2. 没有测距帧时发送只读固件版本查询。
3. 唯一匹配时自动选择。
4. 多个匹配时让用户选择。
5. 无法确认时列出端口并允许手动输入。

扫描不会修改 TF-Luna 参数。但打开其他 Arduino 的串口可能令其复位一次；配置时最好只连接 CP2102 和 TF-Luna，并关闭北醒 GUI、Arduino Serial Monitor 等串口程序。

### 2. 现场参考测距

向导可依次采样：

1. 门关闭、无人。
2. 门打开、无人。
3. 门正在摆动、无人。
4. 人刚进入门口。
5. 人进入约 20cm。
6. 人进入约 40cm。

用户摆好状态后按 Enter，每个场景采样 5 秒。脚本会：

- 校验 `59 59` 帧头和 checksum。
- 读取距离、Amp 和温度。
- 排除 `Amp < 100` 或 `Amp = 65535` 的不可靠距离。
- 显示有效帧数以及距离最小值、中位数、最大值。
- 保存全部可靠数据到带时间戳的 CSV。
- 以空场景最低值 `E_min - 40cm` 作为 Dist 输入默认值。
- 提示门摆动误触发或人在门口不能及时触发的风险。

脚本不会自动识别门的物理状态；场景标签由用户摆好现场后按 Enter 指定。测量结束后选择“不继续配置”，即可只测距退出，不修改 TF-Luna。

### 3. 固定 Mode，并输入四项参数

协议包含五项参数，但本项目不允许现场选择 Mode。脚本固定：


```text
Mode = 1
检测到近距离目标 → TF-Luna Pin 6 输出 HIGH
Nano D2 接收到 HIGH → presence 有效
```

这是因为 Nano 固件已经设置 `SENSOR_ACTIVE_HIGH=true`。如果误选 Mode 2，近距离目标会输出 LOW，与 Nano 当前逻辑相反，所以向导不再提供这个选项。

用户只需依次输入：

1. `Dist / 触发距离（厘米）`：实测距离小于此值时判定有人。
2. `Zone / 释放回差（厘米）`：离开距离必须大于 `Dist + Zone`，避免临界位置反复跳变。这里的 Zone 不是房间背景预留的 40cm。
3. `Delay1 / 进入确认延迟（毫秒）`：近距离条件连续保持多久，Pin 6 才输出 HIGH。当前默认 0ms，因为 Nano 端还会做一次 100ms HIGH 确认。
4. `Delay2 / 离开确认延迟（毫秒）`：远距离条件连续保持多久，Pin 6 才恢复 LOW。当前默认 300ms。

直接按 Enter 接受方括号中的默认值。最终逻辑为：

```text
距离 < Dist
→ Pin 6 输出 HIGH

Dist ≤ 距离 ≤ Dist + Zone
→ 保持此前输出，避免临界抖动

距离 > Dist + Zone 并持续 Delay2
→ Pin 6 输出 LOW
```

Mode 由脚本固定，不需要用户判断 HIGH/LOW。

### 4. 生成 HEX 与安全确认

脚本把十进制参数转换为官方协议需要的小端序字节并计算 checksum，同时自动使用：

```text
Amp threshold = 100
Dummy Dist    = 至少大于 Dist + Zone 50cm
```

这是为了降低弱信号虚拟距离造成的误触发。显示全部 HEX 后，只有准确输入大写 `WRITE` 才会继续，其他输入全部取消。

### 5. 写入、保存与立即读回

确认后依次发送：

1. 读取固件版本。
2. 设置 Amp threshold 和 Dummy Dist。
3. 写入固定的 Mode=1，以及用户输入的 Dist、Zone、Delay1、Delay2。
4. 发送保存命令 `5A 04 11 6F`。
5. 读取 On/Off 参数。
6. 把返回帧解码为十进制并显示。

Python 不是刷写 TF-Luna 固件；它只通过 CP2102 发送官方 UART 配置命令。

### 6. 断电后只读验证

写入完成后，向导提示给 TF-Luna 断电重启。重新连接并按 Enter 后：

1. 确认原串口，必要时重新扫描。
2. 只发送固件查询和参数读取命令。
3. 不发送设置或保存命令。
4. 解码并显示重新上电后的参数。

断电后仍读到相同参数，才视为保存成功。

## CP2102 配置接线

```text
CP2102 5V/VBUS → TF-Luna Pin 1 +5V
CP2102 TXD     → TF-Luna Pin 2 RXD
CP2102 RXD     ← TF-Luna Pin 3 TXD
CP2102 GND     → TF-Luna Pin 4 GND
TF-Luna Pin 5  → 悬空，保持 UART
TF-Luna Pin 6  → 配置时不接
```

注意：

- TX/RX 必须交叉。
- TF-Luna 串口为 3.3V LVTTL，不是 ±12V RS-232。
- TF-Luna 供电范围为 3.7–5.2V，不要用 CP2102 的 3.3V 电源脚供电。
- 峰值电流可达 150mA；不确定 CP2102 的 5V/VBUS 能力时，使用独立稳定 5V 电源并共地。

## 当前拟定参数

```text
Mode       = 1
Dist       = 370 cm
Zone       = 20 cm
Delay1     = 0 ms
Delay2     = 300 ms
Amp        = 100
Dummy Dist = 500 cm
```

假设空房最低距离为 410cm，则触发点为 370cm、保持区间为 370–390cm、释放点为大于 390cm并持续 300ms。这只是到货前拟定值，最终应以六场景测量为准。

如果门与人进入激光束后产生相似距离，单点 TF-Luna 无法区分二者，需要调整安装方向，而不是只靠参数解决。

## 官方协议依据与 GUI 边界

TF-Luna 出厂默认 UART 数据格式为：

```text
59 59
Dist_L Dist_H
Amp_L Amp_H
Temp_L Temp_H
Checksum
```

脚本与官方 GUI 读取的是同一数据源。如果传感器已改为 PIX、毫米或时间戳格式，或者 UART 输出关闭，脚本不会猜测，而会报告没有有效 `59 59` 帧；此时用官方 GUI 检查或恢复输出格式。

官方资料：

- [TF-Luna 用户手册与串口协议](https://en.benewake.com/uploadfiles/2025/04/20250430174515390.pdf)
- [TF-Luna 产品下载页](https://en.benewake.com/DataDownload/index_pid_20_lcid_21.html)
- [北醒 BW_TFDS GUI 官方说明](https://en.benewake.com/uploadfiles/2025/04/20250430175630717.pdf)

官方 GUI 可以查看距离、设置帧率和输出状态，并发送部分自定义命令；现有官方说明没有证明截图版本提供完整的 On/Off 表单，因此本工具负责固定正确极性，并生成、发送、保存和读回完整参数。

## 非交互命令

一般使用双击向导即可。需要脚本化时可使用：

只打印拟定 HEX：

```powershell
python configure_tf_luna.py
```

自动找端口并只读验证：

```powershell
python configure_tf_luna.py --verify-only
```

Windows 指定端口写入：

```powershell
python configure_tf_luna.py --port COM5 --distance 365 --zone 15 --delay-in 0 --delay-out 300 --apply
```

macOS 指定端口写入：

```bash
python3 configure_tf_luna.py --port /dev/cu.SLAB_USBtoUART --distance 365 --zone 15 --delay-in 0 --delay-out 300 --apply
```

## 正式安装接线

配置、保存和断电验证完成后移除 CP2102：

```text
TF-Luna Pin 1 +5V         → 稳定 5V
TF-Luna Pin 4 GND         → Nano GND
TF-Luna Pin 6 Digital OUT → Nano D2
Nano D2                   → 约 10kΩ 下拉 → Nano GND
```

TF-Luna 只向 Nano D2 提供 presence 信号。Nano 再通过继电器控制两台装置，装置侧代码不直接读取 TF-Luna。
