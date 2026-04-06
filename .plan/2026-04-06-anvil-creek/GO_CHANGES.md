# Changes Made to Go Implementation

## Enhancement Session (2026-04-06)

### New Go Unit Tests

#### Config Tests (`go/config_test.go`) — 8 tests
- `TestDefaultConfig` - Verifies all default config values
- `TestMergeConfigOverrides` - All fields from file config override defaults
- `TestMergeConfigNoOverride` - Empty file config leaves defaults unchanged
- `TestMergeConfigPartial` - Only set fields override
- `TestLoadConfigFileMissing` - Missing config returns empty without error
- `TestLoadConfigFileExists` - Full TOML config parsed correctly
- `TestApplyFileConfigNoFile` - Missing config doesn't alter defaults
- `TestLoadConfigFileInvalidTOML` - Invalid TOML returns error

#### Layout Tests (`go/layouts_test.go`) — 13 tests
- `TestGetLayoutAll` - All 6 layout names resolve
- `TestGetLayoutPanics` - Unknown layout panics
- `TestLayoutChromeCosts` - Correct top/bottom/side costs for all layouts
- `TestRenderLayoutBox` - Box corners and content present
- `TestRenderLayoutRounded` - Rounded corners present
- `TestRenderLayoutMinimal` - Content + status, no borders
- `TestRenderLayoutNone` - Content only, no status
- `TestRenderLayoutQuote` - Quote marker present
- `TestRenderLayoutCompact` - Content and status present
- `TestRenderLayoutBoxWidthAlignment` - All lines exactly N runes wide
- `TestPadRight` / `TestPadRightOverflow` / `TestPadRightExact`

#### Spinner Tests (`go/spinner_test.go`) — 13 tests
- `TestGetSpinnerFrame*` - All 4 spinner styles (braille/dots/arrows/off)
- `TestGetSpinnerFrameWraps` - Frame wraps around
- `TestGetSpinnerFrameUnknown` - Unknown returns empty
- `TestFormatStatusLineStreaming` - Non-EOF status line
- `TestFormatStatusLineDone` - EOF status line
- `TestFormatStatusLineProgress` - Progress bar with known file size
- `TestFormatStatusLineProgressFull` - 100% progress
- `TestFormatStatusLineZeroWidth` - Zero width doesn't crash
- `TestFormatStatusLineNarrowWidth` - Truncates to narrow width
- `TestFormatStatusLineSpinnerOff` - Spinner off hides frame

#### Additional Render Tests (`go/render_test.go`) — 12 new tests
- `TestTrimLine_widthOne` / `TestTrimLine_customTruncChar` / `TestTrimLine_multiByteTruncChar`
- `TestStripANSI_multipleCodes` / `TestStripANSI_oscSequence`
- `TestVisibleWidth_plain` / `TestVisibleWidth_unicode`
- `TestWrapLine_exactSplit`
- `TestTrimLineANSI_preservesColor` / `TestTrimLineANSI_truncatesWithColor`
- `TestWrapLineANSI_multipleColors`

### Go Benchmarks (`go/bench_test.go`) — 21 benchmarks
Run via `go test -bench=. -benchmem`:
- `BenchmarkRingBufferPush` — 46 ns/op, 0 allocs
- `BenchmarkRingBufferPushEvict` — 46 ns/op, 0 allocs
- `BenchmarkRingBufferLast` — 190 ns/op, 1 alloc
- `BenchmarkRingBufferLines` — 636 µs/op (full 50k buffer)
- `BenchmarkTrimLine` — 711 ns/op (200-char line)
- `BenchmarkTrimLineShort` — 5.6 ns/op (short-circuit path)
- `BenchmarkTrimLineANSI` — 1.5 µs/op
- `BenchmarkWrapLineANSI` — 2.5 µs/op
- `BenchmarkWrapLineANSIColor` — 2.8 µs/op
- `BenchmarkStripANSI` — 740 ns/op
- `BenchmarkStripANSIHeavy` — 10 µs/op (50 color codes)
- `BenchmarkVisibleWidth` — 376 ns/op
- `BenchmarkFormatStatusLine` — 407 ns/op
- `BenchmarkFormatStatusLineEOF` — 329 ns/op
- `BenchmarkRenderLayoutMinimal` — 100 ns/op
- `BenchmarkRenderLayoutBox` — 3.4 µs/op
- `BenchmarkFullRenderCycle` — 6.5 µs/op (20-line minimal)
- `BenchmarkFullRenderCycleBox` — 11 µs/op
- `BenchmarkFullRenderCycleWithANSI` — 10 µs/op
- `BenchmarkFullRenderCycleWrap` — 62 µs/op (long lines)
- `BenchmarkFullRenderCycleLineNumbers` — 8.8 µs/op

### New Integration Tests (`tests/integration/spinner_progress.bats`) — 24 tests
- Spinner style acceptance (braille/dots/arrows/off)
- Status line shows "Streaming" during render
- Status line shows "Done" after EOF
- Status line shows line count
- Status line shows "q:quit" hint
- spinner=off hides braille frames
- Progress bar for file input
- `--max-lines` flag acceptance
- Config file loads layout from TOML
- CLI flags override config file
- Missing config file uses defaults
- Config file sets spinner style
- Rendering shows content in output
- Last N lines shown when more input
- Window title OSC escape on startup

### --help Polish
- Added `Example` section to cobra root command with 3 usage examples matching cli.md spec

### Code Quality
- `go vet ./...` — clean
- `gofmt -d .` — no diffs
- No dead code or unused imports found

## Test Summary

