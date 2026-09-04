#!/usr/bin/env python3
"""Build and optionally send TF-Luna UART configuration frames.

Dry-run is the default. Add --apply and --port only after commissioning values
have been checked against real empty-room measurements.
"""

from __future__ import annotations

import argparse
import sys
import time
from typing import Iterable


def u16_le(value: int) -> bytes:
    if not 0 <= value <= 0xFFFF:
        raise ValueError(f"value outside uint16 range: {value}")
    return bytes((value & 0xFF, (value >> 8) & 0xFF))


def make_frame(command_id: int, payload: bytes = b"") -> bytes:
    length = 4 + len(payload)
    body = bytes((0x5A, length, command_id)) + payload
    return body + bytes((sum(body) & 0xFF,))


def hex_text(data: bytes) -> str:
    return " ".join(f"{value:02X}" for value in data)


def valid_frames(data: bytes) -> Iterable[bytes]:
    index = 0
    while index + 4 <= len(data):
        if data[index] != 0x5A:
            index += 1
            continue
        length = data[index + 1]
        if length < 4 or index + length > len(data):
            index += 1
            continue
        frame = data[index : index + length]
        if (sum(frame[:-1]) & 0xFF) == frame[-1]:
            yield frame
            index += length
        else:
            index += 1


def read_for(serial_port, duration_seconds: float = 0.5) -> bytes:
    deadline = time.monotonic() + duration_seconds
    received = bytearray()
    while time.monotonic() < deadline:
        waiting = serial_port.in_waiting
        if waiting:
            received.extend(serial_port.read(waiting))
        else:
            time.sleep(0.01)
    return bytes(received)


def build_commands(args: argparse.Namespace):
    if args.amp_threshold % 10:
        raise ValueError("--amp-threshold must be divisible by 10")
    if not 0 <= args.amp_threshold <= 2550:
        raise ValueError("--amp-threshold must be between 0 and 2550")
    if args.dummy_distance <= args.distance + args.zone:
        raise ValueError(
            "--dummy-distance must be greater than --distance + --zone"
        )

    on_off_payload = (
        bytes((args.mode,))
        + u16_le(args.distance)
        + u16_le(args.zone)
        + u16_le(args.delay_in)
        + u16_le(args.delay_out)
    )
    amp_payload = bytes((args.amp_threshold // 10,)) + u16_le(
        args.dummy_distance
    )

    return [
        ("Read firmware version", make_frame(0x01)),
        ("Set Amp threshold and dummy distance", make_frame(0x22, amp_payload)),
        ("Set on/off mode", make_frame(0x3B, on_off_payload)),
        ("Save current settings", make_frame(0x11)),
        ("Read back on/off configuration", make_frame(0x3F, bytes((0x3B,)))),
    ]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Prepare or send TF-Luna on/off-mode configuration."
    )
    parser.add_argument("--port", help="COM5 on Windows or /dev/cu.* on macOS")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--mode", type=int, choices=(1, 2), default=1)
    parser.add_argument("--distance", type=int, default=370, help="Dist in cm")
    parser.add_argument("--zone", type=int, default=20, help="Zone in cm")
    parser.add_argument("--delay-in", type=int, default=0, help="Delay1 in ms")
    parser.add_argument("--delay-out", type=int, default=300, help="Delay2 in ms")
    parser.add_argument("--amp-threshold", type=int, default=100)
    parser.add_argument("--dummy-distance", type=int, default=500, help="cm")
    action = parser.add_mutually_exclusive_group()
    action.add_argument(
        "--apply",
        action="store_true",
        help="Actually write to the connected TF-Luna; default is dry-run.",
    )
    action.add_argument(
        "--verify-only",
        action="store_true",
        help="Read firmware and on/off settings without changing or saving them.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        commands = build_commands(args)
    except ValueError as error:
        print(f"Configuration error: {error}", file=sys.stderr)
        return 2

    selected_commands = (
        [commands[0], commands[-1]] if args.verify_only else commands
    )

    heading = "Read-only TF-Luna frames:" if args.verify_only else "Proposed TF-Luna frames:"
    print(heading)
    for label, frame in selected_commands:
        print(f"  {label}: {hex_text(frame)}")

    if not args.apply and not args.verify_only:
        print("\nDry-run only. Nothing was written.")
        print("Add --port PORT --apply after checking real room measurements.")
        return 0

    if not args.port:
        print("--port is required for --apply or --verify-only", file=sys.stderr)
        return 2

    try:
        import serial
    except ImportError:
        print(
            "pyserial is required: python -m pip install pyserial",
            file=sys.stderr,
        )
        return 2

    print(f"\nOpening {args.port} at {args.baudrate} baud...")
    try:
        with serial.Serial(
            args.port,
            baudrate=args.baudrate,
            bytesize=8,
            parity="N",
            stopbits=1,
            timeout=0.05,
            write_timeout=1,
        ) as device:
            time.sleep(0.3)
            device.reset_input_buffer()

            for label, frame in selected_commands:
                device.reset_input_buffer()
                device.write(frame)
                device.flush()
                response = read_for(device)
                parsed = list(valid_frames(response))

                print(f"\nSent {label}: {hex_text(frame)}")
                if parsed:
                    for reply in parsed:
                        print(f"Reply: {hex_text(reply)}")
                elif response:
                    print(f"Raw reply (no valid 0x5A frame found): {hex_text(response)}")
                else:
                    print("No reply received.")
                time.sleep(0.15)
    except serial.SerialException as error:
        print(f"Serial error: {error}", file=sys.stderr)
        return 1

    if args.verify_only:
        print("\nRead-only verification completed. No settings were changed.")
    else:
        print("\nConfiguration sequence completed.")
        print("Power-cycle the sensor, then run again with --verify-only.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
