# USAGE.md

Complete usage guide for **slit** — a streaming terminal viewer.

---

## Quick Start

```sh
# Basic: stream output into a fixed-height pane
make build 2>&1 | slit

# With line numbers and timestamps
tail -f /var/log/syslog | slit -l -t

# Box layout, save to file
pytest tests/ | slit --box -o results.log
```

---

## Common Use Cases

### Tail -f replacement

```sh
tail -f /var/log/nginx/access.log | slit -l -t --layout compact
```

### Build output viewer

```sh
make build 2>&1 | slit --box -o build.log
```

### Test runner viewer

```sh
go test -v ./... | slit --compact -l
```

### Log viewer with timestamps

```sh
cat production.log | slit -l -t -n 20
```

### Colorized command output

```sh
ls --color=always | slit --color=always
```

### Watch a data stream

```sh
./scripts/slit-test-data | slit --rounded -l
```

---

## All CLI Flags

### Display

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--lines` | `-n` | `0` (auto) | Pane height. `0` = terminal height minus layout chrome. In passthrough mode: default shows first+last 10 lines; `0` = pipe all |
| `--layout` | | `minimal` | Layout style: `minimal`, `box`, `rounded`, `compact`, `none`, `quote` |
| `--wrap` | `-w` | `false` | Wrap long lines instead of truncating |
| `--line-numbers` | `-l` | `false` | Show line numbers in gutter |
| `--timestamp` | `-t` | `false` | Prepend timestamp (HH:MM:SS) to each line |
| `--truncation-char` | | `…` | Character shown when a line is trimmed |

### Color & Style

| Flag | Default | Description |
|------|---------|-------------|
| `--color` | `auto` | ANSI passthrough: `auto`, `always`, `never` |
| `--spinner` | `braille` | Spinner style: `braille`, `dots`, `arrows`, `off` |

### Layout Shortcuts

| Flag | Equivalent |
|------|------------|
| `--box` | `--layout=box` |
| `--rounded` | `--layout=rounded` |
| `--compact` | `--layout=compact` |
| `--minimal` | `--layout=minimal` |
| `--none` | `--layout=none` |
| `--quote` | `--layout=quote` |

### Output

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--output` | `-o` | | Tee stdin to file |
| `--append` | `-a` | `false` | Append to tee file instead of overwriting |
| `--tee-format` | | `raw` | Tee output format: `raw` (original input) or `display` (as rendered) |

### Buffer

| Flag | Default | Description |
|------|---------|-------------|
| `--max-lines` | `50000` | Maximum lines kept in memory |

### Quote Layout

| Flag | Default | Description |
|------|---------|-------------|
| `--quote-bg` | `off` | Quote background: `off` or `on` |

### Debug

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--debug` | `-d` | `false` | Enable debug logging |
| `--log-file` | | `/tmp/slit-$PID.log` | Custom debug log file path |

### Meta

| Flag | Description |
|------|-------------|
| `--help` | Show help |
| `--version` | Show version |

### Subcommands

| Command | Description |
|---------|-------------|
| `slit completion bash` | Generate bash completions |
| `slit completion zsh` | Generate zsh completions |
| `slit completion fish` | Generate fish completions |

---

## Layouts

### minimal (default)

Status line only. No borders. Maximum content area.

```
line 1...
line 2...
line N...
⠋ Streaming... (142)                   q:quit
```

### box

Full box-drawing borders with title header.

```
┌─ slit ─────────────────────────────────────┐
│ line 1...                                  │
│ line 2...                                  │
│ line N...                                  │
└─ ⠋ Streaming... (142) q:quit ─────────────┘
```

### rounded

Same as box with rounded corners.

```
╭─ slit ─────────────────────────────────────╮
│ line 1...                                  │
│ line 2...                                  │
│ line N...                                  │
╰─ ⠋ Streaming... (142) q:quit ─────────────╯
```

### compact

Colored top bar + status line. No side borders.

```
█ slit
  line 1...
  line 2...
  line N...
⠋ Streaming... (142)                   q:quit
```

### quote

Left vertical bar on every line. No top/bottom borders.

```
▌ slit
▌ line 1...
▌ line 2...
▌ line N...
▌ ⠋ Streaming... (142)            q:quit
```

### none

No chrome at all. Just content. No status line, no borders.

---

## Config File

slit reads `~/.config/slit/config.toml` (or `$XDG_CONFIG_HOME/slit/config.toml`) on startup. CLI flags take precedence.

### Example config

```toml
[display]
layout = "compact"
lines = 20
line_numbers = true
color = "auto"
wrap = false
timestamp = false
truncation_char = "…"

