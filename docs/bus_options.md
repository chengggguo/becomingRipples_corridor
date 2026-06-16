# Cross-Controller Bus Options / 控制器之间的 Bus 方案

## 中文

两个 row-level group controller 必须共享 presence 状态。

逻辑规则如下：

```text
roomPresence = localPresence OR remotePresence
```

当 `roomPresence` 为真时，两块控制器都会打开自己的三路 `RUN` 继电器，所以六台 190cmBar 装置都会运行。

### 方案 A：简单低电平有效数字 Bus

如果两块 group controller Arduino 距离足够近，并且可以共地，这是最简单的第一版方案。

概念如下：

```text
Controller A bus pin ---- shared RUN bus ---- Controller B bus pin
Shared GND between the two group controllers
Bus HIGH = no shared presence
Bus LOW  = at least one controller reports presence
```

实现要点：

- 使用 `INPUT_PULLUP` 读取 Bus。
- 当控制器检测到本地 presence 时，通过把 Bus 拉低来声明 presence。
- 代码使用 open-drain 风格：引脚只在 `INPUT_PULLUP` 和 `OUTPUT LOW` 之间切换，绝不驱动 `OUTPUT HIGH`。
- 这样任意一块控制器都可以把同一根 Bus 线拉低，不会互相对抗。

优点：

- 非常简单。
- 不需要额外通信模块。
- 对短距离、可控布线足够。

缺点：

- 两块 group controller 之间需要共地。
- 对长线缆或展览现场噪声环境的抗干扰能力较弱。

### 方案 B：RS485 Bus

如果两块控制器距离较远，或者线缆附近噪声较大，RS485 是更稳的方案。

概念如下：

```text
Controller A UART -> RS485 module -> twisted pair A/B -> RS485 module -> Controller B UART
```

优点：

- 更适合长距离走线。
- 抗干扰能力更好。
- 如果之后需要更多消息类型，也更容易扩展。

缺点：

- 需要两个 RS485 收发模块。
- 占用更多引脚，代码也更复杂。
- 需要一个小型消息协议。

### 当前 V1 选择

当前 `presence_group_controller.ino` 使用方案 A：

```text
Controller A D3 -> Controller B D3
Controller A GND -> Controller B GND
```

代码使用 open-drain 风格：

- 本地 3 分钟保持期内：`D3` 变为 `OUTPUT LOW`。
- 本地 3 分钟保持结束：`D3` 变为 `INPUT_PULLUP`。
- 当 `D3` 读到 `LOW` 时，认为对方正在请求 `RUN`。

Bus 只传播本地 hold 状态，不会把 remote request 再次发布回 Bus。这样可以让任意一侧触发六台运行，同时避免两个控制器互相保持导致永远不停止。

如果线缆较长，可以在 Bus 线上加一个外部上拉电阻，例如 4.7kΩ 到 5V，以提高稳定性。

### 建议

如果两块 group controller Arduino 物理距离较近，并且可以共地，快速原型优先使用方案 A。

如果两排之间走线较长、靠近电机或电源线，或者需要更强抗干扰能力，建议改用方案 B。

## English

Two row-level group controllers must share presence state.

The logical rule is:

```text
roomPresence = localPresence OR remotePresence
```

When `roomPresence` is true, both controllers turn on their own three `RUN` relays, so all six 190cmBar devices run.

### Option A: Simple Active-Low Digital Bus

This is the simplest first build if the two group controller Arduinos are close enough and can share GND.

Concept:

```text
Controller A bus pin ---- shared RUN bus ---- Controller B bus pin
Shared GND between the two group controllers
Bus HIGH = no shared presence
Bus LOW  = at least one controller reports presence
```

Implementation notes:

- Use `INPUT_PULLUP` for reading the bus.
- When a controller detects local presence, it asserts the bus by pulling it LOW.
- Use open-drain style behavior in code: switch the pin between `INPUT_PULLUP` and `OUTPUT LOW`, never drive it `OUTPUT HIGH`.
- This allows either controller to pull the same bus line LOW without fighting the other controller.

Pros:

- Very simple.
- No extra communication modules.
- Enough for short, controlled wiring.

Cons:

- Requires shared GND between the two group controllers.
- Less robust for long cable runs or noisy exhibition environments.

### Option B: RS485 Bus

This is the more robust option if the controllers are far apart or the cable route is noisy.

Concept:

```text
Controller A UART -> RS485 module -> twisted pair A/B -> RS485 module -> Controller B UART
```

Pros:

- Better for longer cable runs.
- Better noise immunity.
- More extensible if more messages are needed later.

Cons:

- Requires two RS485 transceiver modules.
- Uses more pins and more code.
- Needs a small message protocol.

### Selected V1 Implementation

The current `presence_group_controller.ino` sketch uses Option A:

```text
Controller A D3 -> Controller B D3
Controller A GND -> Controller B GND
```

The code uses open-drain style behavior:

- Local 3-minute hold active: D3 becomes `OUTPUT LOW`.
- Local 3-minute hold ended: D3 becomes `INPUT_PULLUP`.
- Remote `RUN` request is detected when D3 reads `LOW`.

The bus only publishes the local hold state. It does not re-publish a remote request back onto the bus. This lets either side trigger all six devices while avoiding an infinite mutual hold.

For longer cables, adding an external pull-up resistor such as 4.7kΩ to 5V on the bus line can improve stability.

### Recommendation

Use Option A for the fastest prototype if the two group controller Arduinos are physically close and can share GND.

Use Option B if the installation wiring between the two rows is long, routed near motors/power, or needs stronger noise tolerance.
