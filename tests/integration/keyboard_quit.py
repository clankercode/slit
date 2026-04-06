#!/usr/bin/env python3
"""Test that slit responds to keyboard quit (q or ctrl+c) while processing piped data.

Uses a PTY so that slit runs in render mode with /dev/tty available for key input,
matching the real user experience of piping data into slit and pressing q to quit.
"""

import fcntl
import os
import pty
import select
import signal
import struct
import sys
import termios
import time


def run_keyboard_quit_test(binary, use_ctrl_c=False, timeout=8):
    master_fd, slave_fd = pty.openpty()
    winsize = struct.pack("HHHH", 24, 80, 0, 0)
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
            os.execle(str(binary), "slit", env)
        except OSError:
            os._exit(127)

    os.close(slave_fd)
    os.close(data_r)

    all_output = b""
    start_time = time.time()

    try:
        while time.time() - start_time < timeout:
            r, _, _ = select.select([master_fd], [], [], 0.05)
            if r:
                try:
                    data = os.read(master_fd, 4096)
                    all_output += data
                except OSError:
                    break

            if b"\x1b[6n" in all_output:
                all_output = all_output.replace(b"\x1b[6n", b"")
                os.write(master_fd, b"\x1b[1;1R")

            elapsed = time.time() - start_time

            if elapsed > 0.2 and elapsed < 0.4:
                os.write(data_w, b"line1\nline2\nline3\n")

            if elapsed > 0.6:
                os.close(data_w)
                break

            try:
                pid_res, status = os.waitpid(pid, os.WNOHANG)
                if pid_res != 0:
                    break
            except ChildProcessError:
                break

        time.sleep(0.3)

        if use_ctrl_c:
            os.write(master_fd, b"\x03")
        else:
            os.write(master_fd, b"q")

        quit_time = time.time()
        exited = False
        while time.time() - quit_time < 3:
            r, _, _ = select.select([master_fd], [], [], 0.05)
            if r:
                try:
                    data = os.read(master_fd, 4096)
                    all_output += data
                except OSError:
                    break

            try:
                pid_res, status = os.waitpid(pid, os.WNOHANG)
                if pid_res != 0:
                    exited = True
                    break
            except ChildProcessError:
                exited = True
                break

        if not exited:
            os.kill(pid, signal.SIGKILL)
            _, status = os.waitpid(pid, 0)
            return (
                False,
                f"slit did not exit within 3s after sending {'ctrl+c' if use_ctrl_c else 'q'}",
            )

        exit_code = os.WEXITSTATUS(status) if os.WIFEXITED(status) else -1
        if exit_code != 0:
            return False, f"slit exited with code {exit_code}, expected 0"

        return (
            True,
            f"slit exited cleanly (code 0) after {'ctrl+c' if use_ctrl_c else 'q'}",
        )

    except Exception as e:
        return False, f"exception: {e}"
    finally:
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError:
            pass
        try:
            os.waitpid(pid, os.WNOHANG)
        except ChildProcessError:
            pass
        try:
            os.close(master_fd)
        except OSError:
            pass
        try:
            os.close(data_w)
        except OSError:
            pass


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: keyboard_quit.py <slit-binary> [--ctrl-c]", file=sys.stderr)
        sys.exit(1)

    binary = sys.argv[1]
    use_ctrl_c = "--ctrl-c" in sys.argv

    if not os.path.exists(binary):
        print(f"binary not found: {binary}", file=sys.stderr)
        sys.exit(1)

    ok, msg = run_keyboard_quit_test(binary, use_ctrl_c=use_ctrl_c)
    if not ok:
        print(f"FAIL: {msg}", file=sys.stderr)
        sys.exit(1)
    else:
        print(f"PASS: {msg}")
        sys.exit(0)
