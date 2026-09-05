# Arduino Code Map

## Production Sketches

- `devices/190cmBar_device_status_led/`
  Current linked 190cmBar device version with D13 status feedback: OFF in IDLE, steady ON in RUN, and non-blocking fast blink during AUTOHOME/RESET.
- `devices/190cmBar_device/`
  Standalone mechanism test. It ignores external RUN/RESET, homes at power-on,
  then repeatedly moves to a random point, pushes the servo, and waits 10 seconds.
- `controllers/presence_group_controller_2devices_sr602/`
  Two-device SR602 version. The SR602 Digital OUT is read on controller D2.
- `controllers/presence_group_controller_2devices_tfluna/`
  Two-device TF-Luna Digital OUT version. The sensor is configured beforehand and uses only 5V, GND and Digital OUT during operation; OUT is read on controller D2.
- `controllers/presence_group_controller/`
  Retained older three-device Presence Group Controller.

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
  `190cmBar_device_no_led_only.ino` is the standalone device version without external RUN/RESET inputs. It homes on startup, runs the original random-motion cycle continuously, retains the cycle-count reboot logic, and removes the LED control path. This is not the linked Presence Group Controller version.
