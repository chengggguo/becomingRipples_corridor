# 190cmBar Device Firmware

This folder is for the modified version of the original `190cmBar_code.ino`.

Current sketch:

- `190cmBar_device.ino`

Current changes:

- Copied from the original `190cmBar_code.ino`.
- Removed the old D13 LED relay path and random LED movement branch.

Planned behavior:

- Boot and run `autoHome()`.
- Move to a random standby point.
- Wait for `D2 = LOW`.
- On first `RUN`, delay randomly and swing the servo once.
- Continue original random movement while `RUN` remains active.
- After `IDLE`, finish the current action and return to random standby.
