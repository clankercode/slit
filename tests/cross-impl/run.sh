#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  test    Run all test cases (default)"
    echo "  fuzz    Run fuzzing mode (default: 50 iterations)"
    echo "  build   Build all 3 implementations"
    echo "  help    Show this help"
    echo ""
    echo "Options:"
    echo "  FUZZ_N=100 $0 fuzz    Set fuzz iterations"
}

cmd="${1:-test}"

case "$cmd" in
    test)
        python3 "$SCRIPT_DIR/run_tests.py"
        ;;
    fuzz)
        n="${FUZZ_N:-50}"
        echo "Running fuzzing with $n iterations..."
        python3 "$SCRIPT_DIR/fuzz.py" "$n"
        ;;
    build)
        "$SCRIPT_DIR/build.sh"
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "Unknown command: $cmd"
        usage
        exit 1
        ;;
esac
