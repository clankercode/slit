#!/usr/bin/env python3
"""Cross-implementation output comparison engine for slit.

Uses the PTY runner to capture rendered TUI output from all 3 implementations
and compare them. Reports differences without fixing them.
"""

import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

sys.path.insert(0, os.path.dirname(__file__))
from pty_runner import (
    run_with_pty,
    BINARIES,
    DEFAULT_ROWS,
    DEFAULT_COLS,
    DEFAULT_TIMEOUT,
)
from vt_renderer import render_pty_output

SPINNER_CHARS = set(
    "⠁⠂⠃⠄⠅⠆⠇⡀⡁⡂⡃⡄⡅⡆⡇⠈⠉⠊⠋⠌⠍⠎⠏⡈⡉⡊⡋⡌⡍⡎⡏⠐⠑⠒⠓⠔⠕⠖⠗⡐⡑⡒⡓⡔⡕⡖⡗⠘⠙⠚⠛⠜⠝⠞⠟⡘⡙⡚⡛⡜⡝⡞⡟⠠⠡⠢⠣⠤⠥⠦⠧⡠⡡⡢⡣⡤⡥⡦⡧⠨⠩⠪⠫⠬⠭⠮⠯⡨⡩⡪⡫⡬⡭⡮⡯⠰⠱⠲⠳⠴⠵⠶⠷⡰⡱⡲⡳⡴⡵⡶⡷⠸⠹⠺⠻⠼⠽⠾⠿⡸⡹⡺⡻⡼⡽⡾⡿⢀⢁⢂⢃⢄⢅⢆⢇⣀⣁⣂⣃⣄⣅⣆⣇⢈⢉⢊⢋⢌⢍⢎⢏⣈⣉⣊⣋⣌⣍⣎⣏⢐⢑⢒⢓⢔⢕⢖⢗⣐⣑⣒⣓⣔⣕⣖⣗⢘⢙⢚⢛⢜⢝⢞⢟⣘⣙⣚⣛⣜⣝⣞⣟⢠⢡⢢⢣⢤⢥⢦⢧⣠⣡⣢⣣⣤⣥⣦⣧⢨⢩⢪⢫⢬⢭⢮⢯⣨⣩⣪⣫⣬⣭⣮⣯⢰⢱⢲⢳⢴⢵⢶⢷⣰⣱⣲⣳⣴⣵⣶⣷⢸⢹⢺⢻⢼⢽⢾⢿⣸⣹⣺⣻⣼⣽⣾⣿"
)

ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*[a-zA-Z]")
ANSI_CURSOR_RE = re.compile(r"\x1b\[[0-9;]*[ABCDJKHf]")
ANSI_CLEAR_RE = re.compile(r"\x1b\[[0-9]*J")
ANSI_SGR_RE = re.compile(r"\x1b\[[\d;]*m")
ANSI_PRIVATE_RE = re.compile(r"\x1b\[\?[0-9]+[hl]")
OSC_RE = re.compile(r"\x1b\][^\x07]*\x07")
TIMESTAMP_RE = re.compile(r"\d{2}:\d{2}:\d{2}")
LINE_COUNT_RE = re.compile(r"\b\d+\s*lines?\b")
BYTE_COUNT_RE = re.compile(r"\b\d+\s*[BKMG]?[Bb]\b")
DURATION_RE = re.compile(r"\d+\.\d+[a-zA-Z]*[bB]?")


@dataclass
class ImplResult:
    impl: str
    binary: Path
    exit_code: int
    output: str
    timed_out: bool = False


@dataclass
class Difference:
    test_name: str
    impl_a: str
    impl_b: str
    description: str
    output_a: str
    output_b: str


@dataclass
class TestResult:
    name: str
    passed: bool
    differences: list = field(default_factory=list)
    results: dict = field(default_factory=dict)
    error: Optional[str] = None


def normalize_output(raw: str, rows=DEFAULT_ROWS, cols=DEFAULT_COLS) -> str:
    """Normalize PTY-rendered output for comparison.

    Renders through a virtual terminal first to properly handle cursor
    positioning, then normalizes timing-dependent content.
    """
    rendered = render_pty_output(raw, rows=rows, cols=cols)
    out = rendered
    for ch in SPINNER_CHARS:
        out = out.replace(ch, "")
    out = TIMESTAMP_RE.sub("HH:MM:SS", out)
    out = LINE_COUNT_RE.sub("N lines", out)
    out = BYTE_COUNT_RE.sub("NB", out)
    out = DURATION_RE.sub("DURATION", out)
    lines = out.splitlines()
    lines = [l.rstrip() for l in lines]
    lines = [l for l in lines if l.strip()]
    return "\n".join(lines)


def normalize_for_content(raw: str, rows=DEFAULT_ROWS, cols=DEFAULT_COLS) -> str:
    """Strip all formatting for pure content comparison.

    Renders through virtual terminal, then removes spinner chars and
    chrome elements to extract just the data text.
    """
    rendered = render_pty_output(raw, rows=rows, cols=cols)
    for ch in SPINNER_CHARS:
        rendered = rendered.replace(ch, "")
    rendered = TIMESTAMP_RE.sub("", rendered)
    rendered = LINE_COUNT_RE.sub("", rendered)
    rendered = BYTE_COUNT_RE.sub("", rendered)
    lines = rendered.splitlines()
    lines = [l.rstrip() for l in lines]
    lines = [l for l in lines if l.strip()]
    return "\n".join(lines)


