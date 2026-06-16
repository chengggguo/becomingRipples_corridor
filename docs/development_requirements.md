# Development Requirements

## Confirmed Deliverables

There are two code deliverables:

1. Modified device firmware based on the original `190cmBar_code.ino`.
2. New Arduino firmware for the row-level presence group controller.

## Device Firmware Target

The original 190cmBar device currently runs random movement continuously. It should become externally triggered:

- Add `triggerPin = 2`.
- Configure `triggerPin` as `INPUT_PULLUP`.
- Treat `D2 = LOW` as `RUN`.
- Treat `D2 = HIGH` as `IDLE`.
- On boot, run `autoHome()`, then move to a random standby position.
- When `RUN` first arrives:
  - wait a short random delay;
  - swing the servo once at the current position;
  - then enter the original random movement and servo behavior.
- When `RUN` becomes `IDLE`:
  - do not emergency stop;
  - finish the current blocking action;
  - move to a random standby position;
  - return the servo to its initial angle;
  - wait for the next trigger.
- Disable the old LED random path.
- Fix the local `rebootThreshold` shadowing bug.

## Presence Group Controller Target

Each row has one group controller Arduino. There are two group controllers total, and each one has its own LD2410C or similar presence sensor.

Each group controller should:

- Read its local presence sensor using a digital OUT pin.
- Share its local presence state with the other group controller through a cross-controller bus.
- Treat room presence as `local presence OR remote presence`.
- Keep its three downstream devices in `RUN` for 3 minutes after the last room-level presence.
- Control three relay channels separately, using D5, D6, and D7.
- Allow relay active polarity to be configured with a constant.

## Important V1 Scope

The first version does not need precise synchronization between the six devices.

The first version can keep the existing blocking movement style. Faster interruption can be a later non-blocking state-machine refactor.

The first version does need the two group controllers to share room-level presence, so either row can trigger all six devices.
