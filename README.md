# slit

A streaming terminal viewer — pipe stdout into a fixed-height pane that re-renders live.

```sh
long-running-process | slit
```

> [!NOTE]
> **Status: pre-release.** Implementation is planned (see [`.plan/2026-04-06-anvil-creek/`](.plan/2026-04-06-anvil-creek/index.md)). Go implementation is in progress; Rust and C to follow.

---

## Why

You're running something that spews output and you want to **see it happen** without your terminal scrolling into oblivion. `slit` gives you a fixed viewport — like a mini terminal-within-a-terminal — that shows the latest N lines and nothing else.

Unlike `tail -f` (file only) or `less` (pager, not live), `slit` is purpose-built for streaming stdin in place.

## Quick Examples

```sh
# Default: fills terminal height minus 1
make build 2>&1 | slit

# Fixed 10-line pane with line numbers and timestamps
tail -f /var/log/syslog | slit -n 10 -l --timestamp

# Box layout, save output simultaneously
pytest tests/ | slit --box -o results.log

# Show progress bar (auto-detects file size)
cat large-file.ndjson | slit --layout compact

# Passthrough for piping (no layout when stderr isn't a tty)
make build 2>&1 | slit | grep ERROR
```

## Layouts

Pick a layout to trade screen real estate for visual structure.

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
      --wrap               Wrap long lines instead of truncating
  -l, --line-numbers       Show line numbers
  -t, --timestamp          Prepend timestamp to each line
      --truncation string  Truncation indicator (default: "…")

Color & Style
      --color string       ANSI passthrough: auto|always|never (default: auto)
      --spinner string     Spinner style: braille|dots|arrows|off (default: braille)

Output
  -o, --output file        Tee stdin to file
  -a, --append             Append to tee file instead of overwriting

Meta
      --debug              Write diagnostics to /tmp/slit-$PID.log
      --config string      Config file path (default: ~/.config/slit/config.toml)
      --completion string  Generate shell completions: bash|zsh|fish
```

> [!TIP]
> All flags can be set persistently in `~/.config/slit/config.toml`. CLI flags take precedence.

## Key Features

- **Live rendering** at ~30fps with render debouncing
- **Terminal resize** handled via SIGWINCH
- **Progress bar** auto-detected from file size, spinner + count for pipes
- **ANSI color passthrough** (`--color=auto|always|never`, like `bat`)
- **Pipe-safe** — renders to stderr; when stderr isn't a TTY, falls back to passthrough
- **Window title** set via OSC escape sequences

## Building

Three implementations planned — Go (Bubbletea), Rust (ratatui), C (raw ANSI). Only Go is started.

```sh
just build          # all three
just build-go       # go only
```

> [!NOTE]
> Requires [just](https://github.com/casey/just).

## Testing

```sh
just test-all       # unit + integration across all implementations
```

Integration tests use [bats-core](https://github.com/bats-core/bats-core) and validate that all three binaries behave identically.

## Shell Completions

```sh
slit completion bash  > ~/.local/share/bash-completion/completions/slit
slit completion zsh   > ~/.zfunc/_slit
slit completion fish  > ~/.config/fish/completions/slit.fish
```

## License

<!-- Add your license here -->
