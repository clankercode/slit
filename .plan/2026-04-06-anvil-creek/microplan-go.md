# Micro Plan — Go Implementation

Each step produces a testable increment. Steps are ordered by dependency.

## Step 1: Project scaffold + CLI flags

**Goal**: `go/slit --help` works, all flags defined.

- `go mod init github.com/user/slit`
- Install deps: `cobra`, `bubbletea`, `lipgloss`, `bubbles`, `BurntSushi/toml`
- `main.go`: cobra root command with all flags from cli.md
- `completion.go`: cobra completion subcommand (bash/zsh/fish)
- Flag structs map to a `Config` type
- `--version` prints version from ldflags
- **Test**: `go build && ./slit --help` shows all flags

## Step 2: Config file loading

**Goal**: `~/.config/slit/config.toml` loaded, merged with CLI flags (CLI wins).

- `config.go`: TOML struct matching cli.md config spec
- Find config path: `$XDG_CONFIG_HOME/slit/config.toml` or `~/.config/slit/config.toml`
- Merge: defaults → config file → CLI flags
- **Test**: unit test config merge logic

## Step 3: Ring buffer

**Goal**: Bounded circular buffer for lines.

- `buffer.go`: `RingBuffer` struct with `Push(line)`, `Lines() []string`, `Len()`, `Capacity()`
- Default 50k lines, configurable via `--max-lines`
- Drops oldest when full
- Tracks total line count (including evicted) for status display
- Tracks total bytes read for progress bar
- **Test**: unit test push/evict/capacity

## Step 4: Bare Bubbletea pipeline (minimal layout, no features)

**Goal**: `echo -e "line1\nline2\nline3" | ./slit -n 2` shows last 2 lines + status on stderr.

- `model.go`: Bubbletea model with `Init()`, `Update()`, `View()`
- Goroutine reads stdin via `bufio.Scanner`, sends `lineMsg` via `p.Send()`
- Inline rendering (NO `tea.WithAltScreen()`)
- Minimal layout only: N content lines + 1 status line
- Status line: `Streaming... (N lines)  q:quit`
- EOF detected → show `Done. (N lines)` summary → quit after brief pause
- Pipe detection: if `!isatty(os.Stderr)` → passthrough mode (just copy stdin→stdout)
- `q` or `ctrl+c` quits
- Content lines are raw (no trimming, no features yet)
- **Test**: `seq 1 100 | ./slit -n 5` shows lines 96-100 + status

## Step 5: Line trimming + truncation indicator

**Goal**: Lines trimmed to terminal width, `…` appended when trimmed.

- `render.go`: `trimLine(line string, width int, truncChar string) string`
- Width comes from `tea.WindowSizeMsg`
- Truncation char configurable via `--truncation-char`
- Account for layout chrome width (side borders, etc.)
- **Test**: unit test trim logic with various widths and chars

## Step 6: Layout system (all 6 layouts)

**Goal**: `--layout=box|rounded|compact|minimal|none|quote` all render correctly.

- `layouts.go`: layout functions return `string` via lipgloss composition
- Box: `lipgloss.NewStyle().Border(lipgloss.NormalBorder())` — top/bottom borders with title/status
- Rounded: `lipgloss.RoundedBorder()`
- Compact: colored top bar via `.Background().Width(w)`
- Minimal: just content + status (already done in step 4)
- None: just content, no status
- Quote: left bar `▌` on every line, title+status inside content area
- Shortcut flags (`--box`, `--compact`, etc.) resolve to layout enum
- Each layout reports its chrome costs (top lines, bottom lines, side chars)
- Content width = `termWidth - sideCost`, rendered line count adjusts
- **Test**: visual check of each layout

## Step 7: Status line (spinner + line count + progress bar + keybinding hints)

**Goal**: Full status line with animated spinner and optional progress bar.

- `spinner.go`: spinner frame sets (braille, dots, arrows, off)
- Use `bubbles/spinner` widget or hand-rolled ticker
- Status format: `<spinner> <label> (<count>) [<progress>]  <keys>`
- Progress bar: only when stdin is a regular file with known size (`os.Stdin.Stat()`)
- Progress bar: `[====      ]` proportional fill
- Keybinding hints on RHS: `q:quit`
- After EOF: spinner stops, label changes to `Done.`
- **Test**: `cat /tmp/bigfile | ./slit` shows progress bar

## Step 8: Line numbers

**Goal**: `-l` flag shows dim-colored line numbers.

