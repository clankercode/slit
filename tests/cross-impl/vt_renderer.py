#!/usr/bin/env python3
"""Virtual terminal renderer for interpreting PTY output.

Takes raw PTY output with ANSI escape sequences and renders it into
a 2D screen buffer, simulating a terminal. This allows proper comparison
of output from different implementations that use different cursor
positioning strategies.
"""

import re


class VirtualTerminal:
    def __init__(self, rows=24, cols=80):
        self.rows = rows
        self.cols = cols
        self.screen = [[" "] * cols for _ in range(rows)]
        self.cursor_row = 0
        self.cursor_col = 0

    def _clamp_cursor(self):
        self.cursor_row = max(0, min(self.cursor_row, self.rows - 1))
        self.cursor_col = max(0, min(self.cursor_col, self.cols - 1))

    def _write_char(self, ch):
        if self.cursor_row >= self.rows or self.cursor_col >= self.cols:
            return
        self.screen[self.cursor_row][self.cursor_col] = ch
        self.cursor_col += 1
        if self.cursor_col >= self.cols:
            self.cursor_col = self.cols - 1

    def _newline(self):
        self.cursor_col = 0
        self.cursor_row += 1
        if self.cursor_row >= self.rows:
            self._scroll_up()
            self.cursor_row = self.rows - 1

    def _scroll_up(self):
        self.screen.pop(0)
        self.screen.append([" "] * self.cols)

    def feed(self, data: str):
        i = 0
        while i < len(data):
            ch = data[i]

            if ch == "\x1b":
                seq, consumed = self._parse_escape(data, i)
                if consumed > 0:
                    self._handle_escape(seq)
                    i += consumed
                    continue

            if ch == "\n":
                self._newline()
            elif ch == "\r":
                self.cursor_col = 0
            elif ch == "\t":
                self.cursor_col = min((self.cursor_col // 8 + 1) * 8, self.cols - 1)
            elif ch == "\x08":
                self.cursor_col = max(0, self.cursor_col - 1)
            elif ch == "\x07":
                pass
            elif ch >= " ":
                self._write_char(ch)

            i += 1

    def _parse_escape(self, data, start):
        if start + 1 >= len(data):
            return None, 0
        if data[start + 1] == "[":
            j = start + 2
            while j < len(data) and data[j] in "0123456789;":
                j += 1
            if j < len(data) and data[j] in "ABCDEFGHJKSTfmsulqh":
                params = data[start + 2 : j]
                cmd = data[j]
                return ("CSI", params, cmd), j - start + 1
            if j < len(data) and data[j] == "?":
                j += 1
                while j < len(data) and data[j] in "0123456789":
                    j += 1
                if j < len(data) and data[j] in "hl":
                    params = data[start + 2 : j]
                    cmd = data[j]
                    return ("CSI_PRIVATE", params, cmd), j - start + 1
            return None, 0
        if data[start + 1] == "]":
            end = data.find("\x07", start + 2)
            if end == -1:
                end = data.find("\x1b\\", start + 2)
                if end == -1:
                    return None, 0
                return ("OSC", data[start + 2 : end]), end - start + 2
            return ("OSC", data[start + 2 : end]), end - start + 1
        return None, 0

    def _parse_params(self, params_str, default=0):
        if not params_str:
            return [default]
        parts = params_str.split(";")
        return [int(p) if p else default for p in parts]

    def _handle_escape(self, seq):
        kind = seq[0]
        if kind == "CSI":
            _, params_str, cmd = seq
            params = self._parse_params(params_str)

            if cmd == "H" or cmd == "f":
                row = (params[0] if len(params) > 0 else 1) - 1
                col = (params[1] if len(params) > 1 else 1) - 1
                self.cursor_row = max(0, min(row, self.rows - 1))
                self.cursor_col = max(0, min(col, self.cols - 1))
            elif cmd == "A":
                n = params[0] if params[0] else 1
                self.cursor_row = max(0, self.cursor_row - n)
            elif cmd == "B":
                n = params[0] if params[0] else 1
                self.cursor_row = min(self.rows - 1, self.cursor_row + n)
            elif cmd == "C":
                n = params[0] if params[0] else 1
                self.cursor_col = min(self.cols - 1, self.cursor_col + n)
            elif cmd == "D":
                n = params[0] if params[0] else 1
                self.cursor_col = max(0, self.cursor_col - n)
            elif cmd == "J":
                mode = params[0]
                if mode == 0:
                    for c in range(self.cursor_col, self.cols):
                        self.screen[self.cursor_row][c] = " "
                    for r in range(self.cursor_row + 1, self.rows):
                        self.screen[r] = [" "] * self.cols
                elif mode == 2:
                    self.screen = [[" "] * self.cols for _ in range(self.rows)]
            elif cmd == "K":
                mode = params[0]
                if mode == 0:
                    for c in range(self.cursor_col, self.cols):
                        self.screen[self.cursor_row][c] = " "
                elif mode == 1:
                    for c in range(0, self.cursor_col + 1):
                        self.screen[self.cursor_row][c] = " "
                elif mode == 2:
                    self.screen[self.cursor_row] = [" "] * self.cols
            elif cmd == "m":
                pass
            elif cmd == "l" or cmd == "h":
                pass
        elif kind == "CSI_PRIVATE":
            pass
        elif kind == "OSC":
            pass

    def get_lines(self):
        return ["".join(row) for row in self.screen]

    def get_rendered_text(self):
        lines = self.get_lines()
        while lines and not lines[-1].strip():
            lines.pop()
        return "\n".join(lines)


def render_pty_output(raw: str, rows=24, cols=80) -> str:
    """Render raw PTY output through a virtual terminal.

    Returns the final screen content as a string.
    """
    vt = VirtualTerminal(rows=rows, cols=cols)
    vt.feed(raw)
    return vt.get_rendered_text()