def extract_content_lines(raw: str, rows=DEFAULT_ROWS, cols=DEFAULT_COLS) -> list:
    """Extract just the data content lines from PTY output.

    Renders through virtual terminal, then filters out chrome/UI elements.
    """
    norm = normalize_for_content(raw, rows=rows, cols=cols)
    lines = norm.split("\n")
    content = []
    skip_patterns = [
        r"Streaming",
        r"Done\.",
        r"q:quit",
        r"^[│┌┐└┘─┬┴├┤┼┤]+$",
        r"^$",
    ]
    for line in lines:
        if not line.strip():
            continue
        skip = False
        for pat in skip_patterns:
            if re.search(pat, line):
                skip = True
                break
        if not skip:
            content.append(line.strip())
    return content


def run_impl(
    binary: Path,
    input_data: bytes,
    args: list = None,
    timeout: float = DEFAULT_TIMEOUT,
    rows: int = DEFAULT_ROWS,
    cols: int = DEFAULT_COLS,
) -> ImplResult:
    """Run a single slit implementation with the given input via PTY."""
    args = args or []
    try:
        exit_code, output = run_with_pty(
            binary,
            args,
            input_data,
            timeout=timeout,
            rows=rows,
            cols=cols,
        )
        return ImplResult(
            impl=binary.parent.name,
            binary=binary,
            exit_code=exit_code,
            output=output,
        )
    except Exception as e:
        return ImplResult(
            impl=binary.parent.name,
            binary=binary,
            exit_code=-1,
            output=f"ERROR: {e}",
        )


def run_all(
    input_data: bytes,
    args: list = None,
    timeout: float = DEFAULT_TIMEOUT,
    rows: int = DEFAULT_ROWS,
    cols: int = DEFAULT_COLS,
) -> dict:
    """Run all implementations with the same input via PTY."""
    results = {}
    for name, binary in BINARIES.items():
        if not binary.exists():
            results[name] = ImplResult(
                impl=name,
                binary=binary,
                exit_code=-1,
                output=f"BINARY NOT FOUND: {binary}",
            )
            continue
        results[name] = run_impl(binary, input_data, args, timeout, rows, cols)
    return results


def compare_results(
    results: dict,
    test_name: str,
    mode: str = "normalized",
) -> list:
    """Compare results across implementations. Returns list of Difference."""
    diffs = []
    impls = list(results.keys())

    for i in range(len(impls)):
        for j in range(i + 1, len(impls)):
            a_name, b_name = impls[i], impls[j]
            a, b = results[a_name], results[b_name]

            if a.timed_out and b.timed_out:
                continue
            if a.timed_out and not b.timed_out:
                diffs.append(
                    Difference(
                        test_name=test_name,
                        impl_a=a_name,
                        impl_b=b_name,
                        description=f"{a_name} timed out, {b_name} did not",
                        output_a="<TIMEOUT>",
                        output_b=b.output[:2000],
                    )
                )
                continue
            if not a.timed_out and b.timed_out:
                diffs.append(
                    Difference(
                        test_name=test_name,
                        impl_a=a_name,
                        impl_b=b_name,
                        description=f"{b_name} timed out, {a_name} did not",
                        output_a=a.output[:2000],
                        output_b="<TIMEOUT>",
                    )
                )
                continue

            if a.exit_code != b.exit_code:
                diffs.append(
                    Difference(
                        test_name=test_name,
                        impl_a=a_name,
                        impl_b=b_name,
                        description=f"Exit code differs: {a_name}={a.exit_code}, {b_name}={b.exit_code}",
                        output_a=a.output[:500],
                        output_b=b.output[:500],
                    )
                )

            if mode == "normalized":
                a_norm = normalize_output(a.output)
                b_norm = normalize_output(b.output)
            elif mode == "content":
                a_norm = normalize_for_content(a.output)
                b_norm = normalize_for_content(b.output)
            elif mode == "content_lines":
                a_norm = "\n".join(extract_content_lines(a.output))
                b_norm = "\n".join(extract_content_lines(b.output))
            else:
                a_norm = a.output
                b_norm = b.output

            if a_norm != b_norm:
                diffs.append(
                    Difference(
                        test_name=test_name,
                        impl_a=a_name,
                        impl_b=b_name,
                        description=f"Output differs ({mode} mode)",
                        output_a=a_norm[:3000],
                        output_b=b_norm[:3000],
                    )
                )

    return diffs


def format_diff(diff: Difference) -> str:
    """Format a single difference for display."""
    lines = [
        f"  DIFFERENCE: {diff.impl_a} vs {diff.impl_b}",
        f"  Description: {diff.description}",
    ]
    if diff.output_a != diff.output_b:
        lines.append(f"  --- {diff.impl_a} output ---")
        for line in diff.output_a.split("\n")[:20]:
            lines.append(f"  | {line}")
        if diff.output_a.count("\n") > 20:
            lines.append(f"  | ... ({diff.output_a.count(chr(10)) - 20} more lines)")
        lines.append(f"  --- {diff.impl_b} output ---")
        for line in diff.output_b.split("\n")[:20]:
            lines.append(f"  | {line}")
        if diff.output_b.count("\n") > 20:
            lines.append(f"  | ... ({diff.output_b.count(chr(10)) - 20} more lines)")
    return "\n".join(lines)
