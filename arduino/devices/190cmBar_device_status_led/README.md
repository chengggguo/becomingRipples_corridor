# 190cmBar Device Firmware — D13 Status LED Version

## 文件与作用

- `190cmBar_device_status_led.ino`：以正式联动装置固件为基础，保留 D2 RUN/IDLE、D3 AUTOHOME/RESET、步进、舵机、Hall、watchdog 和随机运动逻辑，并增加 D13 板载 LED 状态反馈。
- `README.md`：说明本版本的文件用途、灯光语义、实现方式和更新历史。

旧文件 `arduino/devices/190cmBar_device/190cmBar_device.ino` 保留不变。需要 D13 状态反馈时，应上传本目录中的 `190cmBar_device_status_led.ino`。

## D13 状态反馈

```text
D13 熄灭    -> IDLE
D13 常亮    -> RUN
D13 150ms间隔快闪 -> 正在执行启动归零或 D3 请求触发的 AUTOHOME/RESET
```

D3 请求如果在 RUN 中出现，只会先记录为 `pendingReset`；此时 D13 继续常亮。装置真正进入 IDLE 并开始执行 `autoHome()` 后，D13 才开始快闪。归零及随机待机移动完成后，D13 熄灭。

## 非阻塞实现

LED 闪烁通过 `millis()` 判断时间，没有增加用于闪灯的 `delay()`。状态更新被插入现有的 watchdog 延时、步进运动和左右 Hall 寻零循环，因此 AUTOHOME 期间仍能持续闪烁，同时不额外阻塞机械控制流程。

## 更新历史

### 2026-08-15

- 从正式联动装置固件派生 D13 状态灯版本，不覆盖旧文件。
- 新增 IDLE 熄灭、RUN 常亮、AUTOHOME/RESET 快闪。
- 在长时间循环中加入非阻塞状态灯刷新。
