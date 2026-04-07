# Cross-Implementation Consistency Notes

Comparing the Go, Rust, and C implementations of slit.

---

## CLI Flags

All three implementations support the same set of CLI flags:

| Flag | Go | Rust | C | Match |
|------|:--:|:----:|:-:|:-----:|
| `-n` / `--lines` | yes | yes | yes | yes |
| `--max-lines` | yes | yes | yes | yes |
| `-o` / `--output` | yes | yes | yes | yes |
| `-a` / `--append` | yes | yes | yes | yes |
| `--tee-format` | yes | yes | yes | yes |
| `-l` / `--line-numbers` | yes | yes | yes | yes |
| `--color` | yes | yes | yes | yes |
| `-w` / `--wrap` | yes | yes | yes | yes |
| `-t` / `--timestamp` | yes | yes | yes | yes |
| `--truncation-char` | yes | yes | yes | yes |
| `--layout` | yes | yes | yes | yes |
| `--box` | yes | yes | yes | yes |
| `--rounded` | yes | yes | yes | yes |
| `--compact` | yes | yes | yes | yes |
| `--minimal` | yes | yes | yes | yes |
| `--none` | yes | yes | yes | yes |
| `--quote` | yes | yes | yes | yes |
| `--quote-bg` | yes | yes | yes | yes |
| `--spinner` | yes | yes | yes | yes |
| `-d` / `--debug` | yes | yes | yes | yes |
| `--log-file` | yes | yes | yes | yes |
| `--version` | yes | yes | yes | yes |
| `--help` | yes | yes | yes | yes |
| `completion` subcommand | yes | yes | yes | yes |
| `--generate-man` (hidden) | yes | yes | yes | yes |

**Note:** All three implementations support `--generate-man` as a hidden documentation/export flag.

---

## Default Values

| Setting | Go | Rust | C | Match |
|---------|:--:|:----:|:-:|:-----:|
| `lines` | 0 (auto) | 0 (auto) | 0 (auto) | yes |
| `max_lines` | 50000 | 50000 | 50000 | yes |
| `layout` | minimal | minimal | minimal | yes |
| `color` | auto | auto | auto | yes |
| `wrap` | false | false | false | yes |
| `line_numbers` | false | false | false | yes |
| `timestamp` | false | false | false | yes |
| `truncation_char` | `…` | `…` | `…` | yes |
| `spinner` | braille | braille | braille | yes |
| `tee_format` | raw | raw | raw | yes |
| `append` | false | false | false | yes |
| `quote_bg` | off | off | off | yes |
| `debug` | false | false | false | yes |
| `log_file` | (auto) | (auto) | (auto) | yes |
| `RENDER_INTERVAL_MS` | ~33 (100ms spinner tick) | 33ms tick interval | 33ms poll | yes |
| `version` | 0.2.4 | 0.2.4 | 0.2.4 | yes |

All defaults are consistent across implementations.

---

## Layouts

All six layouts are implemented in all three:

| Layout | Go | Rust | C |
|--------|:--:|:----:|:-:|
| minimal | yes | yes | yes |
| box | yes | yes | yes |
| rounded | yes | yes | yes |
| compact | yes | yes | yes |
| none | yes | yes | yes |
| quote | yes | yes | yes |

---

## Key Behavioral Differences

### Rendering framework

| Impl | Approach |
|------|----------|
| Go | Bubbletea ( Elm-like TUI framework), renders to stderr |
| Rust | ratatui + crossterm, inline viewport, renders to stderr |
| C | raw ANSI escape codes via `fprintf(stderr, ...)`, manual cursor movement |

### Passthrough detection

All three check if stderr is a TTY. When it's not, they enter passthrough mode: stdin is copied to stdout. By default, passthrough shows the first N and last N lines (like `head` + `tail`) with a separator for omitted lines. Use `-n 0` to pipe everything through unchanged. All respect `SLIT_FORCE_RENDER=1`.

| Impl | TTY check method |
|------|-------------------|
| Go | `term.IsTerminal(int(os.Stderr.Fd()))` |
| Rust | `libc::isatty(libc::STDERR_FILENO)` |
| C | `isatty(STDERR_FILENO)` |

### Config file loading

All three read from `$XDG_CONFIG_HOME/slit/config.toml` or `~/.config/slit/config.toml`. CLI flags override file config in all implementations.

| Impl | TOML library |
|------|-------------|
| Go | `github.com/BurntSushi/toml` |
| Rust | `toml` crate (serde) |
| C | Built-in minimal TOML parser (`configfile.c`) |

### Window title

All three set the terminal window title to `slit` via OSC escape sequences when the layout is not `none`.

### Quit handling

| Impl | Keys |
|------|------|
| Go | `q`, `Ctrl+C` |
| Rust | `q`, `Ctrl+C` |
| C | `q`, `Q`, `Ctrl+C` (via SIGINT) |

**Note:** C also accepts uppercase `Q` due to raw terminal read. Go and Rust frameworks normalize key input.

### SIGTSTP (Ctrl+Z)

| Impl | Behavior |
|------|----------|
| Go | Bubbletea framework handles (restores terminal, suspends, resumes) |
| Rust | Explicitly handled: restores terminal state via `SignalKind::from_raw(libc::SIGTSTP)`, raises SIGSTOP, re-enters raw mode on resume |
| C | Explicitly handled: restores terminal state, raises SIGSTOP, re-enters raw mode on resume |

### Status line

All three show spinner, line count, human-readable byte count (e.g., `1.5KB`), progress bar (when file size is known), and `q:quit` hint. The byte count display was added to Go and Rust to match the original C implementation.

### Quote background (`--quote-bg`)

All three accept `--quote-bg` with a hex color (e.g., `#1a1a2e`) or `off`. When a hex color is set, the quote layout applies a 24-bit ANSI background color to all lines, padded to full terminal width.

### EOF behavior

All three show a final "Done" status and wait briefly before exiting (~200ms).

---

## Intentional Differences

1. **C accepts `Q` (uppercase)** — raw terminal reads both cases; Go/Rust frameworks normalize
2. **Go uses Bubbletea's 100ms spinner tick** vs C/Rust using 33ms intervals — visual result is equivalent since spinner increments are timed

---

## Shell Completions

All three can generate bash, zsh, and fish completion scripts:

| Impl | Method |
|------|--------|
| Go | `cobra.GenBashCompletion`, `cobra.GenZshCompletion`, `cobra.GenFishCompletion` |
| Rust | `clap_complete::generate` with `Bash`, `Zsh`, `Fish` shells |
| C | Custom static completion strings in `completion.c` |

The completion content is functionally identical (same flags, same subcommands).

---

## Testing

| Impl | Unit tests | Integration tests |
|------|:----------:|:-----------------:|
| Go | yes (`go test`) | yes (bats) |
| Rust | yes (`cargo test`) | yes (bats, same suite) |
| C | yes (custom test files, `make test`) | yes (bats, same suite) |

Integration tests in `tests/integration/` are shared across all implementations via the `SLIT_BIN` environment variable.
