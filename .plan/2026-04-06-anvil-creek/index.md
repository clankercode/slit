# slit — Implementation Plan

Date: 2026-04-06
Plan ID: anvil-creek
Status: **DRAFT — pending user confirmation**

## Overview

**slit** is a small streaming terminal viewer. It reads stdin into a fixed-height pane,
trims lines to terminal width, and re-renders on each new line.

```
long-process | slit -n 20
```

Three implementations: **Go**, **Rust**, **C** (in that order).

## Supporting Documents

| File | Contents |
|------|----------|
| `features.md` | Complete feature list with adoption status (v1/v2/v3) |
| `architecture.md` | Per-language stack, rendering approach, data flow |
| `layouts.md` | Layout specifications (box, rounded, compact, minimal, none, quote) |
| `cli.md` | CLI flags, completions, config file spec |
| `testing.md` | Testing strategy (per-language unit + shared bats integration) |
| `research.md` | Key research findings from planning phase |

## Design Decisions Summary

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Rendering target | stderr | Leaves stdout free for pipeline passthrough |
| Rendering approach | Raw ANSI (C), lib-based (Go: Bubbletea, Rust: ratatui) | Leverage ecosystem per language |
| Pipe detection | If stderr not a tty → passthrough mode (no rendering) | Like `less` and `bat` |
| Line buffer | Ring buffer, 50k lines default, configurable | Bounded memory |
| ANSI color | `--color=auto\|always\|never` (like bat) | Flexible, configurable |
| Truncation indicator | `…` (configurable) | Unicode, visually clean |
| Default layout | minimal | Maximum content, still has status |
| Line wrapping | Default: truncate. `--wrap` to wrap instead | Truncate is safer for streaming |
| Progress | Spinner + line count + auto-detect file size for fill bar | Good coverage without sidechannels |
| Post-EOF | Show done summary, exit immediately | Scrollback deferred to v2 |
| Exit code | 0 on success, 1 on error | Simple |
| Debug logging | `--debug` flag → log file (`/tmp/slit-$PID.log` by default) | stderr is used for rendering |
| Config file | `~/.config/slit/config.toml` | Persistent defaults |
| Completions | `slit completion bash\|zsh\|fish` + static files in repo | Best practice, like ripgrep/bat |
| Keybinding bar | RHS of status line | Saves vertical space |

## Repo Structure

```
slit/
├── README.md
├── IDEAS.md
├── justfile
├── completions/
│   ├── slit.bash
│   ├── _slit
│   └── slit.fish
├── go/
│   ├── go.mod
│   ├── main.go
│   ├── model.go
│   ├── config.go
│   ├── layouts.go
│   ├── render.go
│   └── ...
├── rust/
│   ├── Cargo.toml
│   ├── src/
│   │   ├── main.rs
│   │   ├── app.rs
│   │   ├── config.rs
│   │   ├── layout.rs
│   │   └── ...
│   └── ...
├── c/
│   ├── Makefile
│   ├── main.c
│   ├── config.c
│   ├── layout.c
│   ├── render.c
│   └── ...
├── tests/
│   └── integration/
│       ├── test.bats
│       ├── helpers.bash
│       └── ...
└── .plan/
    └── 2026-04-06-anvil-creek/
        ├── index.md
        ├── features.md
        ├── architecture.md
        ├── layouts.md
        ├── cli.md
        ├── testing.md
        └── research.md
```

## Implementation Order

### Phase 1: Go (reference implementation)

1. CLI flag parsing (cobra)
2. Config file loading (BurntSushi/toml)
3. Core stdin → ring buffer → Bubbletea rendering pipeline
4. Layout system (all 6 layouts)
5. Status line (spinner, line count, progress bar, keybinding hints)
6. Line numbers, timestamps, ANSI passthrough, truncation indicator
7. Wrap mode
8. Tee to file
9. SIGWINCH, render debouncing, pipe detection
10. Window title
11. Debug logging
12. Shell completions
13. --help and man page

### Phase 2: Rust (reference Go behavior)

Mirror Go implementation using ratatui + crossterm + tokio + clap + serde.

### Phase 3: C (reference Go+Rust behavior)

Raw termios + poll() + hand-rolled ANSI rendering. No TUI library.

### Phase 4: Shared integration tests (bats)

Write tests that validate all three binaries produce identical behavior.

## Chrome / Layout Model

All layouts render content in the same N-line area. Chrome is **extra** (outside N):

| Layout | Top chrome | Bottom chrome | Side cost | Total extra lines |
|--------|-----------|---------------|-----------|-------------------|
| box | 1 (border + title) | 1 (border + status) | ~4 chars (2 border + 2 padding) | 2 |
| rounded | 1 (border + title) | 1 (border + status) | ~4 chars (2 border + 2 padding) | 2 |
| compact | 1 (colored bar) | 1 (status) | 0 | 2 |
| minimal | 0 | 1 (status) | 0 | 1 |
| quote | 0 | 0 (status inside bar) | ~2 chars | 0 |
| none | 0 | 0 | 0 | 0 |

## Status Line (1 line, shared across layouts)

```
⠋ Streaming... (142 lines) [====      ]  q:quit
```

- LHS: spinner + label + line count + optional progress bar
- RHS: keybinding hints
- After EOF: `Done. (142 lines) [===================] q:quit`
