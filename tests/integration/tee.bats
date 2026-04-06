#!/usr/bin/env bats

load helpers

@test "-o flag is accepted in passthrough mode" {
    local outfile="$TEMP_DIR/output.txt"
    output=$(printf "hello\n" | "$SLIT_BIN" -o "$outfile")
    [ $? -eq 0 ]
    [ "$output" = "hello" ]
}

@test "-a flag is accepted in passthrough mode" {
    local outfile="$TEMP_DIR/output.txt"
    printf "first\n" > "$outfile"
    output=$(printf "second\n" | "$SLIT_BIN" -o "$outfile" -a)
    [ $? -eq 0 ]
    [ "$output" = "second" ]
}

@test "--tee-format=raw is accepted" {
    run_slit --tee-format=raw
    assert_exit_code 0
}

@test "--tee-format=display is accepted" {
    run_slit --tee-format=display
    assert_exit_code 0
}

@test "multiple lines pass through with -o flag" {
    local outfile="$TEMP_DIR/output.txt"
    output=$(printf "line1\nline2\nline3\n" | "$SLIT_BIN" -o "$outfile")
    [ $? -eq 0 ]
    [ "$output" = "line1
line2
line3" ]
}

@test "-o file content verification - writes all input lines (render mode)" {
    # Skip if not in TTY environment (CI)
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/tee_output.txt"
    printf "hello world\nsecond line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile"
    [ $? -eq 0 ]
    [ -f "$outfile" ]
    local content=$(cat "$outfile")
    [ "$content" = "hello world
second line" ]
}

@test "-o flag overwrites existing file by default (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/overwrite.txt"
    printf "old content\n" > "$outfile"
    printf "new content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile"
    [ $? -eq 0 ]
    local content=$(cat "$outfile")
    [ "$content" = "new content" ]
}

@test "-a flag appends to existing file (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/append.txt"
    printf "first line\n" > "$outfile"
    printf "second line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile" -a
    [ $? -eq 0 ]
    local content=$(cat "$outfile")
    [ "$content" = "first line
second line" ]
}

@test "--tee-format=display writes formatted output with timestamps (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/display.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile" --tee-format=display -t
    [ $? -eq 0 ]
    [ -f "$outfile" ]
    local content=$(cat "$outfile")
    # Should contain timestamp in HH:MM:SS format
    [[ "$content" =~ ^[0-9]{2}:[0-9]{2}:[0-9]{2} ]]
}

@test "--tee-format=display writes formatted output with line numbers (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/display_ln.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile" --tee-format=display -l
    [ $? -eq 0 ]
    [ -f "$outfile" ]
    local content=$(cat "$outfile")
    # Should contain line number
    [[ "$content" =~ 1 ]]
}

@test "--tee-format=raw writes raw input without formatting (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local outfile="$TEMP_DIR/raw.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -o "$outfile" --tee-format=raw -t -l
    [ $? -eq 0 ]
    [ -f "$outfile" ]
    local content=$(cat "$outfile")
    # Raw format should NOT contain timestamps or line numbers
    [ "$content" = "test line" ]
}