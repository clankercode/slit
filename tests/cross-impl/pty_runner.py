#!/usr/bin/env python3
"""PTY-based runner for slit implementations.

Uses a pipe+PTY hybrid approach:
- stdin = pipe (for data input, keeps it separate from keyboard events)
- stdout/stderr = PTY slave (for TUI rendering)
- /dev/tty = PTY slave (controlling terminal for crossterm events)

This works for all 3 implementations:
- Go: opens /dev/tty for keyboard, stdin for data -> pipe feeds data, PTY for display
- Rust: crossterm tty_fd() sees pipe stdin -> opens /dev/tty for events -> PTY for display
- C: opens /dev/tty for keyboard, stdin for data -> pipe feeds data, PTY for display
"""

import os
import pty
import select
import signal
import struct
import sys
import time
import fcntl
import termios
from pathlib import Path


SLIT_ROOT = Path(__file__).resolve().parent.parent.parent

BINARIES = {
    "go": SLIT_ROOT / "go" / "slit",
    "rust": SLIT_ROOT / "rust" / "target" / "release" / "slit",
    "c": SLIT_ROOT / "c" / "slit",
}

DEFAULT_ROWS = 24
DEFAULT_COLS = 80
DEFAULT_TIMEOUT = 10


def run_with_pty(
    binary,
    args,
    input_data,
    timeout=DEFAULT_TIMEOUT,
    rows=DEFAULT_ROWS,
    cols=DEFAULT_COLS,
):
    """Run a slit binary in a PTY and capture rendered output.

    Uses pipe for stdin data and PTY for display.
    Returns (exit_code, pty_output_str).
    """
    master_fd, slave_fd = pty.openpty()

    winsize = struct.pack("HHHH", rows, cols, 0, 0)
    fcntl.ioctl(slave_fd, termios.TIOCSWINSZ, winsize)
    slave_name = os.ttyname(slave_fd)

    data_r, data_w = os.pipe()

    env = os.environ.copy()
    env["SLIT_FORCE_RENDER"] = "1"
    env["TERM"] = "xterm-256color"

    pid = os.fork()
    if pid == 0:
        os.close(master_fd)
        os.close(data_w)

        os.setsid()
        ctty_fd = os.open(slave_name, os.O_RDWR)
        try:
            fcntl.ioctl(ctty_fd, termios.TIOCSCTTY, 0)
        except OSError:
            pass

        os.dup2(data_r, 0)
        os.dup2(ctty_fd, 1)
        os.dup2(ctty_fd, 2)

        if ctty_fd > 2:
            os.close(ctty_fd)
        os.close(slave_fd)
        if data_r > 2:
            os.close(data_r)

        try:
            os.execle(str(binary), "slit", *args, env)
        except OSError:
            os._exit(127)

    os.close(slave_fd)
    os.close(data_r)

    all_output = b""
    data_written = False
    pipe_closed = False
    quit_sent = False
    phase = "init"
    start_time = time.time()
    data_write_time = None
    pipe_close_time = None

    try:
        while time.time() - start_time < timeout:
            elapsed = time.time() - start_time

            r, _, _ = select.select([master_fd], [], [], 0.02)
            if r:
                try:
                    data = os.read(master_fd, 4096)
                    all_output += data
                except OSError:
                    break

            if b"\x1b[6n" in all_output:
                all_output_clean = all_output.replace(b"\x1b[6n", b"")
                os.write(master_fd, b"\x1b[1;1R")
                all_output = all_output_clean

            if phase == "init" and elapsed > 0.15:
                phase = "write_data"

            if phase == "write_data" and not data_written:
                if input_data:
                    os.write(data_w, input_data)
                data_written = True
                data_write_time = time.time()
                phase = "wait_process"

            if phase == "wait_process" and data_write_time:
                if time.time() - data_write_time > 0.5:
                    phase = "close_pipe"

            if phase == "close_pipe" and not pipe_closed:
                os.close(data_w)
                pipe_closed = True
                pipe_close_time = time.time()
                phase = "wait_render"

            if phase == "wait_render" and pipe_close_time:
                if time.time() - pipe_close_time > 0.5:
                    phase = "send_quit"

            if phase == "send_quit" and not quit_sent:
                os.write(master_fd, b"q")
                quit_sent = True
                phase = "wait_exit"

            try:
                pid_res, status = os.waitpid(pid, os.WNOHANG)
                if pid_res != 0:
                    break
            except ChildProcessError:
                break

    except Exception:
        pass
    finally:
        try:
            os.kill(pid, signal.SIGTERM)
            time.sleep(0.05)
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            _, status = os.waitpid(pid, 0)
        except ChildProcessError:
            status = 0
        if not pipe_closed:
            try:
                os.close(data_w)
            except OSError:
                pass

    try:
        flags = fcntl.fcntl(master_fd, fcntl.F_GETFL)
        fcntl.fcntl(master_fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        while True:
            r, _, _ = select.select([master_fd], [], [], 0.1)
            if r:
                try:
                    data = os.read(master_fd, 4096)
                    all_output += data
                except OSError:
                    break
            else:
                break
    except Exception:
        pass

    try:
        os.close(master_fd)
    except OSError:
        pass

    exit_code = os.WEXITSTATUS(status) if os.WIFEXITED(status) else -1
    text = all_output.decode("utf-8", errors="replace")
    return exit_code, text


def run_all_with_pty(
    input_data, args=None, timeout=DEFAULT_TIMEOUT, rows=DEFAULT_ROWS, cols=DEFAULT_COLS
):
    """Run all implementations with the same input via PTY.

    Returns dict of {impl_name: (exit_code, output)}.
    """
    args = args or []
    results = {}
    for name, binary in BINARIES.items():
        if not binary.exists():
            results[name] = (-1, f"BINARY NOT FOUND: {binary}")
            continue
        try:
            exit_code, output = run_with_pty(
                binary,
                args,
                input_data,
                timeout=timeout,
                rows=rows,
                cols=cols,
            )
            results[name] = (exit_code, output)
        except Exception as e:
            results[name] = (-1, f"ERROR: {e}")
    return results
