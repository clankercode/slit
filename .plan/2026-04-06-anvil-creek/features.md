# Features — Adoption Status

## v1 (initial implementation)

### Core behavior

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Fixed-height scrolling pane | Rolling buffer of last N lines, in-place re-render | **adopted** | Ring buffer, 50k lines default |
| Auto-trim to terminal width | Detect width + SIGWINCH, trim lines to fit | **adopted** | No wrap by default |
| `-n` line count flag | `slit -n 20` allocates 20 lines of content | **adopted** | Default: terminal height - 1 |
| Spinner status line | Braille spinner + label + line count | **adopted** | Combined with keybinding hints on RHS |
| Done summary on EOF | Replace spinner with `Done. (142 lines)` | **adopted** | Exit immediately after display |

### Output / tee

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Tee to file (`-o`) | Write raw input to file while rendering | **adopted** | Raw by default |
| Tee append (`-a`) | Append mode for tee file | **adopted** | |
| `--tee-format=raw\|display` | Choose raw or display-formatted output for tee | **adopted** | Default: raw |

### Progress bar

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Progress bar (auto-detect) | Show fill bar if stdin is a regular file with known size | **adopted** | Falls back to spinner+count for pipes |
| Spinner + line count | Always show spinner and line count in status line | **adopted** | Part of the status line |

### Display options

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Line numbers (`-l`) | Dim-colored right-aligned number | **adopted** | No separator pipe |
| ANSI color passthrough | `--color=auto\|always\|never` | **adopted** | Like bat |
| Truncation indicator | `…` at end of trimmed lines | **adopted** | Default `…`, configurable |
| Wrap mode (`--wrap`) | Wrap long lines instead of truncating | **adopted** | Default: truncate |
| Timestamp prefix (`--timestamp`) | Prepend HH:MM:SS to each line | **adopted** | |

### Layouts

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| box | Full borders, title in top, status in bottom | **adopted** | |
| rounded | Same as box with rounded corners | **adopted** | |
| compact | Colored top bar + status, no side borders | **adopted** | |
| minimal (default) | Just status line below content | **adopted** | Default layout |
| none | No chrome at all | **adopted** | |
| quote | Left bar, optional bg tint, status inside bar | **adopted** | `--quote-bg=color\|off` |

### Interaction

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Exit on EOF | Show summary, exit | **adopted** | No scrollback |
| Keybinding bar | `q:quit` on RHS of status line | **adopted** | Expandable in v2 |

### Performance

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Render debouncing | Cap at ~30fps, batch lines between renders | **adopted** | |
| SIGWINCH handling | React to terminal resize | **adopted** | |

### Infrastructure

| Feature | Description | Status | Notes |
|---------|-------------|--------|-------|
| Window title | Set via OSC escape, restore on exit | **adopted** | |
| Custom spinner | `--spinner=braille\|dots\|arrows\|off` | **adopted** | |
| Config file | `~/.config/slit/config.toml` | **adopted** | CLI overrides config |
| Man page + `--help` | Thorough help, generated man page | **adopted** | |
| Shell completions | `slit completion bash\|zsh\|fish` + static files | **adopted** | bash, zsh, fish |
| Debug logging | `--debug` → log file, configurable `--log-file` | **adopted** | Default: `/tmp/slit-$PID.log` |
| Ring buffer | 50k lines default, configurable `--max-lines` | **adopted** | Drop oldest when full |
| Exit code | 0 on success, 1 on error | **adopted** | |
| Pipe detection | stderr not a tty → passthrough mode | **adopted** | Like less/bat |
| stderr rendering | Render to stderr, passthrough data on stdout | **adopted** | |

---

## v2 (deferred)

| Feature | Description | Notes |
|---------|-------------|-------|
| Interactive scrollback after EOF | Scroll up/down through full buffer | Depends on ring buffer growing or persisting |
| Search/filter (`/`) | Search within buffer, highlight matches | Depends on scrollback |
| Pause/resume (Space) | Freeze pane, accumulate in buffer | Key handling during streaming |
| Highlight new lines | Brief dim flash on arrival | Timed transitions |
| Progress via sidechannel | Named pipe/env var for progress hints | Complex, deferred |
| Exit code passthrough | Detect upstream process exit code | Non-trivial |

## v3 (maybe never)

| Feature | Description | Notes |
|---------|-------------|-------|
| Named pipe / multi-input | Multiple inputs in split panes or interleaved | Major architectural change |
