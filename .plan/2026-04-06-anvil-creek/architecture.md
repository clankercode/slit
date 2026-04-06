# Architecture — Per-Language Details

## Shared Architecture

All three implementations share the same logical architecture:

```
stdin ──► line reader ──► ring buffer ──► renderer ──► stderr
                │                              │
                └── tee writer ──► file         └── terminal control
                                                      │
                                     ┌────────────────┤
                                     │                │
                                  layout engine   status line
                                     │
                              ┌──────┼──────┐
                              │      │      │
                            box   compact  minimal  ...
```

### Data Flow

1. **Line reader**: reads stdin line-by-line (non-blocking)
2. **Ring buffer**: bounded VecDeque/circular buffer, 50k default
3. **Tee writer**: writes raw/display lines to file (if `-o`)
4. **Renderer**: debounced (~30fps), renders current buffer state to stderr
5. **Layout engine**: applies chrome (borders, bars) around content
6. **Status line**: spinner + count + progress + keybinding hints
7. **Terminal control**: raw mode setup, SIGWINCH, cleanup on exit

### Rendering Mode: Inline (NOT alternate screen)

All three implementations must use **inline rendering** — render in-place within the normal terminal scrollback, NOT in an alternate screen buffer (like vim/less do). This gives a `tail -f` feel where output persists in terminal history after slit exits.

- **Go (Bubbletea)**: Do NOT use `tea.WithAltScreen()`. Use the default inline rendering.
- **Rust (ratatui)**: Use `Viewport::Inline(height)` — renders in-place without taking over the full terminal.
- **C**: Already inline by default (writing directly to stderr with cursor movements).

### Startup Sequence

```
1. Parse CLI flags
2. Load config file (if exists)
3. Check stderr isatty → if not, enter passthrough mode
4. Open /dev/tty for keyboard input (if needed)
5. Enter semi-raw terminal mode (keep ISIG for Ctrl-C)
6. Set window title (OSC escape)
7. Start stdin reader
8. Enter render loop
```

### Shutdown Sequence

```
1. Detect EOF on stdin
2. Render done summary
3. Restore window title
4. Restore terminal mode
5. Close /dev/tty
6. Close tee file (if open)
7. Exit 0
```

### Signal Handling

| Signal | Action |
|--------|--------|
| SIGINT | Set flag, clean exit via shutdown sequence |
| SIGPIPE | Ignore (`SIG_IGN`), check `EPIPE` on stdout writes |
| SIGWINCH | Set flag, recalculate width, re-render |
| SIGTERM | Clean exit via shutdown sequence |
| SIGHUP | Clean exit via shutdown sequence |
| SIGTSTP | Restore terminal, suspend, re-init on resume |

---

## Go Implementation

### Dependencies

| Library | Purpose |
|---------|---------|
| `github.com/charmbracelet/bubbletea` | TUI framework |
| `github.com/charmbracelet/lipgloss` | Styling, layout, borders |
| `github.com/charmbracelet/bubbles/spinner` | Spinner widget |
| `github.com/charmbracelet/bubbles/viewport` | Scrollable content pane |
| `github.com/spf13/cobra` | CLI flag parsing + completions |
| `github.com/BurntSushi/toml` | Config file parsing |

### Key Patterns

- **Stdin reading**: goroutine with `bufio.Scanner`, sends `lineMsg` via `p.Send()`
- **Bubbletea auto-detects piped stdin**: opens `/dev/tty` for key events automatically
- **Render debouncing**: Bubbletea's `tea.Tick` at ~33ms interval
- **SIGWINCH**: Bubbletea handles internally, delivers `tea.WindowSizeMsg`
- **Layouts**: lipgloss `JoinVertical` composes top bar + viewport + status bar
- **Config**: Load TOML into struct, merge with CLI flags (CLI wins)

### File Structure

