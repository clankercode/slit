#!/usr/bin/env bats

load helpers

# Wrap mode tests

@test "--wrap flag is accepted" {
    run_slit --wrap
    assert_exit_code 0
}

@test "-w short flag is accepted" {
    run_slit -w
    assert_exit_code 0
}

@test "wrap mode wraps long lines (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    # Create a line longer than typical terminal width
    local longline=$(printf '%0.sA' {1..300})
    local output_stderr="$TEMP_DIR/wrap_stderr.txt"
    printf "${longline}\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --wrap 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # In wrap mode, the line should appear on multiple lines
    local line_count=$(echo "$content" | wc -l)
    [ "$line_count" -gt 1 ]
}

@test "wrap mode preserves ANSI codes across wraps (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    # Create colored long line
    local longline="\033[31m$(printf '%0.sA' {1..300})\033[0m"
    local output_stderr="$TEMP_DIR/wrap_ansi.txt"
    printf "${longline}\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --wrap --color=always 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain ANSI codes in wrapped lines
    [[ "$content" =~ \x1b\[31m ]]
}

# Line numbers tests

@test "-l flag is accepted" {
    run_slit -l
    assert_exit_code 0
}

@test "--line-numbers flag is accepted" {
    run_slit --line-numbers
    assert_exit_code 0
}

@test "line numbers appear in output (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/ln_stderr.txt"
    printf "line1\nline2\nline3\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -l 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain line numbers
    [[ "$content" =~ "1" ]]
    [[ "$content" =~ "2" ]]
    [[ "$content" =~ "3" ]]
}

@test "line numbers are right-aligned (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/ln_align.txt"
    # Generate 12 lines to get double-digit numbers
    seq 1 12 | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -l 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Line 10 should have proper spacing
    [[ "$content" =~ "10" ]]
}

# Timestamp tests

@test "-t flag is accepted" {
    run_slit -t
    assert_exit_code 0
}

@test "--timestamp flag is accepted" {
    run_slit --timestamp
    assert_exit_code 0
}

@test "timestamps appear in HH:MM:SS format (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/ts_stderr.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -t 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain timestamp in HH:MM:SS format
    [[ "$content" =~ [0-9]{2}:[0-9]{2}:[0-9]{2} ]]
}

@test "timestamps with line numbers both appear (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/ts_ln_stderr.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -t -l 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain both timestamp and line number
    [[ "$content" =~ [0-9]{2}:[0-9]{2}:[0-9]{2} ]]
    [[ "$content" =~ "1" ]]
}

# Combined features tests

@test "wrap with line numbers works together (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local longline=$(printf '%0.sA' {1..300})
    local output_stderr="$TEMP_DIR/wrap_ln.txt"
    printf "${longline}\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -w -l 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should have line number on first wrapped line
    [[ "$content" =~ "1" ]]
}