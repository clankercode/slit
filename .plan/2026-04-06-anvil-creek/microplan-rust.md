# Micro Plan — Rust Implementation

Each step produces a testable increment. Steps are ordered by dependency.
This plan mirrors the Go microplan structure and targets identical behavior.

## Step 1: Project scaffold + CLI flags

**Goal**: `rust/slit --help` works, all flags defined via clap derive.

- `cargo init rust/`
- `Cargo.toml`: dependencies — `clap` (features: `derive`), `clap_complete`, `ratatui`,
  `crossterm` (features: `event-stream`), `tokio` (features: `full`), `serde`
  (features: `derive`), `toml`
- `src/main.rs`: `#[derive(Parser)]` struct with all flags from cli.md
  - Core: `-n/--lines`, `--max-lines`, `-o/--output`, `-a/--append`, `--tee-format`
  - Display: `-l/--line-numbers`, `--color`, `-w/--wrap`, `-t/--timestamp`, `--truncation-char`
  - Layout: `--layout`, `--box`, `--rounded`, `--compact`, `--minimal`, `--none`, `--quote`, `--quote-bg`
  - Progress: `--spinner`
  - Debug: `-d/--debug`, `--log-file`
  - Meta: `--help`, `--version` (from `env!("CARGO_PKG_VERSION")`), `--generate-man`
- `src/completion.rs`: `completion` subcommand via `#[derive(Subcommand)]`
  - `clap_complete::aot::generate()` for `Shell::Bash`, `Shell::Zsh`, `Shell::Fish`
- `#[tokio::main]` async entry point that prints help/version or delegates to completion
- `--generate-man`: output roff man page via `clap_mangen` (add dep) to stdout
- **Test**: `cargo build && ./target/debug/slit --help` shows all flags

## Step 2: Config file loading

**Goal**: `~/.config/slit/config.toml` loaded, merged with CLI flags (CLI wins).

- `src/config.rs`: `Config` struct with serde `#[derive(Deserialize)]`
  - Nested: `DisplayConfig`, `BufferConfig`, `SpinnerConfig`, `QuoteConfig`, `DebugConfig`
  - Matches cli.md TOML spec exactly
- Find config path: `$XDG_CONFIG_HOME/slit/config.toml` or `~/.config/slit/config.toml`
  - Use `dirs::config_dir()` or manual `std::env` + `std::path`
- Merge strategy: `Config::default()` → config file values → CLI flag overrides
  - CLI flags that are `None` (not set) don't override config
  - Use `Option<T>` for all CLI flags to detect "not set"
- `resolve()` method returns final `ResolvedConfig` with all fields populated
- **Test**: `#[cfg(test)]` unit tests for config merge: defaults, file overrides, CLI overrides, CLI > file > default

## Step 3: Ring buffer

**Goal**: Bounded circular buffer for `LineEntry` records.

- `src/buffer.rs`: `RingBuffer` struct with `VecDeque<LineEntry>`
- `LineEntry` struct:
  ```rust
  struct LineEntry {
      text: String,
      time: std::time::Instant,   // arrival time (for --timestamp)
      line_num: u64,              // global line number (1-indexed)
  }
  ```
- `RingBuffer::new(capacity: usize)` — default 50k, configurable via `--max-lines`
- Methods: `push(entry: LineEntry)`, `lines() -> &[LineEntry]`, `len() -> usize`,
  `capacity() -> usize`, `total_lines() -> u64` (including evicted)
- `total_bytes: u64` field — incremented on each push by line byte length
- Drops oldest when full (VecDeque auto-trims when capacity exceeded)
- `is_empty()`, `last_n(n: usize) -> &[LineEntry]` for rendering
- **Test**: unit test push/evict/capacity/total_lines/total_bytes, verify oldest drops

## Step 4: Async stdin pipeline (minimal layout, no features)

**Goal**: `echo -e "line1\nline2\nline3" | ./slit -n 2` shows last 2 lines + status on stderr.

