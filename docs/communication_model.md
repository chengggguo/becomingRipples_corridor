# Communication Model

## V1 Model

The room has two row-level group controller Arduinos.

Each group controller has:

- one local LD2410C or similar presence sensor;
- three downstream relay channels;
- one cross-controller bus link to the other group controller.

The key behavior is:

```text
local presence OR remote presence
-> room RUN
-> both row controllers turn on their relay channels
-> all six 190cmBar devices run
```

## Local Row Path

Inside each row, the trigger path is:

```text
LD2410C OUT
-> row-level group Arduino
-> three relay channels
-> three device Arduinos' D2 trigger pins
```

## Row-Level Relay Control

Each row-level Arduino directly reads its local presence sensor.

It then controls three relay inputs:

```text
Group Arduino D5 -> Relay IN1
Group Arduino D6 -> Relay IN2
Group Arduino D7 -> Relay IN3
```

The relay contact side connects to each corresponding device:

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

When the relay closes, D2 is pulled to the device's own GND, so the device reads `LOW` and runs.

## Cross-Controller Bus

The two group controller Arduinos must communicate room-level presence.

If either controller sees presence, it tells the other controller through the bus. Both controllers then keep their own three relays active, so all six devices run.

The current controller sketch uses the simple active-low digital bus on D3. See `docs/bus_options.md`.

## Device Communication Boundary

The group controller does not exchange serial messages with the three 190cmBar device Arduinos.

It gives each device an isolated RUN/IDLE signal through a relay dry contact.