- `render.go`: `formatLineWithNumber(line string, lineNum int, width int) string`
- Right-aligned number, dim/grey color via lipgloss
- Width of number gutter: enough for max digits in buffer
- Number counts from 1 (total lines, not just visible)
- **Test**: `seq 1 100 | ./slit -n 5 -l`

## Step 9: ANSI color passthrough

**Goal**: `--color=auto|always|never` handles colored input.

- `render.go`: ANSI stripping function (regex or state machine)
- `auto`: detect if stderr is tty → pass through; otherwise strip
- `always`: always pass through ANSI codes
- `never`: always strip
- ANSI-aware width calculation: don't count escape sequences toward line width
- **Test**: `echo -e "\033[31mred\033[0m normal" | ./slit --color=always`

## Step 10: Timestamp prefix

**Goal**: `-t` flag prepends `HH:MM:SS` to each line.

- Record arrival time per line in ring buffer (store `time.Time` alongside string)
- Format: `HH:MM:SS` prepended before line number (if both enabled)
- Timestamps add to the gutter width, reducing content width
- **Test**: `seq 1 5 | ./slit -t`

## Step 11: Wrap mode

**Goal**: `--wrap` wraps long lines instead of truncating.

- `render.go`: `wrapLine(line string, width int) []string` — returns multiple strings
- Each wrapped sub-line counts against the visible line budget
- Affects how many logical lines fit in the pane
- **Test**: `python3 -c "print('x'*200)" | ./slit -n 5 --wrap`

## Step 12: Tee to file

**Goal**: `-o file` writes input to file, `-a` appends.

- `tee.go`: opens file on startup, writes each line as it arrives
- `--tee-format=raw`: write original line as received
- `--tee-format=display`: write formatted line (with line numbers, timestamps, etc.)
- Close file on shutdown
- Passthrough on stdout continues regardless
- **Test**: `seq 1 10 | ./slit -o /tmp/test.log && cat /tmp/test.log`

## Step 13: Window title

**Goal**: Terminal title set to `slit` on startup, restored on exit.

- `render.go`: `setTitle(title string)` — writes OSC escape: `\x1b]0;slit\x07`
- Save original title on startup (query via `\x1b[21t` — unreliable, skip)
- Restore: set title to empty or previous on exit
- Set in Bubbletea `Init()`, restore in shutdown
- **Test**: `echo hi | ./slit` — check terminal title changes

## Step 14: Render debouncing

**Goal**: Cap at ~30fps even if input arrives faster.

- Bubbletea `tea.Tick` at 33ms interval triggers render
- Lines accumulate in buffer between ticks
- Only re-render when new lines arrived since last render
- **Test**: `cat /dev/urandom | lines | ./slit` — should not flicker excessively

## Step 15: Debug logging

**Goal**: `--debug` writes diagnostic info to log file.

- `debug.go`: `DebugLog(format string, args...)` — writes to log file
- Log entries: render timings, buffer stats, signal events, config resolution
- Default path: `/tmp/slit-$PID.log`
- Configurable via `--log-file`
- Enabled via `--debug` or config `debug.enabled = true`
- **Test**: `echo hi | ./slit --debug && cat /tmp/slit-*.log`

## Step 16: Done summary + clean exit

**Goal**: EOF shows summary, clean terminal restore, exit 0.

- On EOF: render `Done. (N lines) [===] q:quit`
- Wait briefly (100ms) so user sees summary, then exit
- Or: wait for `q` press after summary (since keybinding bar shows `q:quit`)
- Restore terminal state on all exit paths (defer in main)
- Exit 0 on clean EOF, 1 on error
- **Test**: `echo done | ./slit; echo $?` → 0

## Step 17: Shell completions + man page + --help polish

**Goal**: Production-ready CLI surface.

- `completion.go`: cobra completion subcommand (already scaffolded in step 1)
- Register custom completions for enum flags (`--color`, `--spinner`, `--layout`, etc.)
- Generate static completion files to `completions/`
- `--generate-man` outputs groff man page
- Polish `--help` output to match cli.md draft exactly
- **Test**: `./slit completion bash | source` and verify completions work

## Step 18: Integration + justfile

**Goal**: Build and test infrastructure wired up.

- Root `justfile` with build/test targets from testing.md
- Run bats integration tests against go binary
- Verify all test cases from testing.md
- **Test**: `just test-go && just test-integration`

---

## Dependency Graph

```
Step 1 (CLI) ──► Step 2 (Config) ──► Step 3 (Buffer)
                                           │
                                           ▼
                                      Step 4 (Pipeline) ──► Step 5 (Trim) ──► Step 6 (Layouts)
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
