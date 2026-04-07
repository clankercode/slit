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

@test "Passthrough head+tail: shows first and last N lines with separator" {
    output=$(seq 25 | "$SLIT_BIN" -n 5)
    first5=$(echo "$output" | head -5)
    [ "$first5" = "$(seq 1 5)" ]
    echo "$output" | grep -q 'lines omitted'
    last5=$(echo "$output" | tail -5)
    [ "$last5" = "$(seq 21 25)" ]
}

@test "Passthrough head+tail: no separator when total <= 2N" {
    output=$(seq 10 | "$SLIT_BIN" -n 5)
    ! echo "$output" | grep -q 'omitted'
    [ "$output" = "$(seq 10)" ]
}

@test "Passthrough head+tail: fewer than N lines shows everything" {
    output=$(seq 3 | "$SLIT_BIN" -n 10)
    [ "$output" = "$(seq 3)" ]
}

@test "Passthrough head+tail: -n 0 pipes everything" {
    output=$(seq 100 | "$SLIT_BIN" -n 0)
    [ "$output" = "$(seq 100)" ]
}

@test "Passthrough head+tail: default N is 10" {
    output=$(seq 25 | "$SLIT_BIN")
    first10=$(echo "$output" | head -10)
    [ "$first10" = "$(seq 1 10)" ]
    echo "$output" | grep -q 'lines omitted'
    last10=$(echo "$output" | tail -10)
    [ "$last10" = "$(seq 16 25)" ]
}

@test "Passthrough head+tail: tee captures all lines including skipped middle" {
    tmpfile="$(mktemp)"
    seq 25 | "$SLIT_BIN" -n 5 -o "$tmpfile"
    [ "$(wc -l < "$tmpfile")" -eq 25 ]
    diff "$tmpfile" <(seq 25)
    rm -f "$tmpfile"
}
