#!/usr/bin/env python3
"""Fuzzing mode for cross-implementation comparison."""

import random
import string
import sys
import os
import json
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
from compare import run_all, compare_results, format_diff, BINARIES


def random_ansi_code():
    codes = [
        f"\x1b[{random.randint(30, 37)}m",
        f"\x1b[{random.randint(40, 47)}m",
        f"\x1b[{random.randint(1, 9)}m",
        f"\x1b[38;5;{random.randint(0, 255)}m",
        f"\x1b[48;5;{random.randint(0, 255)}m",
        f"\x1b[38;2;{random.randint(0, 255)};{random.randint(0, 255)};{random.randint(0, 255)}m",
        "\x1b[0m",
        "",
    ]
    return random.choice(codes)


def random_unicode_segment():
    segments = [
        "".join(
            random.choice("Hello世界テスト한국") for _ in range(random.randint(1, 20))
        ),
        "".join(random.choice("🎉🚀💻✨🔥⚡🌟") for _ in range(random.randint(1, 5))),
        "".join(random.choice("Приветмир") for _ in range(random.randint(1, 15))),
        "".join(
            random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
            for _ in range(random.randint(1, 50))
        ),
        random.choice(string.printable[:62]) * random.randint(1, 30),
    ]
    return random.choice(segments)


def random_control_char():
    chars = [
        b"\x00",
        b"\x01",
        b"\x07",
        b"\x08",
        b"\x0b",
        b"\x0c",
        b"\x0e",
        b"\x0f",
        b"\x1b",
        b"\x7f",
        b"\x80",
        b"\x90",
        b"\xa0",
    ]
    return random.choice(chars)


def generate_fuzz_line():
    parts = []
    segments = random.randint(1, 8)
    for _ in range(segments):
        if random.random() < 0.3:
            parts.append(random_ansi_code())
        elif random.random() < 0.1:
            parts.append(random_control_char().decode("utf-8", errors="replace"))
        else:
            parts.append(random_unicode_segment())
    if random.random() < 0.3:
        parts.append("\x1b[0m")
    line = "".join(parts)
    if random.random() < 0.05:
        line = ""
    if random.random() < 0.02:
        line = "X" * random.randint(500, 5000)
    return line


def generate_fuzz_input():
    n_lines = random.randint(1, 50)
    lines = [generate_fuzz_line() for _ in range(n_lines)]
    input_str = "\n".join(lines) + "\n"
    return input_str.encode("utf-8", errors="replace")


def random_args():
    all_args = [
        [],
        ["--wrap"],
        ["--line-numbers"],
        ["--layout=box"],
        ["--layout=rounded"],
        ["--layout=compact"],
        ["--layout=minimal"],
        ["--layout=none"],
        ["--layout=quote"],
        ["--color=always"],
        ["--color=never"],
        ["--wrap", "--line-numbers"],
        ["--layout=box", "--line-numbers"],
        ["--wrap", "--color=always"],
        ["--truncation-char=~"],
        ["--wrap", "--layout=compact", "--line-numbers"],
    ]
    return random.choice(all_args)


def run_fuzz(iterations=100, output_dir=None):
    print(f"=== FUZZING: {iterations} iterations ===")
    print()

    total = 0
    passed = 0
    differences_found = 0
    all_diffs = []

    for i in range(iterations):
        total += 1
        input_data = generate_fuzz_input()
        args = random_args()
        test_name = f"fuzz_{i:04d}"

        try:
            results = run_all(input_data, args=args, timeout=15.0)
            diffs = compare_results(results, test_name)

            if diffs:
                differences_found += 1
                all_diffs.extend(diffs)
                print(
                    f"  [{i + 1}/{iterations}] DIFF FOUND - {test_name} (args: {' '.join(args)})"
                )
                for d in diffs:
                    print(f"    {d.impl_a} vs {d.impl_b}: {d.description}")
            else:
                passed += 1
                if (i + 1) % 20 == 0:
                    print(f"  [{i + 1}/{iterations}] ... {passed} passed so far")

        except Exception as e:
            print(f"  [{i + 1}/{iterations}] ERROR: {e}")

    print()
    print(f"=== FUZZ RESULTS ===")
    print(f"  Total iterations: {total}")
    print(f"  Passed (all same): {passed}")
    print(f"  Differences found: {differences_found}")

    if output_dir and all_diffs:
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = Path(output_dir) / f"fuzz_report_{ts}.txt"
        with open(report_path, "w") as f:
            f.write(f"Fuzz Report: {datetime.now().isoformat()}\n")
            f.write(
                f"Iterations: {total}, Passed: {passed}, Differences: {differences_found}\n"
            )
            f.write("=" * 60 + "\n\n")
            for d in all_diffs:
                f.write(format_diff(d) + "\n")
                f.write("-" * 40 + "\n")
        print(f"  Report saved to: {report_path}")

    return differences_found == 0


if __name__ == "__main__":
    iters = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    results_dir = Path(__file__).parent / "results"
    results_dir.mkdir(exist_ok=True)
    ok = run_fuzz(iterations=iters, output_dir=results_dir)
    sys.exit(0 if ok else 1)
