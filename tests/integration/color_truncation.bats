#!/usr/bin/env bats

load helpers

# Color mode tests - Note: color processing only happens in render mode

@test "--color=never flag is accepted" {
    run_slit --color=never
    assert_exit_code 0
}

@test "--color=always flag is accepted" {
    run_slit --color=always
    assert_exit_code 0
}

@test "--color=auto flag is accepted" {
    run_slit --color=auto
    assert_exit_code 0
}

@test "--color=never strips ANSI codes (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    # Input with ANSI color codes (red text)
    local input="\033[31mred text\033[0m"
    local output_stderr="$TEMP_DIR/color_stderr.txt"
    printf "$input\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --color=never 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Output should not contain ANSI escape sequences
    [[ ! "$content" =~ \x1b\[ ]]
    [[ "$content" =~ "red text" ]]
}

@test "--color=always preserves ANSI codes (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local input="\033[31mred text\033[0m"
    local output_stderr="$TEMP_DIR/color_always.txt"
    printf "$input\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --color=always 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Output should still contain ANSI escape sequences
    [[ "$content" =~ \x1b\[31m ]]
}

@test "--color=auto strips ANSI when stderr is not TTY (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    # Force isStderrTTY to false using environment
    local input="\033[31mred text\033[0m"
    local output_stderr="$TEMP_DIR/color_auto.txt"
    printf "$input\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --color=auto 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should be stripped since we're in non-TTY environment
    [[ ! "$content" =~ \x1b\[31m ]]
}

# Truncation char tests

@test "--truncation-char flag accepts custom character" {
    run_slit --truncation-char=">"
    assert_exit_code 0
}

@test "--truncation-char accepts multi-byte character" {
    run_slit --truncation-char="…"
    assert_exit_code 0
}

@test "--truncation-char=~ is accepted" {
    run_slit --truncation-char="~"
    assert_exit_code 0
}

@test "truncation char appears in output for long lines (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    # Create a very long line that will be truncated
    local longline=$(printf '%0.s-' {1..500})
    local output_stderr="$TEMP_DIR/trunc_stderr.txt"
    printf "${longline}\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --truncation-char=">" 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain the truncation character
    [[ "$content" =~ ">" ]]
}

@test "default truncation char is ellipsis (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local longline=$(printf '%0.s-' {1..500})
    local output_stderr="$TEMP_DIR/trunc_default.txt"
    printf "${longline}\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain the default ellipsis character
    [[ "$content" =~ … ]]
}