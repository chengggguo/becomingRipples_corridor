#!/usr/bin/env python3
"""Build and optionally send TF-Luna UART configuration frames.

Dry-run is the default. Add --apply and --port only after commissioning values
have been checked against real empty-room measurements.
"""

from __future__ import annotations

import argparse
import csv
from datetime import datetime
import importlib
from pathlib import Path
import statistics
import subprocess
import sys
import time
from typing import Iterable


MINIMUM_PYTHON = (3, 8)


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


def has_measurement_frame(data: bytes) -> bool:
    """Return True when a valid TF-series 0x59 0x59 measurement frame exists."""
    for index in range(max(0, len(data) - 8)):
        frame = data[index : index + 9]
        if (
            len(frame) == 9
            and frame[:2] == b"\x59\x59"
            and (sum(frame[:8]) & 0xFF) == frame[8]
        ):
            return True
    return False


def measurement_frames(data: bytes):
    """Decode official TF-Luna default 9-byte/cm UART frames."""
    index = 0
    while index + 9 <= len(data):
        frame = data[index : index + 9]
        if (
            frame[:2] == b"\x59\x59"
            and (sum(frame[:8]) & 0xFF) == frame[8]
        ):
            raw_temperature = int.from_bytes(frame[6:8], "little")
            yield {
                "distance_cm": int.from_bytes(frame[2:4], "little"),
                "strength": int.from_bytes(frame[4:6], "little"),
                "temperature_c": raw_temperature / 8.0 - 256.0,
            }
            index += 9
        else:
            index += 1


def load_pyserial(offer_install: bool = False):
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        if offer_install:
            answer = input(
                "pySerial is not installed. Install it now? [Y/n]: "
            ).strip().lower()
            if answer in ("", "y", "yes"):
                print("Installing pySerial for the current user...")
                result = subprocess.run(
                    [sys.executable, "-m", "pip", "install", "--user", "pyserial"],
                    check=False,
                )
                if result.returncode == 0:
                    importlib.invalidate_caches()
                    try:
                        import serial
                        from serial.tools import list_ports
                        return serial, list_ports
                    except ImportError:
                        pass
        print(
            "pyserial is required: python -m pip install pyserial",
            file=sys.stderr,
        )
        return None, None
    return serial, list_ports


def probe_tf_luna(serial_module, port_name: str, baudrate: int):
    """Probe without changing settings: listen, then request firmware version."""
    try:
        with serial_module.Serial(
            port_name,
            baudrate=baudrate,
            bytesize=8,
            parity="N",
            stopbits=1,
            timeout=0.05,
            write_timeout=0.5,
        ) as device:
            time.sleep(0.2)
            passive_data = read_for(device, 0.45)
            if has_measurement_frame(passive_data):
                return True, "valid 0x59 measurement stream"

            device.reset_input_buffer()
            device.write(make_frame(0x01))
            device.flush()
            response = read_for(device, 0.55)
            if any(frame[2] == 0x01 for frame in valid_frames(response)):
                return True, "firmware-version reply"
            return False, "no TF-Luna frame"
    except (serial_module.SerialException, OSError) as error:
        return False, str(error)


def choose_detected_port(serial_module, list_ports_module, baudrate: int):
    ports = list(list_ports_module.comports())
    if not ports:
        print("No serial ports were found.", file=sys.stderr)
        return None

    # CP210x adapters first, then other serial ports. Opening another Arduino's
    # serial port can reset it, so the scan reports every port it touches.
    ports.sort(
        key=lambda item: (
            "cp210" not in f"{item.description} {item.manufacturer}".lower(),
            item.device,
        )
    )
    print("\nScanning serial ports (read-only probe):")
    matches = []
    for info in ports:
        print(f"  {info.device}: {info.description} ... ", end="", flush=True)
        matched, reason = probe_tf_luna(serial_module, info.device, baudrate)
        print("TF-Luna found" if matched else f"not confirmed ({reason})")
        if matched:
            matches.append(info.device)

    if len(matches) == 1:
        print(f"Automatically selected {matches[0]}.")
        return matches[0]

    if len(matches) > 1:
        print("More than one TF-compatible stream replied:")
        for number, port_name in enumerate(matches, 1):
            print(f"  {number}. {port_name}")
        while True:
            raw = input("Choose port number: ").strip()
            if raw.isdigit() and 1 <= int(raw) <= len(matches):
                return matches[int(raw) - 1]
            print("Please enter one of the listed numbers.")

    print("\nTF-Luna was not identified automatically.")
    print("Available ports: " + ", ".join(info.device for info in ports))
    raw = input("Enter a port manually, or press Enter to cancel: ").strip()
    return raw or None