```
go/
├── go.mod
├── go.sum
├── main.go          # cobra setup, entry point
├── model.go         # bubbletea model, Init/Update/View
├── config.go        # TOML config loading
├── layouts.go       # layout definitions (box, compact, minimal, etc.)
├── render.go        # content rendering, ANSI handling, truncation
├── buffer.go        # ring buffer implementation
├── tee.go           # tee file writer
├── spinner.go       # spinner styles
├── progress.go      # progress bar (auto-detect file size)
├── completion.go    # shell completion generation
└── debug.go         # debug logging
```

---

## Rust Implementation

### Dependencies

| Crate | Purpose |
|-------|---------|
| `ratatui` | TUI framework (widgets, layout) |
| `crossterm` (with `event-stream` feature) | Terminal control, events, SIGWINCH |
| `tokio` | Async runtime |
| `clap` (with `derive`) | CLI flag parsing |
| `clap_complete` | Shell completion generation |
| `serde` + `toml` | Config file parsing |

### Key Patterns

- **Stdin reading**: `tokio::spawn` with `tokio::io::BufReader::lines()`, sends via `tokio::sync::mpsc`
- **Main loop**: `tokio::select!` multiplexes stdin lines, terminal events, render ticks
- **Render debouncing**: `tokio::time::interval(Duration::from_millis(33))`
- **SIGWINCH**: crossterm handles internally, delivers `Event::Resize`
- **Layouts**: ratatui `Layout::split()` with `Block` widget for borders
- **Config**: serde deserialize TOML into struct, merge with CLI flags

### File Structure

```
rust/
├── Cargo.toml
├── Cargo.lock
├── src/
│   ├── main.rs      # clap setup, entry point, tokio main
│   ├── app.rs       # application state, event loop
│   ├── config.rs    # TOML config loading
│   ├── layout.rs    # layout definitions
│   ├── render.rs    # content rendering, ANSI handling
│   ├── buffer.rs    # ring buffer
│   ├── tee.rs       # tee file writer
│   ├── spinner.rs   # spinner styles
│   ├── progress.rs  # progress bar
│   ├── completion.rs # shell completion generation
│   └── debug.rs     # debug logging
```

---

## C Implementation

### Dependencies

- **None** (POSIX standard library only: termios, poll, signal, unistd)
- Optional: a minimal TOML parser (hand-rolled or single-header lib)

### Key Patterns

- **Stdin reading**: `poll()` on STDIN_FILENO + tty fd, `getline()` or line-buffered `read()`
- **Render debouncing**: track last render time, skip if < 33ms since last render
- **SIGWINCH**: manual `sigaction(SIGWINCH, ...)` handler, sets flag, main loop checks
- **Terminal mode**: semi-raw via `termios` (disable ICANON + ECHO, keep ISIG + OPOST)
- **Layouts**: hand-rolled ANSI cursor movement + box-drawing characters
- **Config**: hand-rolled TOML/ini parser or simple key=value format

### Terminal Control

```c
// Semi-raw mode (less-style, not full cfmakeraw)
struct termios orig_termios;
tcgetattr(fd, &orig_termios);
struct termios raw = orig_termios;
raw.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
raw.c_oflag |= (OPOST | ONLCR);
raw.c_cc[VMIN] = 1;
raw.c_cc[VTIME] = 0;
tcsetattr(fd, TCSADRAIN, &raw);
```

### File Structure

```
c/
├── Makefile
├── slit.h           # shared types, constants
├── main.c           # entry point, arg parsing
├── config.c         # config file loading
├── config.h
├── layout.c         # layout rendering
├── layout.h
├── render.c         # content rendering, ANSI handling
├── render.h
├── buffer.c         # ring buffer
├── buffer.h
├── tee.c            # tee file writer
├── tee.h
├── spinner.c        # spinner styles
├── spinner.h
├── progress.c       # progress bar
├── progress.h
├── terminal.c       # termios, signals, cleanup
├── terminal.h
├── debug.c          # debug logging
└── debug.h
```
