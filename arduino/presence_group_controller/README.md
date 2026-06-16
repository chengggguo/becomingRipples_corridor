# Presence Group Controller Firmware

This folder is for the new row-level Arduino sketch.

Current sketch:

- `presence_group_controller.ino`

Planned behavior:

- Read LD2410C digital OUT on D2.
- Keep relays active for 3 minutes after last presence.
- Drive relay channels on D5, D6, and D7.
- Support active-low or active-high relay modules.
- Send RUN/IDLE to three device Arduinos through relay dry contacts.
- Share local presence with the other row controller through an active-low bus on D3.

## Default Wiring

```text
LD2410C OUT -> Nano D2

Nano D3    -> other controller D3
Nano GND   -> other controller GND

Nano D5    -> Relay IN1
Nano D6    -> Relay IN2
Nano D7    -> Relay IN3
Nano 5V    -> Relay VCC
Nano GND   -> Relay GND
```

Relay contact side:

```text
Relay CH1 NO  -> Device 1 Arduino D2
Relay CH1 COM -> Device 1 Arduino GND

Relay CH2 NO  -> Device 2 Arduino D2
Relay CH2 COM -> Device 2 Arduino GND

Relay CH3 NO  -> Device 3 Arduino D2
Relay CH3 COM -> Device 3 Arduino GND
```

## Config Constants

- `SENSOR_ACTIVE_HIGH`: change if the sensor OUT polarity is opposite.
- `RELAY_ACTIVE_LOW`: change if the relay module is active-high.
- `holdTimeMs`: default is `180000UL`, equal to 3 minutes.