| Category | Count |
|----------|-------|
| Go unit tests (buffer) | 15 |
| Go unit tests (render + maze) | 23 |
| Go unit tests (config) | 8 |
| Go unit tests (layouts) | 13 |
| Go unit tests (spinner) | 13 |
| **Go unit total** | **72** |
| Integration tests (existing) | 84 |
| Integration tests (new) | 24 |
| **Integration total** | **108** |
| Go benchmarks | 21 |
| **Grand total** | **201** |

## Behavior Clarifications for Rust/C Implementations

### Passthrough Mode vs Render Mode

**Critical Discovery**: The Go implementation has two distinct operating modes:

1. **Passthrough Mode** (default in non-TTY): Simply copies stdin to stdout via `io.Copy()`. Most flags have no effect in this mode.
2. **Render Mode** (active in TTY or with `SLIT_FORCE_RENDER=1`): Full TUI with all features enabled.

**Implications**:
- Tee file writing (`-o`, `-a`) only works in render mode
- Debug logging (`--debug`) only works in render mode
- Color processing only happens in render mode
- Layouts only render in render mode
- Line numbers, timestamps, wrapping only apply in render mode

### Feature Implementation Details

**Line Numbers** (`-l`, `--line-numbers`):
- Right-aligned with dynamic padding based on total line count
- Only shown on first line of wrapped content
- Format: `  1 ` (space-padded, right-aligned, trailing space)

**Timestamps** (`-t`, `--timestamp`):
- Format: `HH:MM:SS` (24-hour, always 8 chars + space = 9 chars gutter)
- Uses local system time at line arrival
- Appears before line numbers if both enabled

**Truncation Character** (`--truncation-char`):
- Default: `…` (Unicode ellipsis, U+2026)
- Single character only (can be multi-byte Unicode)
- Only shown when line exceeds visible width
- Accounts for ANSI sequences when calculating width

**Color Modes** (`--color`):
- `never`: Strip all ANSI CSI sequences (`\x1b\[[0-9;]*[a-zA-Z]`) and OSC sequences (`\x1b\].*?\x07`)
- `always`: Preserve all ANSI sequences
- `auto`: Strip if stderr is not a TTY, preserve if TTY

**Wrap Mode** (`-w`, `--wrap`):
- Wraps at terminal width minus gutter width (line numbers + timestamps)
- Preserves ANSI color codes across wrapped lines
- Each wrapped segment is a separate line in output

**Tee Format** (`--tee-format`):
- `raw`: Writes original input lines only
- `display`: Writes formatted output with timestamps/line numbers
- Display format order: timestamp (if enabled) + line number (if enabled) + content

**Debug Logging** (`--debug`, `--log-file`):
- Default path: `/tmp/slit-<pid>.log`
- Format: `HH:MM:SS.mmm message`
- Key log entries: `init:`, `window-size:`, `eof:`
- Mutex-protected for thread safety

**Layouts**:
- `box`: Heavy borders with title bar
- `rounded`: Light rounded corners with title bar
- `compact`: Top bar with background color
- `minimal`: Content + status line only
- `none`: Content only, no decorations
- `quote`: Left border with ▌ marker

**Window Title**:
- Sets OSC escape `\x1b]0;slit\x07` on startup (only when stderr is TTY and layout is not "none")
- Resets title to empty `\x1b]0;\x07` on exit

### Exit Codes

- `0`: Success (normal exit after EOF or 'q'/Ctrl+C)
- `1`: Error (invalid flag value, TTY error, man page generation error)

### Buffer Behavior

- Ring buffer with configurable max lines (default 50,000)
- Oldest lines evicted when max reached
- Total count and byte tracking for status line

## File Locations

- **Source**: `go/*.go` (main, config, buffer, model, render, layouts, spinner, tee, debug)
- **Unit Tests**: `go/*_test.go` (buffer_test, render_test, maze_test, config_test, layouts_test, spinner_test, bench_test)
- **Integration Tests**: `tests/integration/*.bats`
  - `tee.bats` - Tee file writing tests
  - `debug.bats` - Debug logging tests
  - `color_truncation.bats` - Color and truncation tests
  - `wrap_lines.bats` - Wrap, line numbers, timestamps tests
  - `layout.bats` - Layout rendering tests
  - `eof.bats` - EOF behavior tests
  - `flags.bats` - Flag validation tests
  - `passthrough.bats` - Basic passthrough tests
  - `completions.bats` - Shell completion tests
  - `spinner_progress.bats` - Spinner, progress bar, config file, rendering, window title tests
  - `helpers.bash` - Test utilities
- **CI**: `.github/workflows/ci.yml`
- **Documentation**: `.plan/2026-04-06-anvil-creek/GO_CHANGES.md` (this file)

## Notes for Rust/C Teams

1. **TTY Detection**: Use `isatty(stderr)` (C) or `atty` crate (Rust)
2. **ANSI Stripping**: Use regex patterns from `render.go` lines 8-9
3. **Wrap Logic**: Consider visible width vs byte width carefully
4. **Timestamp Format**: Use `%H:%M:%S` (strftime) or equivalent
5. **Debug Logging**: Use mutex for thread safety
6. **Ring Buffer**: Implement circular buffer with eviction
7. **Layout Rendering**: Box-drawing characters are Unicode, not ASCII
8. **Integration Tests**: Use `bats` framework for shell-level testing
9. **CI Strategy**: TTY-dependent tests should skip in CI, run locally with `SLIT_TEST_TTY=1`
10. **Benchmarks**: Use Go benchmarks as performance reference targets
