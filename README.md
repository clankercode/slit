# slit

A streaming terminal viewer — pipe stdout into a fixed-height pane that re-renders live.

```sh
long-running-process | slit
```

---

## Why

You're running something that spews output and you want to **see it happen** without your terminal scrolling into oblivion. `slit` gives you a fixed viewport — like a mini terminal-within-a-terminal — that shows the latest N lines and nothing else.

Unlike `tail -f` (file only) or `less` (pager, not live), `slit` is purpose-built for streaming stdin in place.

## Features

- **Live rendering** at up to ~100fps with render debouncing
- **6 layouts**: minimal, box, rounded, compact, quote, none
- **Line numbers**, **timestamps**, **wrap mode**
- **ANSI color passthrough** (`--color=auto|always|never`)
- **Progress bar** auto-detected from file size; spinner + count for pipes
- **Tee to file** while viewing (`-o file`, `--tee-format=raw|display`)
- **Pipe-safe** — renders to stderr; passthrough when stderr isn't a TTY
- **Terminal resize** via SIGWINCH
- **Config file** (`~/.config/slit/config.toml`)
- **Shell completions** for bash, zsh, fish
- **3 implementations**: Go (Bubbletea), Rust (ratatui), C (raw ANSI)

## Quick Examples

```sh
# Default: fills terminal height minus 1
make build 2>&1 | slit

# Fixed 10-line pane with line numbers and timestamps
tail -f /var/log/syslog | slit -n 10 -l -t

# Box layout, save output simultaneously
pytest tests/ | slit --box -o results.log

# Compact layout with wrap
./scripts/slit-test-data gradient | slit --compact -w

# Passthrough for piping (no layout when stderr isn't a tty)
make build 2>&1 | slit | grep ERROR
```

## Layouts

| Layout | Look | Extra lines | Side cost |
|--------|------|:-----------:|:---------:|
| **minimal** *(default)* | Status line only | +1 | 0 |
| **box** | Full box-drawing borders | +2 | ~4 chars |
| **rounded** | Same as box, round corners | +2 | ~4 chars |
| **compact** | Colored top bar + status | +2 | 0 |
| **quote** | Left bar, no top/bottom | 0 | ~2 chars |
| **none** | No chrome at all | 0 | 0 |

<details>
<summary>Layout previews</summary>

**minimal** *(default)*
```
line 1...
line 2...
line N...
⠋ Streaming... (142)                   q:quit
```

**box**
```
┌─ slit ─────────────────────────────────────┐
│ line 1...                                  │
│ line 2...                                  │
│ line N...                                  │
└─ ⠋ Streaming... (142) q:quit ─────────────┘
```

**rounded**
```
╭─ slit ─────────────────────────────────────╮
│ line 1...                                  │
│ line 2...                                  │
│ line N...                                  │
╰─ ⠋ Streaming... (142) q:quit ─────────────╯
```

**compact**
```
█ slit
  line 1...
  line 2...
  line N...
⠋ Streaming... (142)                   q:quit
```

**quote**
```
▌ slit
▌ line 1...
▌ line 2...
▌ line N...
▌ ⠋ Streaming... (142)            q:quit
```

</details>

## Flags

```
Usage: slit [flags]

Display
  -n, --lines int          Pane height (default: terminal height - 1)
       --layout string      Layout: minimal|box|rounded|compact|none|quote (default: minimal)
      Shorthands: --box, --rounded, --compact
      --wrap               Wrap long lines instead of truncating
  -l, --line-numbers       Show line numbers
  -t, --timestamp          Prepend timestamp to each line
      --truncation-char    Character for truncation indicator (default: "…")

Color & Style
      --color string       ANSI passthrough: auto|always|never (default: auto)
      --spinner string     Spinner style: braille|dots|arrows|off (default: braille)

Output
  -o, --output file        Tee stdin to file
  -a, --append             Append to tee file instead of overwriting
      --tee-format string  Tee format: raw|display (default: raw)

Buffer
      --max-lines int      Maximum lines to buffer (default: 50000)

Meta
  -d, --debug              Write diagnostics to log file
      --log-file string    Custom debug log file path
      --version            Show version info
```

> See [USAGE.md](USAGE.md) for comprehensive documentation including config file format, environment variables, and tips.

## Building

All three implementations are available. The C build is the default.

### C (raw ANSI)

```sh
cd c && make
# or
just build-c
```

### Go (Bubbletea)

```sh
cd go && go build -o slit .
# or
just build-go
```

### Rust (ratatui)

```sh
cd rust && cargo build --release
```

## Installing

```sh
# Homebrew (Linux)
brew install clankercode/slit/slit

# One-liner installer (C binary, Linux/macOS)
curl -sL https://clankercode.github.io/slit/install.sh | sh
```

### From source

```sh
# C
just install-c

# Go
just install-go

# Rust
cp rust/target/release/slit ~/.local/bin/slit

# C (manual)
cd c && make install PREFIX=~/.local
```

If `man slit` still does not resolve after installing to `~/.local`, add `~/.local/share/man` to `MANPATH`.

## Testing

```sh
# Go unit tests
just test-go

# Integration tests (requires bats-core)
just test-integration

# C unit tests
cd c && make test

# Rust tests
cd rust && cargo test
```

## Shell Completions

```sh
slit completion bash  > ~/.local/share/bash-completion/completions/slit
slit completion zsh   > ~/.zfunc/_slit
slit completion fish  > ~/.config/fish/completions/slit.fish
```

## Man Pages

```sh
just man-c
just man-go
just man-rust
```

## Documentation

- [USAGE.md](USAGE.md) — comprehensive usage guide, config file format, all flags
- [IDEAS.md](IDEAS.md) — feature status and roadmap

## License

CC0-1.0
