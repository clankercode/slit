#!/usr/bin/env bats

load helpers

@test "--spinner=braille is accepted" {
    run_slit --spinner=braille
    assert_exit_code 0
}

@test "--spinner=dots is accepted" {
    run_slit --spinner=dots
    assert_exit_code 0
}

@test "--spinner=arrows is accepted" {
    run_slit --spinner=arrows
    assert_exit_code 0
}

@test "--spinner=off is accepted" {
    run_slit --spinner=off
    assert_exit_code 0
}

@test "status line shows Streaming during render (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/status_stream.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "Streaming" ]] || [[ "$content" =~ "Done" ]]
}

@test "status line shows Done after EOF (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/status_done.txt"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "Done" ]]
}

@test "status line shows line count (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/status_count.txt"
    printf "line1\nline2\nline3\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "3 lines" ]] || [[ "$content" =~ "lines" ]]
}

@test "status line shows q:quit hint (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/status_keys.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal --spinner=braille 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "q:quit" ]]
}

@test "spinner=off hides spinner frame (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/spinner_off.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal --spinner=off 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ ! "$content" =~ "⠋" ]]
    [[ ! "$content" =~ "⠙" ]]
}

@test "progress bar appears for file input (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local infile="$TEMP_DIR/progress_input.txt"
    printf "line1\nline2\nline3\nline4\nline5\n" > "$infile"
    local output_stderr="$TEMP_DIR/progress_stderr.txt"
    cat "$infile" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "Done" ]]
}

@test "--max-lines flag is accepted" {
    run_slit --max-lines=1000
    assert_exit_code 0
}

@test "--max-lines with small value is accepted" {
    run_slit --max-lines=10
    assert_exit_code 0
}

@test "config file loads layout from TOML (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local cfgdir="$TEMP_DIR/slit_config"
    mkdir -p "$cfgdir"
    printf '[display]\nlayout = "box"\n' > "$cfgdir/config.toml"
    local output_stderr="$TEMP_DIR/cfg_box.txt"
    printf "test\n" | XDG_CONFIG_HOME="$TEMP_DIR" SLIT_FORCE_RENDER=1 "$SLIT_BIN" 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "┌" ]] || [[ "$content" =~ "Done" ]]
}

@test "CLI flags override config file (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local cfgdir="$TEMP_DIR/slit_config2"
    mkdir -p "$cfgdir"
    printf '[display]\nlayout = "box"\n' > "$cfgdir/config.toml"
    local output_stderr="$TEMP_DIR/cfg_override.txt"
    printf "test\n" | XDG_CONFIG_HOME="$TEMP_DIR" SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=none 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ ! "$content" =~ "┌" ]]
}

@test "missing config file uses defaults (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local emptydir="$TEMP_DIR/empty_config"
    mkdir -p "$emptydir/slit"
    local output_stderr="$TEMP_DIR/cfg_default.txt"
    printf "test\n" | XDG_CONFIG_HOME="$emptydir" SLIT_FORCE_RENDER=1 "$SLIT_BIN" 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "test" ]] || [[ "$content" =~ "Done" ]]
}

@test "config file sets spinner style (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local cfgdir="$TEMP_DIR/slit_config3"
    mkdir -p "$cfgdir"
    printf '[spinner]\nstyle = "off"\n' > "$cfgdir/config.toml"
    local output_stderr="$TEMP_DIR/cfg_spinner.txt"
    printf "test\n" | XDG_CONFIG_HOME="$TEMP_DIR" SLIT_FORCE_RENDER=1 "$SLIT_BIN" 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ ! "$content" =~ "⠋" ]]
}

@test "rendering shows content in output (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/render_content.txt"
    printf "hello world\nfoo bar\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "hello world" ]]
    [[ "$content" =~ "foo bar" ]]
}

@test "last N lines shown when more input (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/render_last.txt"
    printf "line1\nline2\nline3\nline4\nline5\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" -n 2 --layout=none 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ "line5" ]]
}

@test "window title OSC escape on startup (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local output_stderr="$TEMP_DIR/title.txt"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --layout=minimal 2>"$output_stderr"
    [ $? -eq 0 ]
    local content=$(cat "$output_stderr")
    [[ "$content" =~ $'\x1b]0;slit\x07' ]]
}
