#!/usr/bin/env bats

load helpers

@test "keyboard: slit exits cleanly when 'q' is sent via PTY" {
    if [ -z "$SLIT_BIN" ]; then
        skip "SLIT_BIN must be set"
    fi
    if ! python3 -c "import pty" 2>/dev/null; then
        skip "python3 pty module required"
    fi

    result=$(python3 "$BATS_TEST_DIRNAME/keyboard_quit.py" "$SLIT_BIN" 2>&1)
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "FAILED: $result"
        return 1
    fi
}

@test "keyboard: slit exits cleanly when ctrl+c is sent via PTY" {
    if [ -z "$SLIT_BIN" ]; then
        skip "SLIT_BIN must be set"
    fi
    if ! python3 -c "import pty" 2>/dev/null; then
        skip "python3 pty module required"
    fi

    result=$(python3 "$BATS_TEST_DIRNAME/keyboard_quit.py" "$SLIT_BIN" --ctrl-c 2>&1)
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "FAILED: $result"
        return 1
    fi
}