def prompt_int(label: str, default: int, minimum: int, maximum: int) -> int:
    while True:
        raw = input(f"{label} [{default}]: ").strip()
        if not raw:
            return default
        try:
            value = int(raw)
        except ValueError:
            print("Please enter an integer.")
            continue
        if minimum <= value <= maximum:
            return value
        print(f"Enter a value from {minimum} to {maximum}.")


def capture_scene(serial_module, args: argparse.Namespace, label: str, seconds=5.0):
    print(f"正在采样 {seconds:.0f} 秒：{label}")
    try:
        with serial_module.Serial(
            args.port,
            baudrate=args.baudrate,
            bytesize=8,
            parity="N",
            stopbits=1,
            timeout=0.05,
            write_timeout=1,
        ) as device:
            time.sleep(0.2)
            device.reset_input_buffer()
            raw = read_for(device, seconds)
    except (serial_module.SerialException, OSError) as error:
        print(f"读取失败：{error}")
        return []

    decoded = list(measurement_frames(raw))
    reliable = [
        sample
        for sample in decoded
        if sample["strength"] >= 100 and sample["strength"] != 65535
    ]
    if not decoded:
        print("没有收到有效的 59 59 测距帧。")
        print("请确认串口输出已开启，并且输出格式为出厂默认 9-byte/cm。")
        return []
    if not reliable:
        print(f"收到 {len(decoded)} 帧，但全部信号强度不可靠（Amp < 100 或 65535）。")
        return []

    distances = [sample["distance_cm"] for sample in reliable]
    strengths = [sample["strength"] for sample in reliable]
    print(
        f"有效 {len(reliable)}/{len(decoded)} 帧；"
        f"距离 min/median/max = {min(distances)}/"
        f"{statistics.median(distances):.1f}/{max(distances)} cm；"
        f"Amp 中位数 = {statistics.median(strengths):.0f}"
    )
    return reliable


def save_measurements(records) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_path = Path(__file__).resolve().parent / f"tf_luna_measurements_{timestamp}.csv"
    with output_path.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=("scene", "sample", "distance_cm", "strength", "temperature_c"),
        )
        writer.writeheader()
        writer.writerows(records)
    return output_path


def run_measurement_wizard(serial_module, args: argparse.Namespace):
    print("\n现场参考测距不会修改 TF-Luna 设置。")
    print("每个场景准备好后按 Enter；输入 SKIP 可跳过该场景。")
    scenes = (
        ("门关闭、无人", True),
        ("门打开、无人", True),
        ("门正在摆动、无人", True),
        ("人刚进入门口", False),
        ("人进入约 20 cm", False),
        ("人进入约 40 cm", False),
    )
    summaries = []
    records = []
    for label, is_empty in scenes:
        answer = input(f"\n准备“{label}”，按 Enter 采样，或输入 SKIP：").strip().upper()
        if answer == "SKIP":
            continue
        samples = capture_scene(serial_module, args, label)
        if not samples:
            continue
        distances = [sample["distance_cm"] for sample in samples]
        summaries.append(
            {
                "scene": label,
                "is_empty": is_empty,
                "minimum": min(distances),
                "median": statistics.median(distances),
                "maximum": max(distances),
            }
        )
        for number, sample in enumerate(samples, 1):
            records.append({"scene": label, "sample": number, **sample})

    if records:
        output_path = save_measurements(records)
        print(f"\n原始测距记录已保存：{output_path}")

    empty_results = [item for item in summaries if item["is_empty"]]
    if empty_results:
        empty_minimum = min(item["minimum"] for item in empty_results)
        suggested = max(1, empty_minimum - 40)
        if suggested <= 800:
            args.distance = suggested
            print(
                f"空场景最低可靠读数 E_min={empty_minimum} cm；"
                f"按 40 cm 余量，Dist 输入默认值暂定为 {suggested} cm。"
            )
        else:
            print("测量值超出当前厘米模式的预期范围，请先核对输出格式。")

        moving = next(
            (item for item in summaries if item["scene"] == "门正在摆动、无人"),
            None,
        )
        if moving and moving["minimum"] < args.distance:
            print("警告：门摆动已经低于拟定 Dist，会产生无人误触发；应调整激光方向。")

    entry = next(
        (item for item in summaries if item["scene"] == "人刚进入门口"),
        None,
    )
    if entry and entry["median"] >= args.distance:
        print("警告：人在门口的中位距离没有小于拟定 Dist，不能保证刚进门就触发。")
    return summaries


