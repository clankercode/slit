# Feature Matrix — slit

Comprehensive feature tracking across all three implementations (Go, Rust, C).

Verified: 2026-04-08

---

## Core Behavior

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 1 | Fixed-height scrolling pane | Rolling buffer of last N lines, in-place re-render | ✅ | ✅ | ✅ | ✅ Done |
| 2 | Auto-trim to terminal width | Detect width + SIGWINCH, trim lines to fit | ✅ | ✅ | ✅ | ✅ Done |
| 3 | `-n` line count flag | 0=auto (2/3 terminal height, min 10); passthrough: 0=pipe-all | ✅ | ✅ | ✅ | ✅ Done |
| 4 | Spinner status line | Braille/dots/arrows spinner + label + line count | ✅ | ✅ | ✅ | ✅ Done |
| 5 | Done summary on EOF | Replace spinner with `Done. (N lines, Xbytes)` | ✅ | ✅ | ✅ | ✅ Done |

## Output / Tee

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 6 | Tee to file (`-o`) | Write raw input to file while rendering | ✅ | ✅ | ✅ | ✅ Done |
| 7 | Tee append (`-a`) | Append mode for tee file | ✅ | ✅ | ✅ | ✅ Done |
| 8 | `--tee-format=raw\|display` | Raw or display-formatted (line numbers, timestamps, ANSI-stripped) output | ✅ | ✅ | ✅ | ✅ Done |

## Progress Bar

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 9 | Progress bar (auto-detect) | Show fill bar if stdin is a regular file with known size | ✅ | ✅ | ✅ | ✅ Done |
| 10 | Human-readable byte count | B/KB/MB/GB display in status line | ✅ | ✅ | ✅ | ✅ Done |
| 11 | Spinner + line count | Always shown in status line (fallback for pipes) | ✅ | ✅ | ✅ | ✅ Done |

## Display Options

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 12 | Line numbers (`-l`) | Right-aligned dim-colored numbers in gutter | ✅ | ✅ | ✅ | ✅ Done |
| 13 | ANSI color passthrough | `--color=auto\|always\|never` | ✅ | ✅ | ✅ | ✅ Done |
| 14 | Truncation indicator | `…` (default), configurable via `--truncation-char` | ✅ | ✅ | ✅ | ✅ Done |
| 15 | Wrap mode (`--wrap`) | Wrap long lines instead of truncating, SGR carryover | ✅ | ✅ | ✅ | ✅ Done |
| 16 | Timestamp prefix (`--timestamp`) | HH:MM:SS in dim gutter | ✅ | ✅ | ✅ | ✅ Done |

## Layouts

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 17 | box | Full box-drawing borders, title top, status bottom | ✅ | ✅ | ✅ | ✅ Done |
| 18 | rounded | Same as box, rounded corners | ✅ | ✅ | ✅ | ✅ Done |
| 19 | compact | Colored top bar + status, no side borders | ✅ | ✅ | ✅ | ✅ Done |
| 20 | minimal (default) | Just status line below content | ✅ | ✅ | ✅ | ✅ Done |
| 21 | none | No chrome at all | ✅ | ✅ | ✅ | ✅ Done |
| 22 | quote | Left bar (`▌`), optional `--quote-bg` 24-bit color | ✅ | ✅ | ✅ | ✅ Done |

## Passthrough / Pipe-Safe

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 23 | TTY detection | stderr not a tty → passthrough mode | ✅ | ✅ | ✅ | ✅ Done |
| 24 | Head+tail mode | First N + last N lines with `... [X omitted] ...` separator | ✅ | ✅ | ✅ | ✅ Done |
| 25 | Pipe-all (`-n 0`) | Pass everything through unchanged | ✅ | ✅ | ✅ | ✅ Done |
| 26 | `SLIT_FORCE_RENDER=1` | Force TUI mode even without TTY | ✅ | ✅ | ✅ | ✅ Done |
| 27 | stderr rendering | TUI renders to stderr, stdout free for piping | ✅ | ✅ | ✅ | ✅ Done |

## Rendering Engine

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 28 | ANSI CSI stripping | Remove `\x1b[...X` sequences | ✅ | ✅ | ✅ | ✅ Done |
| 29 | ANSI OSC stripping | Remove `\x1b]...BEL` sequences | ✅ | ✅ | ✅ | ✅ Done |
| 30 | Visible width calculation | Unicode + East Asian width aware | ✅ | ✅ | ✅ | ✅ Done |
| 31 | ANSI-aware truncation | TrimLineANSI preserves active colors | ✅ | ✅ | ✅ | ✅ Done |
| 32 | ANSI-aware wrapping | WrapLineANSI carries SGR state across breaks | ✅ | ✅ | ✅ | ✅ Done |
| 33 | UTF-8 decoding | Full 1-4 byte support | ✅ | ✅ | ✅ | ✅ Done |
| 34 | Dangling ANSI cleanup | Strip incomplete sequences at trim boundary | ✅ | ✅ | ✅ | ✅ Done |