- `src/app.rs`: `App` struct holding state (buffer, config, terminal dimensions, eof flag)
- Terminal setup:
  - Check `isatty(STDERR_FILENO)` — if not, enter passthrough mode
    - Also check `SLIT_FORCE_RENDER=1` env var to override tty check
  - Passthrough mode: `tokio::io::copy(stdin, stdout)` then exit
  - Render mode: enter crossterm raw mode on stderr
    - `crossterm::terminal::enable_raw_mode()`
    - Open `/dev/tty` for keyboard input (separate from piped stdin)
- Stdin reader task:
  ```rust
  tokio::spawn(async move {
      let reader = tokio::io::BufReader::new(tokio::io::stdin());
      let mut lines = reader.lines();
      while let Ok(Some(line)) = lines.next_line().await {
          tx.send(line).await.unwrap();
      }
      tx.send(None).await.unwrap(); // signal EOF
  });
  ```
  - Channel: `tokio::sync::mpsc::channel::<Option<String>>(1024)`
- Main event loop via `tokio::select!`:
  ```rust
  tokio::select! {
      line = rx.recv() => { /* push to buffer */ },
      Some(Ok(event)) = EventStream::new().next() => { /* handle key/resize */ },
      _ = tick.tick() => { /* render */ },
  }
  ```
- Rendering with `ratatui::Viewport::Inline(height)` — NOT alternate screen
  - `Terminal::with_options(TerminalOptions { viewport: Viewport::Inline(height), .. })`
  - Write backend to stderr: `CrosstermBackend::new(std::io::stderr())`
- Minimal layout: N content lines + 1 status line
- Status: `Streaming... (N lines)  q:quit`
- EOF → status changes to `Done. (N lines)` → wait 100ms → exit
- `q` or `ctrl+c` quits
- Content lines are raw (no trimming, no features yet)
- Cleanup: `disable_raw_mode()` in drop guard (struct with panic-safe drop)
- **Test**: `seq 1 100 | ./slit -n 5` shows lines 96-100 + status on stderr

## Step 5: Line trimming + truncation indicator

**Goal**: Lines trimmed to terminal width, configurable truncation char appended.

- `src/render.rs`: `trim_line(line: &str, width: usize, trunc_char: &str) -> String`
- Width from `crossterm::event::Event::Resize(w, h)` or initial terminal size
- Truncation char from `--truncation-char` (default: `…`)
- Account for layout chrome width (side borders, line number gutter, timestamp prefix)
  - Content width = terminal_width - layout_side_cost - gutter_width
- Must handle Unicode correctly: use `unicode-width` crate or ratatui's `LineWidth`
  - Count visible columns, not byte length or char count
- **Test**: unit test with various widths, Unicode chars, custom truncation char

## Step 6: Layout system (all 6 layouts)

**Goal**: `--layout=box|rounded|compact|minimal|none|quote` all render correctly.

- `src/layout.rs`: `Layout` enum with variants for all 6 layouts
- Each layout variant defines chrome costs:
  ```rust
  struct ChromeCost {
      top_lines: u16,
      bottom_lines: u16,
      side_width: u16,
  }
  ```

  | Layout | top_lines | bottom_lines | side_width |
  |--------|-----------|--------------|------------|
  | box    | 1         | 1            | 4          |
  | rounded| 1         | 1            | 4          |
  | compact| 1         | 1            | 0          |
  | minimal| 0         | 1            | 0          |
  | none   | 0         | 0            | 0          |
  | quote  | 0         | 0            | 2          |

- Rendering approach using ratatui widgets:
  - **Box**: `Block::default().borders(Borders::ALL).border_type(BorderType::Plain)`
    with title in top border, status in bottom border
  - **Rounded**: same but `.border_type(BorderType::Rounded)`
  - **Compact**: colored top bar via `Block::default().style(Style::default().bg(color))`
    + separate status line below
  - **Minimal**: just content lines + status line (no Block widget)
  - **None**: just content lines, no status, spinner disabled
  - **Quote**: left bar `▌` on every line, title + status inside content area
    - Content width = term_width - 2, effective data lines = N - 2
- Shortcut flags (`--box`, `--compact`, etc.) resolve to `Layout` enum
  - Conflict detection: if multiple shortcuts, last one wins (or error)