def run_wizard(args: argparse.Namespace, serial_module, list_ports_module) -> bool:
    print("TF-Luna guided configuration / 交互式配置")
    print(f"Python {sys.version.split()[0]}: OK")
    print(f"pySerial {getattr(serial_module, '__version__', 'available')}: OK")
    print("继续前请关闭北醒 GUI、Arduino Serial Monitor 和其他串口程序。")
    if not args.port:
        args.port = choose_detected_port(
            serial_module, list_ports_module, args.baudrate
        )
    if not args.port:
        print("Cancelled: no serial port selected.", file=sys.stderr)
        return False

    measure_answer = input(
        "\n是否先用 Terminal 做现场参考测距？[Y/n]: "
    ).strip().lower()
    if measure_answer in ("", "y", "yes"):
        run_measurement_wizard(serial_module, args)
        continue_answer = input(
            "\n是否继续进入参数设置和写入步骤？[Y/n]: "
        ).strip().lower()
        if continue_answer in ("n", "no"):
            args.stop_after_measurement = True
            return True

    print("\n请依次输入五项参数；直接按 Enter 使用方括号中的默认值。")
    print("Mode 1：物体近于 Dist 时 Pin 6 输出 HIGH（本项目推荐）。")
    print("Mode 2：物体近于 Dist 时 Pin 6 输出 LOW。")
    args.mode = prompt_int("1/5 Mode", args.mode, 1, 2)
    args.distance = prompt_int("2/5 Dist / trigger distance (cm)", args.distance, 1, 800)
    args.zone = prompt_int("3/5 Zone / hysteresis width (cm)", args.zone, 0, 800)
    args.delay_in = prompt_int("4/5 Delay1 / approach delay (ms)", args.delay_in, 0, 65535)
    args.delay_out = prompt_int("5/5 Delay2 / leave delay (ms)", args.delay_out, 0, 65535)

    # Keep weak-signal dummy output safely beyond the release threshold.
    args.dummy_distance = max(
        args.dummy_distance, args.distance + args.zone + 50
    )
    print(
        f"\n自动弱信号保护参数：Amp={args.amp_threshold}, "
        f"Dummy Dist={args.dummy_distance} cm"
    )
    return True


def describe_on_off_reply(frame: bytes):
    if len(frame) < 14 or frame[2] != 0x3F or frame[3] != 0x3B:
        return None
    payload = frame[4:-1]
    if len(payload) < 9:
        return None
    return {
        "mode": payload[0],
        "distance": int.from_bytes(payload[1:3], "little"),
        "zone": int.from_bytes(payload[3:5], "little"),
        "delay_in": int.from_bytes(payload[5:7], "little"),
        "delay_out": int.from_bytes(payload[7:9], "little"),
    }


