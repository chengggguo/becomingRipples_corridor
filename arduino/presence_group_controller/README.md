# Presence Group Controller Firmware

This folder is for the new row-level Arduino sketch.

Planned behavior:

- Read LD2410C digital OUT on D2.
- Keep relays active for 3 minutes after last presence.
- Drive relay channels on D5, D6, and D7.
- Support active-low or active-high relay modules.
- Send RUN/IDLE to three device Arduinos through relay dry contacts.

