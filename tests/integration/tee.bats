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
