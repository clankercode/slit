#!/usr/bin/env bats

load helpers

@test "When stderr is not a TTY, stdin is copied to stdout" {
    output=$(printf "hello world\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
    [ "$output" = "hello world" ]
}

@test "Passthrough preserves exact content" {
    output=$(printf "line1\nline2\nline3\n" | "$SLIT_BIN")
    [ $? -eq 0 ]
    [ "$output" = "line1
line2
line3" ]
}

@test "Passthrough with empty input succeeds" {
    output=$(printf "" | "$SLIT_BIN")
    [ $? -eq 0 ]
    [ -z "$output" ]
}
