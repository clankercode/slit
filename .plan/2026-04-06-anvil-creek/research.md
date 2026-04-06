# Research Findings

## Terminal I/O & Pipe Detection

### Key Pattern: Check stderr, not stdout

Since slit renders to stderr, the critical check is `isatty(STDERR_FILENO)`:
- If stderr is not a tty → enter passthrough mode (spool stdin→stdout, no rendering)
- This handles `cmd | slit 2>/dev/null` correctly

### Keyboard Input via /dev/tty

When stdin is piped, we need a separate fd for keyboard events:
- **Go (Bubbletea)**: handles this automatically — detects piped stdin, opens `/dev/tty`
- **Rust (crossterm)**: we open `/dev/tty` manually for event reading
- **C**: open `/dev/tty` via `open("/dev/tty", O_RDWR)` for keyboard input

### Terminal Mode

Use **semi-raw** mode (like `less`), NOT full `cfmakeraw()`:
- Disable `ICANON` (no line buffering) and `ECHO` (no echo)
- Keep `ISIG` (so Ctrl-C still generates SIGINT)
- Keep `OPOST | ONLCR` (so `\n` → `\r\n`)
- This gives us immediate character reads without breaking signal handling

### Signal Handling Pattern

**Deferred signals**: signal handlers just set a flag bit. Main event loop checks flags.

| Signal | Handler Action |
|--------|---------------|
| SIGINT | Set flag → clean exit |
| SIGPIPE | `SIG_IGN` → check `EPIPE` on write |
| SIGWINCH | Set flag → recalculate width → re-render |
| SIGTERM/SIGHUP | Clean exit |
| SIGTSTP | Restore terminal → suspend → re-init on resume |

## Go (Bubbletea) Architecture

### Stdin → Bubbletea Pipeline

Bubbletea auto-detects piped stdin and opens `/dev/tty` for key events. The recommended pattern:

```go
go func() {
    scanner := bufio.NewScanner(os.Stdin)
    for scanner.Scan() {
        p.Send(lineMsg(scanner.Text()))
    }
}()

p := tea.NewProgram(model)
p.Run()
```

### Layout with lipgloss

```go
view := lipgloss.JoinVertical(lipgloss.Left,
    topBarStyle.Width(w).Render("slit"),
    viewport.View(),
    statusBarView(w),
)
```

- `lipgloss.NewStyle().Border()` for box borders
- `lipgloss.NewStyle().Background().Width(w)` for colored bars (fills entire line)
- `JoinVertical` composes sections

### Key Libraries

| Library | Purpose |
|---------|---------|
| `bubbletea` | TUI framework |
| `lipgloss` | Styling, layout, borders |
| `bubbles/spinner` | Spinner widget |
| `bubbles/viewport` | Scrollable content pane |
| `cobra` | CLI flags + completions |
| `BurntSushi/toml` | Config parsing |

## Rust (ratatui) Architecture

### Recommended Stack

ratatui + crossterm + tokio. This is the consensus best-in-class combination.

### Main Loop with tokio::select!

```rust
tokio::select! {
    Some(line) = rx.recv() => { /* process stdin line */ },
    Some(Ok(event)) = events.next() => { /* process terminal event */ },
    _ = tick.tick() => { /* render frame */ },
}
```

### Key: Inline Viewport (NOT alternate screen)

For slit, use **inline rendering** — no alternate screen. Output persists in terminal scrollback like `tail -f`.

- **Go (Bubbletea)**: Do NOT use `tea.WithAltScreen()`. Default inline rendering is correct.
- **Rust (ratatui)**: Use `ratatui::Viewport::Inline(height)` — renders in-place without taking over the full terminal.

### SIGWINCH

crossterm handles internally via `signal-hook` on Unix, delivers `Event::Resize(width, height)`.

### Key Crates

| Crate | Purpose |
|-------|---------|
| `ratatui` | TUI framework, widgets |
| `crossterm` (event-stream) | Terminal control, async events |
| `tokio` | Async runtime |
| `clap` + `clap_complete` | CLI flags + completions |
| `serde` + `toml` | Config parsing |

## C (Raw) Architecture

### Event Loop with poll()

```c
struct pollfd fds[2];
fds[0].fd = STDIN_FILENO;
fds[0].events = POLLIN;
fds[1].fd = tty_fd;
fds[1].events = POLLIN;

while (poll(fds, 2, 33) >= 0) {  // 33ms timeout = ~30fps
    if (fds[0].revents & POLLIN) /* read stdin line */
    if (fds[1].revents & POLLIN) /* read key event */
    if (winch_flag) { /* recalc width, re-render */ winch_flag = 0; }
    render_if_needed();
}
```

### No External Dependencies

Pure POSIX. The only challenge is TOML parsing — options:
1. Hand-rolled simple parser (just `[section]` and `key = "value"`)
2. Single-header TOML library (e.g., `tomlc` from cinirG on GitHub)
3. Simplify to INI format (no nested tables)

## Shell Completions — Best Practice

### Pattern: `slit completion <shell>`

Used by `gh`, `kubectl`, `ripgrep`, `fd`, `bat`, `helm`. The binary generates completions on demand.

### Per-Language Approach

| Language | Library | Approach |
|----------|---------|----------|
| Go | cobra | Built-in `completion` subcommand |
| Rust | clap_complete | `clap_complete::generate()` via subcommand |
| C | Hand-written | Embed scripts as string literals, output via flag |

### Static Files for Packagers

Ship pre-generated scripts in `completions/` for convenience. Install paths:

| Shell | System | User |
|-------|--------|------|
| Bash | `/usr/share/bash-completion/completions/slit` | `~/.local/share/bash-completion/completions/slit` |
| Zsh | `/usr/share/zsh/site-functions/_slit` | `~/.zfunc/_slit` |
| Fish | `/usr/share/fish/vendor_completions.d/slit.fish` | `~/.config/fish/completions/slit.fish` |

## Progress Bar — Auto-Detection

When stdin is a regular file (not a pipe), `fstat(STDIN_FILENO)` gives `st_size`. Track bytes read to calculate percentage:

```c
struct stat st;
if (fstat(STDIN_FILENO, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
    float progress = (float)bytes_read / st.st_size;
    // render progress bar
}
```

For pipes, no size is available → show spinner + line count only.
