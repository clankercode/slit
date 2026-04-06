# Testing Strategy

## Overview

Two layers of testing:
1. **Per-language unit tests** — idiomatic to each implementation
2. **Shared integration tests** — bats-core scripts that validate all three binaries

## Per-Language Unit Tests

### Go

```
go test ./...
```

- Standard `testing` package or `testify`
- Test ring buffer, line trimming, ANSI parsing, config loading, layout rendering
- Table-driven tests for edge cases

### Rust

```
cargo test
```

- Built-in `#[test]` attributes
- Test ring buffer, line trimming, ANSI parsing, config loading, layout rendering
- Property-based testing optional (proptest)

### C

```
make test
```

- Minimal test framework (hand-rolled or single-header like greatest/minunit)
- Test ring buffer, line trimming, ANSI parsing, config loading
- Valgrind for memory leak detection

## Shared Integration Tests (bats-core)

### Location

```
tests/integration/
├── test.bats           # main test file
├── helpers.bash        # shared functions (run_slit, assert_output, etc.)
├── basic.bats          # basic streaming behavior
├── flags.bats          # flag parsing
├── layouts.bats        # layout rendering
├── tee.bats            # tee functionality
└── edge_cases.bats     # pipe detection, SIGWINCH, etc.
```

### Test Matrix

Each test runs against all three binaries: `go/slit`, `rust/slit`, `c/slit`

### Test Categories

#### Basic behavior
- [ ] Reads stdin and displays last N lines
- [ ] Trims lines to terminal width
- [ ] Default line count is terminal height - 1
- [ ] Exits cleanly on EOF
- [ ] Exit code 0 on clean EOF
- [ ] Exit code 1 on error (e.g. invalid flag)
- [ ] Shows done summary on EOF

#### Flags
- [ ] `-n 5` shows exactly 5 content lines
- [ ] `-l` shows line numbers (dim colored)
- [ ] `--timestamp` prepends HH:MM:SS
- [ ] `--wrap` wraps instead of truncating
- [ ] `--color=never` strips ANSI
- [ ] `--color=always` passes ANSI through
- [ ] `--truncation-char=X` uses custom indicator

#### Layouts
- [ ] `--box` renders full box borders
- [ ] `--rounded` renders rounded box borders
- [ ] `--compact` renders top bar + status
- [ ] `--minimal` renders only status line
- [ ] `--none` renders no chrome
- [ ] `--quote` renders left bar
- [ ] `--layout=X` matches `--X` shortcut

#### Tee
- [ ] `-o file` writes raw input to file
- [ ] `-a` appends to existing file
- [ ] `--tee-format=display` writes formatted output
- [ ] Tee file matches input exactly (raw mode)

#### Edge cases
- [ ] Passthrough mode when stderr is not a tty
- [ ] Handles empty input (EOF immediately)
- [ ] Handles very long lines (> terminal width)
- [ ] Handles binary/non-UTF8 input gracefully
- [ ] Handles SIGWINCH (resize)
- [ ] Handles rapid input (debouncing)
- [ ] Respects `--max-lines` buffer limit
- [ ] Progress bar appears for regular file input with known size
- [ ] Spinner works with each style (braille, dots, arrows)
- [ ] `--spinner=off` disables spinner

#### Config file
- [ ] Reads `~/.config/slit/config.toml`
- [ ] CLI flags override config values
- [ ] Missing config file uses defaults

#### Completions
- [ ] `slit completion bash` produces valid bash completion script
- [ ] `slit completion zsh` produces valid zsh completion script
- [ ] `slit completion fish` produces valid fish completion script

#### Window title
- [ ] Window title set on startup (OSC escape)
- [ ] Window title restored on exit

#### Debug logging
- [ ] `--debug` creates log file at default path
- [ ] `--log-file PATH` writes to custom path
- [ ] Log file contains render timings, buffer stats, signal events

#### Keybinding bar
- [ ] Status line RHS shows `q:quit` during streaming
- [ ] Status line RHS shows `q:quit` after EOF summary

#### Quote layout
- [ ] `--quote-bg=off` shows left bar without background
- [ ] `--quote-bg=#1a1a2e` shows left bar with background tint

### Helper Functions (helpers.bash)

```bash
SLIT_BIN="${SLIT_BIN:-}"

# Resolve which binary to test
setup() {
    if [ -z "$SLIT_BIN" ]; then
        echo "SLIT_BIN not set" >&2
        return 1
    fi
}

# Run slit with input, capture stderr output
# Usage: run_slit "input lines" [flags...]
run_slit() {
    local input="$1"
    shift
    echo "$input" | $SLIT_BIN "$@" 2>/tmp/slit-test-stderr
}

# Assert stderr contains pattern
assert_stderr_contains() {
    grep -q "$1" /tmp/slit-test-stderr
}

# Run slit and check exit code
# Usage: assert_exit_code 0 "input" -n 5
assert_exit_code() {
    local expected="$1"
    local input="$2"
    shift 2
    echo "$input" | $SLIT_BIN "$@" 2>/dev/null
    [ "$?" -eq "$expected" ]
}
```

### Running All Tests

Justfile targets:

```
just test              # run per-language unit tests for all implementations
just test-go           # go test ./...
just test-rust         # cargo test
just test-c            # make test
just test-integration  # bats tests against all binaries
just test-integration GO_BIN=/path/to/go/slit
just test-all          # unit + integration for all
```

## Test Execution via justfile

```justfile
# Build all
build: build-go build-rust build-c

build-go:
    mkdir -p target
    cd go && go build -o ../target/slit-go .

build-rust:
    mkdir -p target
    cd rust && cargo build --release && cp target/release/slit ../target/slit-rust

build-c:
    mkdir -p target
    cd c && make && cp slit ../target/slit-c

# Unit tests
test-go:
    cd go && go test ./...

test-rust:
    cd rust && cargo test

test-c:
    cd c && make test

test: test-go test-rust test-c

# Integration tests
test-integration: build
    SLIT_BIN=target/slit-go bats tests/integration
    SLIT_BIN=target/slit-rust bats tests/integration
    SLIT_BIN=target/slit-c bats tests/integration

# Everything
test-all: test test-integration
```
