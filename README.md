# slit

A small streaming terminal viewer. Reads stdin into a fixed-height pane,
trims lines to terminal width, and re-renders on each new line.

    long-process | slit -n 20

Three implementations: Go, Rust, C.

## Features

- Fixed-height scrolling pane (default: terminal height - 1)
- Auto-trims lines to terminal width
- Spinner status line while streaming
- Done summary on EOF
- Progress bar (auto-detects file size for fill bar)
- 6 layout presets: box, rounded, compact, minimal (default), none, quote
- Tee output to file (`-o`, `-a`)
- Line numbers (`-l`)
- ANSI color passthrough (`--color=auto|always|never`)
- Wrap mode (`--wrap`)
- Timestamp prefix (`--timestamp`)
- Truncation indicator (default `…`, configurable)
- Configurable spinner styles (braille, dots, arrows, off)
- Config file (`~/.config/slit/config.toml`)
- Shell completions (bash, zsh, fish)
- Debug logging (`--debug`)
- Window title via OSC escape sequences
- SIGWINCH handling (terminal resize)
- Render debouncing (~30fps)

## Usage

    some-command | slit [-n lines] [-o output.log] [--layout LAYOUT] [flags]

## Layouts

| Layout | Description |
|--------|-------------|
| minimal | Status line only (default) |
| box | Full borders with title + status in border |
| rounded | Same as box with rounded corners |
| compact | Colored top bar + status |
| quote | Left bar, optional bg tint |
| none | No chrome, pure streaming |

## Building

    just build          # build all three
    just build-go       # go only
    just build-rust     # rust only
    just build-c        # c only

## Testing

    just test-all       # unit + integration tests for all implementations

## Shell Completions

    slit completion bash > ~/.local/share/bash-completion/completions/slit
    slit completion zsh > ~/.zfunc/_slit
    slit completion fish > ~/.config/fish/completions/slit.fish

## Plan

See `.plan/2026-04-06-anvil-creek/` for the full implementation plan.
<!-- nts: original plan was to use with autocommit, don't forget -->
