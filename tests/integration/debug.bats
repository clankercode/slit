#!/usr/bin/env bats

load helpers

@test "--debug flag is accepted" {
    run_slit --debug
    assert_exit_code 0
}

@test "--log-file flag is accepted" {
    run_slit --log-file "$TEMP_DIR/test.log"
    assert_exit_code 0
}

@test "--debug creates log file with default path (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --debug
    [ $? -eq 0 ]
    # Log file should be created in temp dir with pattern slit-<pid>.log
    local logfile=$(find /tmp -name "slit-*.log" -mmin -1 2>/dev/null | head -1)
    [ -n "$logfile" ]
    [ -f "$logfile" ]
    # Should contain debug entries with timestamps
    [[ "$(cat "$logfile")" =~ "init:" ]]
}

@test "--log-file creates log file at specified path (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local logfile="$TEMP_DIR/custom.log"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --log-file "$logfile"
    [ $? -eq 0 ]
    [ -f "$logfile" ]
    # Should contain debug entries
    [[ "$(cat "$logfile")" =~ "init:" ]]
}

@test "debug log contains timestamp entries (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local logfile="$TEMP_DIR/timestamp.log"
    printf "test line\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --log-file "$logfile"
    [ $? -eq 0 ]
    [ -f "$logfile" ]
    # Each line should start with HH:MM:SS.mmm timestamp
    while IFS= read -r line; do
        [[ "$line" =~ ^[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3} ]]
    done < "$logfile"
}

@test "debug log tracks buffer statistics (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local logfile="$TEMP_DIR/stats.log"
    printf "line1\nline2\nline3\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --log-file "$logfile"
    [ $? -eq 0 ]
    [ -f "$logfile" ]
    # Should contain eof message with totalLines
    [[ "$(cat "$logfile")" =~ "totalLines=" ]]
}

@test "debug log tracks window size (render mode)" {
    if [ -z "$SLIT_TEST_TTY" ]; then
        skip "Requires TTY environment"
    fi
    local logfile="$TEMP_DIR/window.log"
    printf "test\n" | SLIT_FORCE_RENDER=1 "$SLIT_BIN" --log-file "$logfile"
    [ $? -eq 0 ]
    [ -f "$logfile" ]
    # Should contain window-size message
    [[ "$(cat "$logfile")" =~ "window-size:" ]]
}