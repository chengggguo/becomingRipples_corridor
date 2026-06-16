# 190cmBar Device Firmware

This folder is for the modified version of the original `190cmBar_code.ino`.

Planned behavior:

- Boot and run `autoHome()`.
- Move to a random standby point.
- Wait for `D2 = LOW`.
- On first `RUN`, delay randomly and swing the servo once.
- Continue original random movement while `RUN` remains active.
- After `IDLE`, finish the current action and return to random standby.

