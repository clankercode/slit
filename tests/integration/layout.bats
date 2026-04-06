#!/usr/bin/env bats

load helpers

# Layout rendering tests

@test "--layout=box renders box borders (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/box.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=box 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain box-drawing characters
    [[ "$content" =~ "┌" ]]
    [[ "$content" =~ "┐" ]]
    [[ "$content" =~ "└" ]]
    [[ "$content" =~ "┘" ]]
}

@test "--layout=rounded renders rounded borders (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/rounded.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=rounded 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain rounded corners
    [[ "$content" =~ "╭" ]]
    [[ "$content" =~ "╮" ]]
    [[ "$content" =~ "╰" ]]
    [[ "$content" =~ "╯" ]]
}

@test "--layout=compact renders compact style (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/compact.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=compact 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should have top bar and status line
    [ -n "$content" ]
}

@test "--layout=minimal renders minimal style (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/minimal.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Minimal should just show content and status
    [[ "$content" =~ "test content" ]]
}

@test "--layout=none renders no borders (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/none.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=none 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # None layout should only show content
    [[ "$content" =~ "test content" ]]
    [[ ! "$content" =~ "┌" ]]
    [[ ! "$content" =~ "╭" ]]
}

@test "--layout=quote renders quote style (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/quote.txt"
    printf "test content\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=quote 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    # Should contain quote marker
    [[ "$content" =~ "▌" ]]
}

# Layout shortcut flags (already tested in flags.bats, but verify render output)

@test "--box shortcut renders box layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/box_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --box 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "┌" ]]
}

@test "--rounded shortcut renders rounded layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/rounded_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --rounded 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "╭" ]]
}

@test "--compact shortcut renders compact layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/compact_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --compact 2>"$output_stderr"
    [ $? -eq 0 ]
    [ -s "$output_stderr" ]
}

@test "--minimal shortcut renders minimal layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/minimal_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    [[ "$(cat "$output_stderr")" =~ "test" ]]
}

@test "--none shortcut renders no layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/none_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --none 2>"$output_stderr"
    [ $? -eq 0 ]
    [[ "$(cat "$output_stderr")" =~ "test" ]]
    [[ ! "$(cat "$output_stderr")" =~ "┌" ]]
}

@test "--quote shortcut renders quote layout (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/quote_short.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --quote 2>"$output_stderr"
    [ $? -eq 0 ]
    [[ "$(cat "$output_stderr")" =~ "▌" ]]
}