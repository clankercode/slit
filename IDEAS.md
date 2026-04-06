# IDEAS.md

Ideas and features for slit. Each has a status:

- **done** — implemented and shipped
- **to be considered** — not yet decided
- **deferred** — explicitly punted
- **rejected** — decided against (with reason)

---

## Implementations

| Language | Framework | Status |
|----------|-----------|--------|
| Go | Bubbletea + cobra | done |
| Rust | ratatui + clap | done |
| C | raw ANSI + getopt | done |

---

## Core behavior

### Fixed-height scrolling pane
Read stdin, keep a rolling buffer of the last N lines, re-render in-place within a fixed-height region of the terminal.
- **done** (v1)

### Auto-trim to terminal width
Detect terminal width (and react to SIGWINCH), trim lines to fit. No wrapping.
- **done** (v1)

### `-n` line count flag
`slit -n 20` allocates 20 lines. Default to terminal height minus 1 (for status line).
- **done** (v1)

### Spinner status line
Show `⠋ Streaming...` (braille spinner + label) on the line below the content pane. Clears on EOF.
- **done** (v1) — combined with keybinding hints on RHS

### Done summary on EOF
When stdin closes, replace spinner with something like `Done. (142 lines)`.
- **done** (v1)

---

## Output / tee

### Tee to file (`-o output.log`)
While rendering to the pane, also write all input to a file. Like `tee` but with the visual overlay.
- **done** (v1) — raw by default, configurable format via `--tee-format`

### Tee to file with append (`-a`)
Append mode for the tee file.
- **done** (v1)

---

## Progress bar

### Progress bar (on/off/auto)
Show a progress bar in the status line. `auto` mode tries to detect progress from file size.
- **done** (v1) — auto-detect file size, show fill bar; fallback to spinner+count for pipes

### Progress bar via sidechannel
Accept progress info via a named pipe, signal, or environment variable so the producing process can communicate progress without mixing it into stdout.
- **deferred** (v2)

---

## Display options

### Line numbers
Show line numbers in a gutter. Toggle with `--line-numbers` or `-l`.
- **done** (v1) — dim colored number, no separator pipe

### ANSI color passthrough
Detect and pass through ANSI escape sequences so colored output renders correctly within the pane.
- **done** (v1) — `--color=auto|always|never` (like bat)

### Truncation indicator
Append `…` or `»` when a line is trimmed, so the user knows content was cut.
- **done** (v1) — default `…`, configurable via `--truncation-char`

### Wrap mode
Option to wrap long lines instead of truncating. Would need to adjust the effective line count.
- **done** (v1) — `--wrap` / `-w` flag, default is truncate

### Timestamp prefix
Option to prepend a timestamp to each line as it arrives.
- **done** (v1) — `--timestamp` / `-t`

### Highlight new lines
Briefly highlight newly arrived lines (e.g. dim flash) before they settle.
- **deferred** (v2)

---

## Layouts

### Configurable layouts
Multiple layout presets: box, rounded, compact, minimal (default), none, quote.
Selectable via `--layout` flag, shortcut flags (`--box`, `--compact`, etc.), or config.toml.
- **done** (v1)

### Quote layout
Left vertical bar on every line, optional background tint. Status integrated into bar area.
- **done** (v1)

---

## Interaction

### Interactive scrollback
Once streaming is done (EOF), allow the user to scroll up/down through the full buffer, not just the last N lines. Like a lightweight `less`.
- **deferred** (v2)

### Search/filter
After EOF, allow `/` to search within the buffer. Highlight matches.
- **deferred** (v2) — depends on scrollback

### Pause/resume
Press a key (e.g. Space) to pause rendering and inspect current output. Press again to resume, catching up to latest.
- **deferred** (v2)

---

## Performance

### Render debouncing
If input arrives very fast (e.g. `cat large-file`), debounce renders to avoid excessive terminal writes. Render at most every ~33ms (~30fps).
- **done** (v1)

### SIGWINCH handling
React to terminal resize events smoothly without breaking the display.
- **done** (v1)

---

## Infrastructure

### Exit code passthrough
Exit with the same code as the piped process (if detectable), or 0 on clean EOF.
- **deferred** (v2)

### Window title
Set terminal window title to show `slit` via OSC escape sequences.
- **done** (v1)

### Config file
Support `~/.config/slit/config.toml` (or `$XDG_CONFIG_HOME/slit/config.toml`) for default settings.
- **done** (v1)

### Custom spinner style
Allow choosing spinner style (braille, dots, arrows, etc) or disabling it.
- **done** (v1) — `--spinner=braille|dots|arrows|off`

### Shell completions
Generate shell completion scripts for bash, zsh, fish via `slit completion <shell>`.
- **done** (v1)

### Debug logging
Log diagnostic info to a file (render timings, buffer stats, signals). Enabled with `--debug`.
- **done** (v1) — default log file: `/tmp/slit-$PID.log` or custom via `--log-file`

### Man page / --help
Generate a man page and have a thorough `--help` output.
- **done** (v1) — `slit --generate-man` (hidden flag), thorough `--help`

### Keybinding bar
Show active keybindings on RHS of status line (e.g. `q:quit`).
- **done** (v1)

### SIGTSTP handling
Suspend and resume gracefully (Ctrl+Z), restoring terminal state.
- **done** (v1) — C implementation, Go/Rust handle via framework

---

## V2 ideas

### Configurable status line format
Allow customizing what appears in the status line (line count, byte count, elapsed time, etc).
- **to be considered**

### Multiple input sources
Accept multiple inputs and show them in split panes or interleaved with labels.
- **to be considered**

### Follow mode for files
Like `tail -f`, watch a file and stream new lines. Would complement the existing pipe-based streaming.
- **to be considered**

### Syntax highlighting
Apply basic syntax highlighting to known file types within the pane.
- **to be considered**

### Regex highlighting
Highlight lines matching a pattern, e.g. `--highlight ERROR` or `--highlight 'WARN|ERR'`.
- **to be considered**

## V3 (maybe never)

### Named pipe / multi-input
Accept multiple inputs and show them in split panes or interleaved with labels.
- **deferred** (v3)
