# Cross-Controller Bus Options

Two row-level group controllers must share presence state.

The logical rule is:

```text
roomPresence = localPresence OR remotePresence
```

When `roomPresence` is true, both controllers turn on their own three relays, so all six 190cmBar devices run.

## Option A: Simple Active-Low Digital Bus

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

## Option B: RS485 Bus

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

## Selected V1 Implementation

The current `presence_group_controller.ino` sketch uses Option A:

```text
Controller A D3 -> Controller B D3
Controller A GND -> Controller B GND
```

The code uses open-drain style behavior:

- local presence: D3 becomes `OUTPUT LOW`;
- no local presence: D3 becomes `INPUT_PULLUP`;
- remote presence is detected when D3 reads `LOW`.

For longer cables, adding an external pull-up resistor such as 4.7kΩ to 5V on the bus line can improve stability.

## Recommendation

Use Option A for the fastest prototype if the two group controller Arduinos are physically close and can share GND.

Use Option B if the installation wiring between the two rows is long, routed near motors/power, or needs stronger noise tolerance.
