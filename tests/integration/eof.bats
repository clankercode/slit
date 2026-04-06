#!/usr/bin/env bats

load helpers

# EOF behavior and exit code tests

@test "empty input exits with code 0" {
    output=$(printf "" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "single line input exits with code 0" {
    output=$(printf "single line\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "multiple lines input exits with code 0" {
    output=$(printf "line1\nline2\nline3\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "input without trailing newline exits with code 0" {
    output=$(printf "no newline" | "$SLIT_BIN")
    [ $? -eq 0 ]
    [ "$output" = "no newline" ]
}

@test "large input exits with code 0" {
    # Generate 1000 lines
    local large_input=$(seq 1 1000)
    output=$(printf "%s\n" "$large_input" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "input with special characters exits with code 0" {
    output=$(printf "tab\there\nnewline\n\n\r\ncarriage\r\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "input with unicode exits with code 0" {
    output=$(printf "Hello 世界 🌍\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
}

@test "EOF in render mode exits cleanly (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN"
    [ $? -eq 0 ]
}

@test "multiple EOFs handled correctly" {
    # Sending multiple lines and EOF
    output=$(printf "line1\nline2\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
    [ "$output" = "line1
line2" ]
}

@test "binary-safe input handling" {
    # Input with null bytes and control chars
    output=$(printf "before\x00after\n" | "$SLIT_BIN")
    # Go scanner splits on newlines, null byte handling may vary
    [ $? -eq 0 ]
}