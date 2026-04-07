setup() {
    if [ -z "$SLIT_BIN" ]; then
        echo "SLIT_BIN must be set" >&2
        return 1
    fi
    TEMP_DIR="$(mktemp -d)"
    export TEMP_DIR
}

teardown() {
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
    fi
}

run_slit() {
    run "$SLIT_BIN" "$@"
}

assert_exit_code() {
    local expected="$1"
    [ "$status" -eq "$expected" ]
}

assert_error_exit() {
    [ "$status" -eq 1 ] || [ "$status" -eq 2 ]
}