[buffer]
max_lines = 50000

[spinner]
style = "braille"

[quote]
bg = "off"

[debug]
enabled = false
log_file = "/tmp/slit-debug.log"
```

### Config sections

**`[display]`**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `layout` | string | `"minimal"` | Default layout |
| `lines` | int | `0` | Default pane height (0 = auto) |
| `line_numbers` | bool | `false` | Show line numbers |
| `color` | string | `"auto"` | Color mode |
| `wrap` | bool | `false` | Wrap long lines |
| `timestamp` | bool | `false` | Show timestamps |
| `truncation_char` | string | `"…"` | Truncation indicator |

**`[buffer]`**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `max_lines` | int | `50000` | Max lines in ring buffer |

**`[spinner]`**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `style` | string | `"braille"` | Spinner style |

**`[quote]`**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `bg` | string | `"off"` | Quote background mode |

**`[debug]`**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `false` | Enable debug logging |
| `log_file` | string | | Custom log file path |

---

## Environment Variables

| Variable | Description |
|----------|-------------|
| `SLIT_FORCE_RENDER=1` | Force TUI rendering even when stderr is not a TTY |
| `XDG_CONFIG_HOME` | Override config directory (default: `~/.config`) |

---

## Shell Completions

### bash

```sh
slit completion bash > ~/.local/share/bash-completion/completions/slit
```

Add to `~/.bashrc`:

```sh
source ~/.local/share/bash-completion/completions/slit
```

### zsh

```sh
slit completion zsh > ~/.zfunc/_slit
```

Ensure `~/.zfunc` is in your fpath in `~/.zshrc`:

```sh
fpath=(~/.zfunc $fpath)
autoload -Uz compinit && compinit
```

### fish

```sh
slit completion fish > ~/.config/fish/completions/slit.fish
```

Fish auto-loads completions from this directory.

---

## Build Instructions

### Go (recommended)

```sh
cd go
go build -o slit .
```

Install:

```sh
cp go/slit ~/.local/bin/slit
```

Or using just:

```sh
just build-go
just install-go
```

### Rust

```sh
cd rust
cargo build --release
```

The binary is at `rust/target/release/slit`.

Install:

```sh
cp rust/target/release/slit ~/.local/bin/slit
```

### C

```sh
cd c
make
```

The binary is at `c/slit`.

Install:

```sh
cd c && make install PREFIX=~/.local
```

Run tests:

```sh
cd c && make test
```

---

## Tips and Tricks

### Pipe to slit and another command

When stderr isn't a TTY, slit shows the first 10 and last 10 lines of the stream. Use `-n N` to change the window, or `-n 0` to pipe everything through unchanged. This makes it pipe-safe:

```sh
make build 2>&1 | slit | grep ERROR
```

### Force rendering in a pipe

If you want slit's TUI even when stderr isn't a TTY:

```sh
SLIT_FORCE_RENDER=1 some-command | slit
```

### Watch test data with effects

The `scripts/slit-test-data` script generates colorful test output:

```sh
# Cycle through all effects
./scripts/slit-test-data | slit --rounded -l

# Single effect (runs forever)
./scripts/slit-test-data matrix | slit --box
./scripts/slit-test-data gradient | slit --compact -w
./scripts/slit-test-data maze | slit --rounded
./scripts/slit-test-data aurora | slit
./scripts/slit-test-data spectrum | slit --compact
./scripts/slit-test-data cyberdeck | slit --box -o cyber.log
```

Available effects: `rain`, `matrix`, `gradient`, `waterfall`, `aurora`, `cherry-blossom`, `sierpinski`, `spectrum`, `cyberdeck`, `ember-snow`, `crystal`, `maze`

### Save output while viewing

```sh
# Raw input (what the command outputs)
long-job | slit -o job-output.log

# Display format (with line numbers, timestamps, etc. as configured)
long-job | slit -l -t --tee-format=display -o formatted.log
```

### Use as a persistent log monitor

```sh
# Minimal, just status line
tail -f /var/log/app.log | slit -n 5 -t
```

### Debug rendering issues

```sh
cat data.txt | slit -d --log-file /tmp/slit-debug.log
```

### Keybindings during runtime

| Key | Action |
|-----|--------|
| `q` | Quit |
| `Ctrl+C` | Quit |

---

## Progress Bar

slit automatically shows a progress bar when it can detect the total input size (e.g., when reading from a regular file). For pipes, it shows a spinner with line count instead.

```sh
# Progress bar (file size known)
cat large-file.log | slit

# Spinner + count (pipe, unknown size)
make build 2>&1 | slit
```
