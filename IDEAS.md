# IDEAS.md

Ideas and features for slit. Each has an adopted status:

- to be considered — not yet decided
- adopted — committed to implementing
- deferred — explicitly punted
- rejected — decided against (with reason)

Plan reference: `.plan/2026-04-06-anvil-creek/`

---

## Core behavior

### Fixed-height scrolling pane
Read stdin, keep a rolling buffer of the last N lines, re-render in-place within a fixed-height region of the terminal.
- adopted: adopted (v1)

### Auto-trim to terminal width
Detect terminal width (and react to SIGWINCH), trim lines to fit. No wrapping.
- adopted: adopted (v1)

### `-n` line count flag
`slit -n 20` allocates 20 lines. Default to terminal height minus 1 (for status line).
- adopted: adopted (v1)

### Spinner status line
Show `⠋ Streaming...` (braille spinner + label) on the line below the content pane. Clears on EOF.
- adopted: adopted (v1) — combined with keybinding hints on RHS

### Done summary on EOF
When stdin closes, replace spinner with something like `Done. (142 lines)`.
- adopted: adopted (v1)

---

## Output / tee

### Tee to file (`-o output.log`)
While rendering to the pane, also write all input to a file. Like `tee` but with the visual overlay.
- adopted: adopted (v1) — raw by default, configurable format

### Tee to file with append (`-a`)
Append mode for the tee file.
- adopted: adopted (v1)

---

## Progress bar

### Progress bar (on/off/auto)
Show a progress bar in the status line. `auto` mode would try to detect progress if stdin is a known size or if the producing process emits progress hints.
- adopted: adopted (v1) — auto-detect file size, show fill bar; fallback to spinner+count for pipes

### Progress bar via sidechannel
Accept progress info via a named pipe, signal, or environment variable so the producing process can communicate progress without mixing it into stdout.
- adopted: deferred (v2)

---

## Display options

### Line numbers
Show line numbers in a gutter. Toggle with `--line-numbers` or `-l`.
- adopted: adopted (v1) — dim colored number, no separator pipe

### ANSI color passthrough
Detect and pass through ANSI escape sequences so colored output renders correctly within the pane.
- adopted: adopted (v1) — `--color=auto|always|never` (like bat)

### Truncation indicator
Append `…` or `»` when a line is trimmed, so the user knows content was cut.
- adopted: adopted (v1) — default `…`, configurable

### Wrap mode
Option to wrap long lines instead of truncating. Would need to adjust the effective line count.
- adopted: adopted (v1) — `--wrap` flag, default is truncate

### Timestamp prefix
Option to prepend a timestamp to each line as it arrives.
- adopted: adopted (v1) — `--timestamp` or `-t`

### Highlight new lines
Briefly highlight newly arrived lines (e.g. dim flash) before they settle.
- adopted: deferred (v2)

---

## Layouts

### Configurable layouts
Multiple layout presets: box, rounded, compact, minimal (default), none, quote.
Selectable via `--layout` flag, shortcut flags (`--box`, `--compact`, etc.), or config.toml.
- adopted: adopted (v1)

### Quote layout
Left vertical bar on every line, optional background tint. Status integrated into bar area.
- adopted: adopted (v1)

---

## Interaction

### Interactive scrollback
Once streaming is done (EOF), allow the user to scroll up/down through the full buffer, not just the last N lines. Like a lightweight `less`.
- adopted: deferred (v2)

### Search/filter
After EOF, allow `/` to search within the buffer. Highlight matches.
- adopted: deferred (v2) — depends on scrollback

### Pause/resume
Press a key (e.g. Space) to pause rendering and inspect current output. Press again to resume, catching up to latest.
- adopted: deferred (v2)

---

## Performance

### Render debouncing
If input arrives very fast (e.g. `cat large-file`), debounce renders to avoid excessive terminal writes. Render at most every ~16ms (60fps) or ~33ms (30fps).
- adopted: adopted (v1) — ~30fps

### SIGWINCH handling
React to terminal resize events smoothly without breaking the display.
- adopted: adopted (v1)

---

## Infrastructure

### Exit code passthrough
Exit with the same code as the piped process (if detectable), or 0 on clean EOF.
- adopted: deferred (v2)

### Window title
Set terminal window title to show `slit — <command>` or similar.
- adopted: adopted (v1)

### Config file
Support `~/.config/slit/config.toml` or similar for default settings (default line count, colors, spinner style, etc).
- adopted: adopted (v1)

### Custom spinner style
Allow choosing spinner style (braille, dots, arrows, etc) or disabling it.
- adopted: adopted (v1) — `--spinner=braille|dots|arrows|off`

### Shell completions
Generate shell completion scripts for bash, zsh, fish via `slit completion <shell>`.
- adopted: adopted (v1)

### Debug logging
Log diagnostic info to a file (render timings, buffer stats, signals). Enabled with `--debug`.
- adopted: adopted (v1) — default log file: `/tmp/slit-$PID.log`

### Man page / --help
Generate a man page and have a thorough `--help` output.
- adopted: adopted (v1)

### Keybinding bar
Show active keybindings on RHS of status line (e.g. `q:quit`).
- adopted: adopted (v1)

---

## V3 (maybe never)

### Named pipe / multi-input
Accept multiple inputs and show them in split panes or interleaved with labels.
- adopted: v3 (maybe never)
