# Arduino Code Map

## Production Sketches

- `devices/190cmBar_device/`
  Firmware for each linked 190cmBar device.
- `controllers/presence_group_controller/`
  Firmware for each row-level Presence Group Controller.

## Temporary Tests

- `tests/sensor_pin_test/`
  Safe D2 input test using `INPUT_PULLUP`; D13 turns off when D2 is connected to GND.
- `tests/sensor_serial_read_test/`
  Prints the raw LD2410 OUT value read on D2 and mirrors presence on D13.
- `tests/presence_group_controller_no_hold_test/`
  Presence Group Controller variant without the 3-minute hold, for immediate sensor testing.
- `tests/presence_group_controller_no_bus_relay_test/`
  Presence Group Controller variant with D3 bus disabled, useful for checking local sensor and six relay behavior.
- `tests/presence_group_controller_runtime_reset_test/`
  No-bus test for reset scheduling based on accumulated RUN time, with per-device reset progress tracking.
- `tests/presence_group_controller_runtime_bus_test/`
  Short-timing runtime reset test with D3 bus enabled and an A/B row delay setting.

## Libraries And References

- `libraries/HLK_LD2410_config/`
  Reference Arduino library for reading/configuring LD2410 over UART.

## Legacy

- `legacy/190cmBar_device_no_led_only/`
  Standalone no-LED branch. This is not the current linked six-device firmware.
