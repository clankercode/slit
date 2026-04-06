#!/usr/bin/env python3
"""Test case definitions for cross-implementation comparison."""

import random
import string


def make_ansi_fg(code: int) -> str:
    return f"\x1b[{code}m"


def make_ansi_256(n: int) -> str:
    return f"\x1b[38;5;{n}m"


def make_ansi_rgb(r: int, g: int, b: int) -> str:
    return f"\x1b[38;2;{r};{g};{b}m"


RESET = "\x1b[0m"
BOLD = "\x1b[1m"
DIM = "\x1b[2m"
UNDERLINE = "\x1b[4m"
BLINK = "\x1b[5m"
REVERSE = "\x1b[7m"


TEST_CASES = []


def register(name, input_fn, args=None, mode="normalized"):
    TEST_CASES.append(
        {"name": name, "input_fn": input_fn, "args": args or [], "mode": mode}
    )


# --- Plain text ---


def plain_text_input():
    return b"hello world\nline two\nline three\n"


register("plain_text", plain_text_input)


# --- Long lines ---


def long_lines_input():
    lines = []
    for i in range(5):
        lines.append(("A" * 200 + "\n").encode())
    return b"".join(lines)


register("long_lines", long_lines_input)


# --- ANSI colored lines ---


def ansi_colors_input():
    lines = []
    lines.append(f"{make_ansi_fg(31)}red text{RESET}\n".encode())
    lines.append(f"{make_ansi_256(202)}256-color text{RESET}\n".encode())
    lines.append(f"{make_ansi_rgb(255, 128, 0)}24-bit color text{RESET}\n".encode())
    lines.append(
        f"{make_ansi_fg(32)}green{RESET} normal {make_ansi_fg(34)}blue{RESET}\n".encode()
    )
    return b"".join(lines)


register("ansi_colors", ansi_colors_input)


# --- Empty lines ---


def empty_lines_input():
    return b"\n\n\ncontent after empties\n\n\n"


register("empty_lines", empty_lines_input)


# --- Very long lines with ANSI ---


def very_long_ansi_input():
    line = make_ansi_fg(31) + "X" * 1000 + RESET
    return f"{line}\n".encode()


register("very_long_ansi", very_long_ansi_input)


# --- Rapid input ---


def rapid_input():
    lines = []
    for i in range(100):
        lines.append(f"rapid line {i}\n".encode())
    return b"".join(lines)


register("rapid_input", rapid_input)


# --- Unicode content ---


def unicode_input():
    lines = [
        "Hello 世界\n",
        "こんにちは\n",
        "한국어 테스트\n",
        "🎉🚀💻✨\n",
        "Mixed: café résumé naïve\n",
        "العربية\n",
        "Привет мир\n",
    ]
    return "".join(lines).encode("utf-8")


register("unicode_content", unicode_input)


# --- Binary-ish content ---


def binary_ish_input():
    parts = []
    for i in range(32):
        parts.append(bytes([i]))
    parts.append(b" text between control chars ")
    for i in range(128, 160):
        parts.append(bytes([i]))
    parts.append(b"\n")
    return b"".join(parts)


register("binary_ish_content", binary_ish_input)


# --- Wrap mode ---


def wrap_input():
    return ("X" * 300 + "\n").encode()


register("wrap_mode", wrap_input, args=["--wrap"])


# --- All 6 layouts ---

for layout in ["box", "rounded", "compact", "minimal", "none", "quote"]:

    def make_layout_input(layout_name=layout):
        return b"layout test line\n"

    register(f"layout_{layout}", make_layout_input, args=[f"--layout={layout}"])


# --- Line numbers ---

register("line_numbers", lambda: b"first\nsecond\nthird\n", args=["--line-numbers"])


# --- Timestamps ---

register(
    "timestamps", lambda: b"timestamped line\n", args=["--timestamp"], mode="content"
)


# --- Color modes ---

register(
    "color_always",
    lambda: f"{make_ansi_fg(31)}colored{RESET}\n".encode(),
    args=["--color=always"],
)

register(
    "color_never",
    lambda: f"{make_ansi_fg(31)}colored{RESET}\n".encode(),
    args=["--color=never"],
)


# --- Ring buffer overflow ---


def ring_buffer_overflow_input():
    lines = []
    for i in range(2000):
        lines.append(f"buffer line {i:04d}\n".encode())
    return b"".join(lines)


register("ring_buffer_overflow", ring_buffer_overflow_input)


# --- Mixed ANSI ---


def mixed_ansi_input():
    lines = []
    lines.append(f"{BOLD}{make_ansi_fg(31)}nested{RESET} after reset\n".encode())
    lines.append(f"{make_ansi_fg(32)}{UNDERLINE}double{RESET}\n".encode())
    lines.append(f"incomplete\x1b[31\n".encode())
    lines.append(f"{REVERSE}reverse{RESET}\n".encode())
    lines.append(f"\x1b[38;5;200m\x1b[48;5;17mfg+bg{RESET}\n".encode())
    lines.append(f"\x1b[1;3;4;31m{BOLD}multi-attr{RESET}\n".encode())
    return b"".join(lines)


register("mixed_ansi", mixed_ansi_input)


# --- CJK width characters ---


def cjk_width_input():
    lines = [
        "全角文字テスト\n",
        "ＡＢＣＤ全角英数\n",
        "┌──────────┐\n",
        "│ box drawing │\n",
        "└──────────┘\n",
    ]
    return "".join(lines).encode("utf-8")


register("cjk_width", cjk_width_input)


# --- Truncation ---

register(
    "truncation", lambda: ("Y" * 500 + "\n").encode(), args=["--truncation-char=~"]
)


# --- Combined flags ---

register(
    "combined_flags",
    lambda: f"{make_ansi_fg(32)}hello{RESET} world\nline 2\nline 3\n".encode(),
    args=["--line-numbers", "--wrap", "--layout=compact"],
)


def get_all_test_cases():
    return TEST_CASES
