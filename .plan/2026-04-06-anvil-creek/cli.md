# CLI Interface Specification

## Usage

```
some-command | slit [flags]
```

## Flags

### Core

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--lines` | `-n` | int | terminal height - 1 | Number of content lines to display |
| `--max-lines` | | int | 50000 | Ring buffer capacity (max lines stored) |
| `--output` | `-o` | path | | Tee file path (write input to file) |
| `--append` | `-a` | bool | false | Append to tee file instead of overwrite |
| `--tee-format` | | enum | raw | Tee output format: `raw` or `display` |

### Display

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--line-numbers` | `-l` | bool | false | Show line numbers |
| `--color` | | enum | auto | ANSI color handling: `auto`, `always`, `never` |
| `--wrap` | `-w` | bool | false | Wrap long lines instead of truncating |
| `--timestamp` | `-t` | bool | false | Prepend HH:MM:SS timestamp to each line |
| `--truncation-char` | | string | `…` | Character to show when line is trimmed |

### Layout

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--layout` | | enum | minimal | Layout: `box`, `rounded`, `compact`, `minimal`, `none`, `quote` |
| `--box` | | bool | false | Shortcut for `--layout=box` |
| `--rounded` | | bool | false | Shortcut for `--layout=rounded` |
| `--compact` | | bool | false | Shortcut for `--layout=compact` |
| `--minimal` | | bool | false | Shortcut for `--layout=minimal` |
| `--none` | | bool | false | Shortcut for `--layout=none` |
| `--quote` | | bool | false | Shortcut for `--layout=quote` |
| `--quote-bg` | | string | off | Quote layout background color (e.g. `#1a1a2e`, `off`) |

### Progress

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--spinner` | | enum | braille | Spinner style: `braille`, `dots`, `arrows`, `off` |

### Debug

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--debug` | `-d` | bool | false | Enable debug logging |
| `--log-file` | | path | `/tmp/slit-$PID.log` | Debug log file path |

### Meta

| Flag | Short | Type | Default | Description |
|------|-------|------|---------|-------------|
| `--help` | `-h` | | | Show help |
| `--version` | `-v` | | | Show version |
| `--generate-man` | | | | Generate man page to stdout |
| `completion` | | subcmd | | Generate shell completions |

## Shell Completions

```
slit completion bash
slit completion zsh
slit completion fish
```

### Completion Values

| Flag | Completes |
|------|-----------|
| `--color` | `auto`, `always`, `never` |
| `--spinner` | `braille`, `dots`, `arrows`, `off` |
| `--layout` | `box`, `rounded`, `compact`, `minimal`, `none`, `quote` |
| `--tee-format` | `raw`, `display` |
| `-o`, `--output` | file paths |
| `--log-file` | file paths |
| `-n`, `--lines` | (no completion, integer) |
| `--max-lines` | (no completion, integer) |

### Static Files

Pre-generated completion scripts shipped in `completions/`:
- `completions/slit.bash`
- `completions/_slit` (zsh)
- `completions/slit.fish`

## Config File

Path: `~/.config/slit/config.toml`

Priority: CLI flag > config file > built-in default

```toml
[display]
layout = "minimal"
lines = 0                           # 0 = auto (terminal height - 1)
line_numbers = false
color = "auto"
wrap = false
timestamp = false
truncation_char = "…"

[buffer]
max_lines = 50000

[spinner]
style = "braille"

[progress]
# Future: progress bar config

[quote]
bg = "off"

[debug]
enabled = false
log_file = "/tmp/slit-$PID.log"
```

## --help Output (draft)

```
slit — streaming terminal viewer

USAGE
    some-command | slit [flags]

FLAGS
    -n, --lines <N>           Number of content lines (default: terminal height - 1)
    -o, --output <FILE>       Tee input to file
    -a, --append              Append to tee file
        --tee-format <FMT>    Tee format: raw (default) or display
    -l, --line-numbers        Show line numbers
        --color <WHEN>        Color: auto (default), always, never
    -w, --wrap                Wrap long lines instead of truncating
    -t, --timestamp           Prepend timestamp to each line
        --truncation-char <C> Truncation indicator (default: …)
        --layout <LAYOUT>     Layout: box, rounded, compact, minimal (default), none, quote
        --box                 Shortcut for --layout=box
        --rounded             Shortcut for --layout=rounded
        --compact             Shortcut for --layout=compact
        --minimal             Shortcut for --layout=minimal
        --none                Shortcut for --layout=none
        --quote               Shortcut for --layout=quote
        --quote-bg <COLOR>    Quote layout bg color (default: off)
        --spinner <STYLE>     Spinner: braille (default), dots, arrows, off
        --max-lines <N>       Ring buffer capacity (default: 50000)
    -d, --debug               Enable debug logging
        --log-file <PATH>     Debug log file (default: /tmp/slit-$PID.log)
    -h, --help                Show this help
    -v, --version             Show version

COMMANDS
    completion <shell>        Generate shell completions (bash, zsh, fish)

EXAMPLES
    tail -f /var/log/syslog | slit
    dmesg | slit -n 20 --line-numbers
    build-command 2>&1 | slit --compact -o build.log
```