## Performance

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 35 | Render debouncing | ~30fps cap, batch lines between renders | ✅ | ✅ | ✅ | ✅ Done |
| 36 | SIGWINCH handling | React to terminal resize | ✅ | ✅ | ✅ | ✅ Done |
| 37 | Ring buffer | 50k lines default, configurable `--max-lines` | ✅ | ✅ | ✅ | ✅ Done |
| 38 | Dirty flag optimization | Only re-render when data changed or signal fired | ✅ | ✅ | ✅ | ✅ Done |

## Signal Handling

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 39 | SIGINT / Ctrl+C | Graceful exit, restore terminal | ✅ | ✅ | ✅ | ✅ Done |
| 40 | SIGTERM / SIGHUP | Graceful exit (C only for SIGHUP) | ✅ | ✅ | ✅ | ✅ Done |
| 41 | SIGPIPE | Ignored to prevent crash on broken pipe | — | — | ✅ | ✅ Done |
| 42 | SIGTSTP (Ctrl+Z) | Suspend/resume with terminal state restore | ✅ | ✅ | ✅ | ✅ Done |

## Interaction

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 43 | Quit on `q`/`Ctrl+C` | Clean shutdown, close tee + debug | ✅ | ✅ | ✅ | ✅ Done |
| 44 | Keybinding bar | `q:quit` on RHS of status line | ✅ | ✅ | ✅ | ✅ Done |

## Infrastructure

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 45 | Window title (OSC) | Set via `\x1b]0;slit\x07`, restore on exit | ✅ | ✅ | ✅ | ✅ Done |
| 46 | Custom spinner | `--spinner=braille\|dots\|arrows\|off` | ✅ | ✅ | ✅ | ✅ Done |
| 47 | Config file | `$XDG_CONFIG_HOME/slit/config.toml`, sections: display, buffer, spinner, quote, debug | ✅ | ✅ | ✅ | ✅ Done |
| 48 | Man page + `--help` | `--generate-man` (hidden), thorough `--help` | ✅ | ✅ | ✅ | ✅ Done |
| 49 | Shell completions | `slit completion bash\|zsh\|fish` | ✅ | ✅ | ✅ | ✅ Done |
| 50 | Debug logging | `--debug` → timestamped log file, `--log-file` for custom path | ✅ | ✅ | ✅ | ✅ Done |
| 51 | Exit code | 0 on success, 1 on error | ✅ | ✅ | ✅ | ✅ Done |
| 52 | Version flag | `--version` → `0.2.4` | ✅ | ✅ | ✅ | ✅ Done |

## Testing

| # | Feature | Description | Go | Rust | C | Status |
|---|---------|-------------|:--:|:----:|:-:|:------:|
| 53 | Unit tests | Buffer, render, config, layout, spinner | ✅ | ✅ | ✅ | ✅ Done |
| 54 | Integration tests | Shared bats suite via `SLIT_BIN` env var | ✅ | ✅ | ✅ | ✅ Done |
| 55 | Benchmarks | Render, buffer, layout benchmarks | ✅ | — | — | ✅ Done |
| 56 | Valgrind target | `make valgrind` for leak checking | — | — | ✅ | ✅ Done |
| 57 | Maze/deep ANSI tests | Stress test with heavy ANSI coloring | ✅ | ✅ | — | ✅ Done |

---

## Deferred (v2)

| # | Feature | Status |
|---|---------|--------|
| 58 | Interactive scrollback after EOF | ⏳ Deferred |
| 59 | Search/filter (`/`) | ⏳ Deferred |
| 60 | Pause/resume (Space) | ⏳ Deferred |
| 61 | Highlight new lines | ⏳ Deferred |
| 62 | Progress via sidechannel | ⏳ Deferred |
| 63 | Exit code passthrough | ⏳ Deferred |

## To Be Considered

| # | Feature | Status |
|---|---------|--------|
| 64 | Configurable status line format | 💭 Considering |
| 65 | Multiple input sources / split panes | 💭 Considering |
| 66 | Follow mode for files (`tail -f` style) | 💭 Considering |
| 67 | Syntax highlighting | 💭 Considering |
| 68 | Regex highlighting (`--highlight PATTERN`) | 💭 Considering |

## V3 (maybe never)

| # | Feature | Status |
|---|---------|--------|
| 69 | Named pipe / multi-input | 🔮 V3 |