- Content width = `term_width - side_cost`, rendered line count adjusts for top/bottom chrome
- `viewport_height = content_lines + top_lines + bottom_lines`
  - Pass to `Viewport::Inline(viewport_height)`
- **Test**: visual check of each layout; unit test chrome cost values

## Step 7: Status line (spinner + line count + progress bar + keybinding hints)

**Goal**: Full status line with animated spinner and optional progress bar.

- `src/spinner.rs`: spinner frame sets as static arrays
  - `braille`: `["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]`
  - `dots`: `["⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"]`
  - `arrows`: `["←", "↖", "↑", "↗", "→", "↘", "↓", "↙"]`
  - `off`: empty
- Spinner state: frame index, advanced on each render tick
- Status format: `<spinner> <label> (<count> lines) [<progress>]  q:quit`
  - Rendered as `ratatui::text::Line` with `Span`s for styling
- Progress bar: only when stdin is a regular file with known size
  - `std::fs::metadata("/dev/stdin")` or `fstat` via `nix::fcntl::fstat` on stdin fd
  - Check `st_mode` is regular file and `st_size > 0`
  - Track `total_bytes` in buffer; progress = `total_bytes / file_size`
  - Bar format: `[====      ]` proportional fill
  - After EOF: full bar `[==========]`
- Keybinding hints on RHS: `q:quit` (right-aligned via `Line::default().spans` + rightmost `Span`)
- After EOF: spinner stops, label changes to `Done.`
- **Test**: `cat /tmp/bigfile | ./slit` shows progress bar; unit test spinner frame cycling

## Step 8: Line numbers

**Goal**: `-l` flag shows dim-colored line numbers.

- `src/render.rs`: `format_line_with_number(line: &str, line_num: u64, max_num: u64, width: usize) -> String`
- Right-aligned number in gutter, dim/grey color via `ratatui::style::Style::default().fg(Color::DarkGray)`
- Gutter width: enough for max digits in buffer (`digits(max_line_num) + 1` space)
- Number counts from 1 (total lines, not just visible)
- Render as `Span::styled(format!("{:>width$} ", line_num), dim_style)` + `Span::raw(line)`
- Gutter width reduces content area
- **Test**: `seq 1 100 | ./slit -n 5 -l` — check stderr contains right-aligned numbers

## Step 9: ANSI color passthrough

**Goal**: `--color=auto|always|never` handles colored input.

