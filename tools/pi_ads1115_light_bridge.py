#!/usr/bin/env python3
"""
Read an ADS1115 channel on Raspberry Pi and write a normalized value to a file.

This is the fastest path for the weekend proof-of-concept:
- Wire an LDR divider into ADS1115 A0
- Run this bridge on the Pi
- Point the JUCE host at /tmp/light.txt via AIMH_LIGHT_FILE

The JUCE side already polls that file and maps it to synth parameters.
"""

import argparse
import os
import signal
import sys
import time
from typing import Optional


try:
    from smbus2 import SMBus  # type: ignore
except ImportError:
    try:
        from smbus import SMBus  # type: ignore
    except ImportError:
        SMBus = None  # type: ignore[assignment]


ADS1115_CONVERSION_REG = 0x00
ADS1115_CONFIG_REG = 0x01

_RUNNING = True


def _handle_signal(signum, frame):
    del signum, frame
    global _RUNNING
    _RUNNING = False


def _swap16(value: int) -> int:
    return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF)


def _build_config(channel: int, data_rate_bits: int = 0x4) -> int:
    mux_bits = {0: 0x4, 1: 0x5, 2: 0x6, 3: 0x7}[channel]
    pga_bits = 0x1  # +/-4.096V full scale
    mode_single_shot = 0x1
    comparator_disable = 0x3

    return (
        (0x1 << 15)
        | (mux_bits << 12)
        | (pga_bits << 9)
        | (mode_single_shot << 8)
        | (data_rate_bits << 5)
        | comparator_disable
    )


def _read_ads1115_raw(bus: SMBus, address: int, channel: int) -> int:
    config = _build_config(channel)
    bus.write_word_data(address, ADS1115_CONFIG_REG, _swap16(config))

    # 128 SPS is ~7.8ms conversion time; 10ms is a safe, simple default.
    time.sleep(0.01)

    raw = _swap16(bus.read_word_data(address, ADS1115_CONVERSION_REG))
    if raw & 0x8000:
        raw -= 1 << 16
    return raw


def _normalize(raw: int, raw_min: int, raw_max: int, invert: bool) -> float:
    if raw_max <= raw_min:
        raise ValueError("raw_max must be greater than raw_min")

    clamped = max(raw_min, min(raw_max, raw))
    norm = (clamped - raw_min) / float(raw_max - raw_min)
    if invert:
        norm = 1.0 - norm
    return max(0.0, min(1.0, norm))


def _atomic_write(path: str, value: float) -> None:
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)

    tmp_path = path + ".tmp"
    with open(tmp_path, "w", encoding="ascii") as handle:
        handle.write(f"{value:.6f}\n")
    os.replace(tmp_path, path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Bridge ADS1115 light sensor data into a normalized file for the JUCE host."
    )
    parser.add_argument("--bus", type=int, default=1, help="I2C bus number, usually 1 on Raspberry Pi")
    parser.add_argument(
        "--address",
        type=lambda value: int(value, 0),
        default=0x48,
        help="ADS1115 I2C address, default 0x48",
    )
    parser.add_argument("--channel", type=int, choices=[0, 1, 2, 3], default=0, help="ADS1115 input channel")
    parser.add_argument(
        "--output",
        default="/tmp/light.txt",
        help="Output file path for the normalized sensor value",
    )
    parser.add_argument(
        "--min-raw",
        type=int,
        default=2000,
        help="Raw ADC value treated as 0.0 after clamping",
    )
    parser.add_argument(
        "--max-raw",
        type=int,
        default=22000,
        help="Raw ADC value treated as 1.0 after clamping",
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert the normalized output, useful if brighter light produces a lower raw reading",
    )
    parser.add_argument(
        "--interval-ms",
        type=int,
        default=50,
        help="Polling interval in milliseconds",
    )
    parser.add_argument(
        "--smoothing",
        type=float,
        default=0.2,
        help="EMA smoothing factor in [0,1], where 1 means no smoothing",
    )
    parser.add_argument(
        "--report-every",
        type=float,
        default=1.0,
        help="Print a status line every N seconds",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not 0.0 <= args.smoothing <= 1.0:
        print("--smoothing must be in [0,1]", file=sys.stderr)
        return 2
    if args.report_every < 0.0:
        print("--report-every must be >= 0", file=sys.stderr)
        return 2
    if SMBus is None:
        print(
            "Missing SMBus module. Install one of:\n"
            "  sudo apt install python3-smbus\n"
            "or\n"
            "  pip3 install smbus2",
            file=sys.stderr,
        )
        return 1

    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)

    last_report = 0.0
    filtered: Optional[float] = None

    print(
        "Starting ADS1115 light bridge:",
        f"bus={args.bus}",
        f"addr=0x{args.address:02x}",
        f"channel={args.channel}",
        f"output={args.output}",
        f"raw=[{args.min_raw},{args.max_raw}]",
        f"invert={args.invert}",
    )

    try:
        with SMBus(args.bus) as bus:
            while _RUNNING:
                raw = _read_ads1115_raw(bus, args.address, args.channel)
                norm = _normalize(raw, args.min_raw, args.max_raw, args.invert)

                if filtered is None:
                    filtered = norm
                else:
                    alpha = args.smoothing
                    filtered = (alpha * norm) + ((1.0 - alpha) * filtered)

                _atomic_write(args.output, filtered)

                now = time.monotonic()
                if args.report_every > 0.0 and now - last_report >= args.report_every:
                    print(
                        f"raw={raw:6d} norm={norm:0.3f} filtered={filtered:0.3f} -> {args.output}"
                    )
                    last_report = now

                time.sleep(max(0.0, args.interval_ms / 1000.0))

    except FileNotFoundError as exc:
        print(f"I2C device not found: {exc}", file=sys.stderr)
        print("Did you enable I2C and choose the correct bus?", file=sys.stderr)
        return 1
    except PermissionError:
        print(
            "Permission denied opening the I2C bus. Try running with sudo or add your user to the i2c group.",
            file=sys.stderr,
        )
        return 1
    except OSError as exc:
        print(f"I2C communication failed: {exc}", file=sys.stderr)
        return 1

    print("Stopped ADS1115 light bridge.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