def send_sequence(serial_module, args: argparse.Namespace, commands) -> bool:
    print(f"\nOpening {args.port} at {args.baudrate} baud...")
    try:
        with serial_module.Serial(
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

            for label, frame in commands:
                device.reset_input_buffer()
                device.write(frame)
                device.flush()
                response = read_for(device)
                parsed = list(valid_frames(response))

                print(f"\nSent {label}: {hex_text(frame)}")
                if parsed:
                    for reply in parsed:
                        print(f"Reply: {hex_text(reply)}")
                        decoded = describe_on_off_reply(reply)
                        if decoded:
                            print(
                                "Decoded on/off settings: "
                                f"Mode={decoded['mode']}, "
                                f"Dist={decoded['distance']} cm, "
                                f"Zone={decoded['zone']} cm, "
                                f"Delay1={decoded['delay_in']} ms, "
                                f"Delay2={decoded['delay_out']} ms"
                            )
                elif response:
                    print(f"Raw reply (no valid 0x5A frame found): {hex_text(response)}")
                else:
                    print("No reply received.")
                time.sleep(0.15)
    except (serial_module.SerialException, OSError) as error:
        print(f"Serial error: {error}", file=sys.stderr)
        return False
    return True


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
    action.add_argument(
        "--wizard",
        action="store_true",
        help="Auto-detect TF-Luna, ask for five values, preview, and write.",
    )
    return parser.parse_args()


def main() -> int:
    if sys.version_info < MINIMUM_PYTHON:
        required = ".".join(str(value) for value in MINIMUM_PYTHON)
        current = ".".join(str(value) for value in sys.version_info[:3])
        print(
            f"Python {required} or newer is required; found {current}.",
            file=sys.stderr,
        )
        return 2

    args = parse_args()

    serial_module = None
    list_ports_module = None
    if args.apply or args.verify_only or args.wizard:
        serial_module, list_ports_module = load_pyserial(offer_install=args.wizard)
        if serial_module is None:
            return 2

    if args.wizard and not run_wizard(
        args, serial_module, list_ports_module
    ):
        return 2
    if getattr(args, "stop_after_measurement", False):
        print("测距向导完成，没有修改 TF-Luna 设置。")
        return 0

    try:
        commands = build_commands(args)
    except ValueError as error:
        print(f"Configuration error: {error}", file=sys.stderr)
        return 2

    selected_commands = [commands[0], commands[-1]] if args.verify_only else commands

    heading = "Read-only TF-Luna frames:" if args.verify_only else "Proposed TF-Luna frames:"
    print(heading)
    for label, frame in selected_commands:
        print(f"  {label}: {hex_text(frame)}")

    if not args.apply and not args.verify_only and not args.wizard:
        print("\nDry-run only. Nothing was written.")
        print("Add --port PORT --apply after checking real room measurements.")
        return 0

    if (args.apply or args.verify_only) and not args.port:
        args.port = choose_detected_port(
            serial_module, list_ports_module, args.baudrate
        )
        if not args.port:
            return 2

    if args.wizard:
        print("\n释放阈值 Dist + Zone = "
              f"{args.distance + args.zone} cm")
        confirmation = input(
            f"确认无误后输入大写 WRITE，向 {args.port} 写入："
        ).strip()
        if confirmation != "WRITE":
            print("已取消，没有写入任何设置。")
            return 0

    if not send_sequence(serial_module, args, selected_commands):
        return 1

    if args.verify_only:
        print("\nRead-only verification completed. No settings were changed.")
    else:
        print("\nConfiguration sequence completed.")
        if args.wizard:
            print("\n现在请给 TF-Luna 断电重启。")
            print("可以拔下并重新插入 USB，然后等待串口重新出现。")
            answer = input(
                "准备好后按 Enter 开始只读验证，或输入 SKIP 跳过："
            ).strip().upper()
            if answer != "SKIP":
                matched, reason = probe_tf_luna(
                    serial_module, args.port, args.baudrate
                )
                if not matched:
                    print(f"原串口未确认（{reason}），现在重新扫描。")
                    args.port = choose_detected_port(
                        serial_module, list_ports_module, args.baudrate
                    )
                if not args.port:
                    print("Verification cancelled: no serial port selected.")
                    return 1
                verify_commands = [commands[0], commands[-1]]
                if not send_sequence(serial_module, args, verify_commands):
                    return 1
                print("\nRead-only verification completed. No settings were changed.")
            else:
                print("Verification skipped. Run later with --verify-only.")
        else:
            print("Power-cycle the sensor, then run again with --verify-only.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