- `src/render.rs`:
  - `strip_ansi(s: &str) -> String` — removes all CSI/OSC sequences
  - `ansi_visible_width(s: &str) -> usize` — width excluding escape sequences
  - State machine parser (not regex) for ESC [ ... m sequences (SGR)
- `auto`: detect if stderr is tty → pass through; otherwise strip
- `always`: always pass through ANSI codes (no stripping)
- `never`: always strip
- ANSI-aware width calculation: `ansi_visible_width()` used in trimming
  - Trim must not break mid-sequence; find safe boundary before truncation char
- ratatui can render ANSI-styled text via `text::Line::from_styled()` or
  `ratatui::text::Text::from_raw()` — verify approach for inline ANSI
  - Alternative: parse ANSI into `Span` sequence with styles
- **Test**: `echo -e "\033[31mred\033[0m normal" | ./slit --color=always` — check colored output;
  unit test `strip_ansi()` and `ansi_visible_width()`

## Step 10: Timestamp prefix

**Goal**: `-t` flag prepends `HH:MM:SS` to each line.

- `LineEntry.time` (`std::time::Instant`) recorded on push to buffer
- Convert to `chrono::NaiveTime` or `std::time::SystemTime` for formatting
  - Actually: record `SystemTime` in `LineEntry`, not `Instant`
  - Format: `HH:MM:SS` via `chrono` or manual `SystemTime` formatting
- Prepend before line number (if both enabled): `[HH:MM:SS] NNN text...`
- Timestamp gutter adds fixed width (~10 chars: `[HH:MM:SS] `) reducing content width
- **Test**: `seq 1 5 | ./slit -t` — check stderr lines start with timestamps

## Step 11: Wrap mode

**Goal**: `--wrap` wraps long lines instead of truncating.

- `src/render.rs`: `wrap_line(line: &str, width: usize) -> Vec<String>` — returns multiple display strings
- ANSI-aware wrapping: don't break in the middle of an escape sequence
  - Carry active ANSI state forward to wrapped continuation lines
  - Each wrapped sub-line must re-emit any open color sequences
- Each wrapped sub-line counts against the visible line budget
  - If one logical line produces 3 wrapped lines, it uses 3 of the N-line budget
  - Walk buffer from newest backwards, accumulating wrapped lines until budget full
- When `--wrap` is off: truncate (step 5 behavior)
- **Test**: `python3 -c "print('x'*200)" | ./slit -n 5 --wrap` — check wrapped output on stderr

## Step 12: Tee to file

**Goal**: `-o file` writes input to file, `-a` appends.

- `src/tee.rs`: `TeeWriter` struct
  - Opens file on startup (`File::create` or `OpenOptions::new().append(true).open`)
  - `write_line(&mut self, line: &str)` — writes line + newline
  - `write_display_line(&mut self, formatted: &str)` — writes formatted output
- `--tee-format=raw`: write original line as received (from stdin, before any processing)
- `--tee-format=display`: write formatted line (with line numbers, timestamps, truncation)
  - **Important**: only write once per line arrival (NOT on every render frame)
  - Generate display format at line arrival time, not render time
- Close file on shutdown via `Drop` impl or explicit close
- Passthrough on stdout continues regardless (handled in passthrough mode check)
- **Test**: `seq 1 10 | ./slit -o /tmp/test.log && cat /tmp/test.log` — verify contents;
  test append mode; test display format

## Step 13: Window title

**Goal**: Terminal title set to `slit` on startup, restored on exit.

- `src/render.rs` or `src/app.rs`: `set_title(title: &str)`
  - Write OSC escape to stderr: `\x1b]0;slit\x07`
  - Use `crossterm::execute!(stderr(), SetTitle("slit"))` or raw write
- Save original title on startup (query unreliable — skip, just set to empty on exit)
- Set in `App::new()` after terminal init
- Restore: set title to empty string `""` in drop guard / shutdown
- **Test**: `echo hi | ./slit` — terminal title changes to "slit", reverts on exit

## Step 14: Render debouncing

**Goal**: Cap at ~30fps even if input arrives faster.

- `tokio::time::interval(Duration::from_millis(33))` creates tick stream
- In `tokio::select!`, the tick arm triggers rendering
- Lines accumulate in buffer between ticks (they arrive via the stdin arm)
- Only re-render when `dirty` flag is set (new lines arrived or resize occurred)
- `dirty` flag set on: stdin line arrival, `Event::Resize`, config change
- `dirty` flag cleared after render
- The interval `missed_tick_behavior = MissedTickBehavior::Skip` to avoid burst renders
- **Test**: `cat /dev/urandom | lines | ./slit` — should not flicker excessively;
  debug log should show ~30 renders/sec max

## Step 15: Debug logging

**Goal**: `--debug` writes diagnostic info to log file.

- `src/debug.rs`: `DebugLogger` struct
  - `log(&self, format: &str, args: ...)`
  - Thread-safe via `std::sync::Mutex<File>` (or `tokio::sync::Mutex`)
  - Uses `std::fmt::Arguments` or `write!` macro
- Default path: `/tmp/slit-$PID.log` (`std::process::id()`)
  - `$PID` in config resolved at runtime
- Configurable via `--log-file`
- Enabled via `--debug` or config `debug.enabled = true`
- Log entries: timestamp + message
  - Render timings, buffer stats, signal events, config resolution
- Create file lazily on first log call
- **Test**: `echo hi | ./slit --debug && cat /tmp/slit-*.log` — check log contents

## Step 16: Done summary + clean exit

**Goal**: EOF shows summary, clean terminal restore, exit 0.

- On EOF from stdin reader task (receives `None` through channel):
  - Set `eof = true` flag
  - Render final frame: `Done. (N lines) [===] q:quit`
  - Wait briefly (100ms via `tokio::time::sleep`) so user sees summary
  - Exit the select loop
- Terminal restore on all exit paths:
  - Drop guard struct (created in `main`):
    ```rust
    struct RawModeGuard;
    impl Drop for RawModeGuard {
        fn drop(&mut self) {
            let _ = disable_raw_mode();
            let _ = execute!(stderr(), SetTitle(""));
        }
    }
    ```
  - crossterm `disable_raw_mode()` restores terminal
  - ratatui `Terminal::restore()` or drop clears inline viewport area
- Exit code: 0 on clean EOF, 1 on error
  - Return `Result<()>` from `main`, use `std::process::ExitCode`
- Handle SIGINT gracefully:
  - `ctrl+c` in crossterm delivers `Event::Key(KeyCode::Char('c'), KeyModifiers::CONTROL)`
  - Set `should_quit = true`, exit loop cleanly
- Handle SIGTERM/SIGHUP:
  - `tokio::signal::unix::SignalKind::terminate()` / `hangup()`
  - Or check in select loop
- **Test**: `echo done | ./slit; echo $?` → 0; `./slit --invalid 2>/dev/null; echo $?` → 1 (clap handles this)

## Step 17: Shell completions + man page + --help polish

**Goal**: Production-ready CLI surface.

- `src/completion.rs`:
  - `completion` subcommand: `slit completion bash|zsh|fish`
  - Use `clap_complete::aot::generate(shell, &mut cmd, "slit", &mut stdout)`
  - `clap_complete::aot::Shell` enum handles Bash, Zsh, Fish, etc.
- `--generate-man`:
  - Add `clap_mangen` dependency
  - `clap_mangen::Man::new(cmd).render(&mut stdout)`
  - Produces roff-format man page
- Enum flags get value parsers:
  - `--color`: `value_parser!(["auto", "always", "never"])`
  - `--spinner`: `value_parser!(["braille", "dots", "arrows", "off"])`
  - `--layout`: `value_parser!(["box", "rounded", "compact", "minimal", "none", "quote"])`
  - `--tee-format`: `value_parser!(["raw", "display"])`
- File path flags: `.value_hint(ValueHint::FilePath)` for `-o`, `--log-file`
- Polish `--help` output to match cli.md draft exactly
  - `#[command(about = "streaming terminal viewer")]`
  - `#[command(after_help = "EXAMPLES\n    tail -f /var/log/syslog | slit\n    ...")]`
- Generate static completion files to `completions/` via build script or manual step
- **Test**: `./slit completion bash | head -5` produces valid bash;
  `./slit --generate-man | head -5` produces roff

## Step 18: Integration + justfile

**Goal**: Build and test infrastructure wired up.

- Verify justfile targets from testing.md work for Rust:
  - `just build-rust`: `cd rust && cargo build --release && cp target/release/slit ../target/slit-rust`
  - `just test-rust`: `cd rust && cargo test`
- Run `cargo test` — all unit tests pass
- Run bats integration tests against `target/slit-rust`
- Verify all test cases from testing.md:
  - Basic behavior, flags, layouts, tee, edge cases, config, completions
- `cargo clippy` passes with no warnings
- `cargo fmt --check` passes
- **Test**: `just test-rust && SLIT_BIN=target/slit-rust just test-integration`

---

## Dependency Graph

```
Step 1 (CLI/Clap) ──► Step 2 (Config/Serde) ──► Step 3 (Ring Buffer)
                                                     │
                                                     ▼
                                              Step 4 (Async Pipeline) ──► Step 5 (Trim) ──► Step 6 (Layouts)
                                                                                           │
                                                                             ┌─────────────┤
                                                                             ▼             ▼
                                                                       Step 7 (Status)  Step 13 (Title)
                                                                             │
                                                           ┌─────────────────┼──────────────┐
                                                           ▼                 ▼              ▼
                                                      Step 8 (LineNum)  Step 9 (ANSI)  Step 10 (Timestamp)
                                                           │                 │              │
                                                           └────────┬────────┘              │
                                                                    ▼                       │
                                                               Step 11 (Wrap) ◄─────────────┘
                                                                    │
                                                                    ▼
                                                               Step 12 (Tee)
                                                                    │
                                                                    ▼
                                                             Step 14 (Debounce)
                                                                    │
                                                                    ▼
                                                              Step 15 (Debug)
                                                                    │
                                                                    ▼
                                                            Step 16 (Exit/Cleanup)
                                                                    │
                                                                    ▼
                                                           Step 17 (Completions/Man)
                                                                    │
                                                                    ▼
                                                              Step 18 (Justfile/CI)
```

Steps 8, 9, 10 are independent of each other and can be done in any order after step 7.

## Rust-Specific Implementation Notes

### Crate Versions (as of 2026-04)

| Crate | Version | Notes |
|-------|---------|-------|
| `ratatui` | 0.30 | `Viewport::Inline(u16)` takes height in lines |
| `crossterm` | 0.29 | `event-stream` feature enables `EventStream` (impls `futures::Stream`) |
| `tokio` | 1.x | `full` feature for macros, time, sync, io |
| `clap` | 4.x | `derive` feature for `#[derive(Parser)]` |
| `clap_complete` | 4.x | `aot::generate()` for shell completions |
| `clap_mangen` | 0.2+ | For `--generate-man` |
| `serde` | 1.x | `derive` feature |
| `toml` | 0.8+ | `from_str()` for config parsing |

### Inline Rendering with ratatui

```rust
let backend = CrosstermBackend::new(std::io::stderr());
let mut terminal = Terminal::with_options(
    backend,
    TerminalOptions {
        viewport: Viewport::Inline(height),
    },
)?;
// terminal.draw(|f| { ... }) renders to stderr in-place
```

### Main Event Loop Pattern

```rust
let mut events = EventStream::new();
let mut tick = tokio::time::interval(Duration::from_millis(33));
tick.set_missed_tick_behavior(MissedTickBehavior::Skip);

loop {
    tokio::select! {
        // Stdin lines
        Some(line) = rx.recv() => {
            if let Some(line) = line {
                buffer.push(LineEntry::new(line));
                dirty = true;
            } else {
                eof = true;
                dirty = true;
            }
        }
        // Terminal events (keyboard, resize)
        Some(Ok(event)) = events.next() => {
            match event {
                Event::Key(key) => { /* handle q, ctrl+c */ },
                Event::Resize(w, h) => { dirty = true; /* update dimensions */ },
                _ => {}
            }
        }
        // Render tick (~30fps)
        _ = tick.tick() => {
            if dirty {
                render(&mut terminal, &buffer, &config)?;
                dirty = false;
            }
            if eof {
                tokio::time::sleep(Duration::from_millis(100)).await;
                break;
            }
        }
    }
}
```

### Passthrough Mode

```rust
use std::os::unix::io::AsRawFd;

fn is_stderr_tty() -> bool {
    if std::env::var("SLIT_FORCE_RENDER").as_deref() == Ok("1") {
        return true;
    }
    unsafe { libc::isatty(libc::STDERR_FILENO) != 0 }
}

async fn passthrough() -> std::io::Result<()> {
    tokio::io::copy(&mut tokio::io::stdin(), &mut tokio::io::stdout()).await?;
    Ok(())
}
```

### Keyboard Input from /dev/tty

When stdin is piped, keyboard events come from the terminal via crossterm's event stream.
crossterm's `EventStream` reads from the terminal internally — it uses `/dev/tty` on Unix
automatically when stdin is not a tty. No manual `/dev/tty` opening needed when using
`EventStream::new()`.

### File Structure

```
rust/
├── Cargo.toml
├── Cargo.lock
├── src/
│   ├── main.rs       # clap setup, entry point, #[tokio::main]
│   ├── app.rs        # App struct, event loop, terminal setup/teardown
│   ├── config.rs     # TOML config loading, merge logic
│   ├── layout.rs     # Layout enum, chrome costs, Block construction
│   ├── render.rs     # Line trimming, ANSI handling, wrapping, formatting
│   ├── buffer.rs     # RingBuffer<LineEntry>, push/evict/total tracking
│   ├── tee.rs        # TeeWriter for -o/-a/--tee-format
│   ├── spinner.rs    # Spinner frame sets, frame cycling
│   ├── completion.rs # Shell completion subcommand
│   └── debug.rs      # DebugLogger, file-based logging
```
