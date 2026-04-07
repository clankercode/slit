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

@test "Passthrough mode writes to -o file" {
    tmpfile="$(mktemp)"
    printf "line1\nline2\nline3\n" | "$SLIT_BIN" -o "$tmpfile"
    [ $? -eq 0 ]
    diff <(cat "$tmpfile") <(printf "line1\nline2\nline3\n")
    rm -f "$tmpfile"
}

@test "Passthrough mode -o --append appends" {
    tmpfile="$(mktemp)"
    printf "first\n" | "$SLIT_BIN" -o "$tmpfile"
    printf "second\n" | "$SLIT_BIN" -o "$tmpfile" -a
    diff <(cat "$tmpfile") <(printf "first\nsecond\n")
    rm -f "$tmpfile"
}
