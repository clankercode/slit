#!/usr/bin/env bash
# asciinema demo script for slit
#
# Usage:
#   asciinema rec demo.cast --command bash docs/demo-script.sh
#
# Or run directly to test:
#   bash docs/demo-script.sh
#
# Prerequisites:
#   - slit binary built (just build-go)
#   - scripts/slit-test-data available
#   - Terminal at least 80x24

set -euo pipefail

SLIT="${SLIT_BIN:-./go/slit}"
TEST_DATA="./scripts/slit-test-data"

pause() {
    sleep "$1"
}

section() {
    echo ""
    echo "### $1 ###"
    pause 1
}

echo "========================================"
echo "  slit — streaming terminal viewer demo"
echo "========================================"
pause 2

# --- 1. Basic usage ---
section "Basic usage: streaming output"

echo '$ seq 100 | '"$SLIT"
pause 1
seq 100 | SLIT_FORCE_RENDER=1 "$SLIT" 2>/dev/null || true
pause 1

# --- 2. Line numbers ---
section "Line numbers (-l)"

echo '$ seq 50 | '"$SLIT"' -l'
pause 1
seq 50 | SLIT_FORCE_RENDER=1 "$SLIT" -l 2>/dev/null || true
pause 1

# --- 3. Timestamps ---
section "Timestamps (-t)"

echo '$ seq 30 | '"$SLIT"' -t -n 10'
pause 1
seq 30 | SLIT_FORCE_RENDER=1 "$SLIT" -t -n 10 2>/dev/null || true
pause 1

# --- 4. Wrap mode ---
section "Wrap mode (--wrap)"

echo '$ python3 -c "for i in range(20): print(\"x\" * 120)" | '"$SLIT"' --wrap -n 8'
pause 1
python3 -c 'for i in range(20): print("x" * 120)' | SLIT_FORCE_RENDER=1 "$SLIT" --wrap -n 8 2>/dev/null || true
pause 1

# --- 5. Layout: box ---
section "Layout: box"

echo '$ ./scripts/slit-test-data waterfall | '"$SLIT"' --box -n 8'
pause 1
timeout 3 "$TEST_DATA" waterfall | SLIT_FORCE_RENDER=1 "$SLIT" --box -n 8 2>/dev/null || true
pause 1

# --- 6. Layout: rounded ---
section "Layout: rounded"

echo '$ ./scripts/slit-test-data gradient | '"$SLIT"' --rounded -n 8'
pause 1
timeout 3 "$TEST_DATA" gradient | SLIT_FORCE_RENDER=1 "$SLIT" --rounded -n 8 2>/dev/null || true
pause 1

# --- 7. Layout: compact ---
section "Layout: compact"

echo '$ ./scripts/slit-test-data spectrum | '"$SLIT"' --compact -n 8'
pause 1
timeout 3 "$TEST_DATA" spectrum | SLIT_FORCE_RENDER=1 "$SLIT" --compact -n 8 2>/dev/null || true
pause 1

# --- 8. Layout: quote ---
section "Layout: quote"

echo '$ ./scripts/slit-test-data rain | '"$SLIT"' --quote -n 8'
pause 1
timeout 3 "$TEST_DATA" rain | SLIT_FORCE_RENDER=1 "$SLIT" --quote -n 8 2>/dev/null || true
pause 1

# --- 9. Layout: none ---
section "Layout: none"

echo '$ ./scripts/slit-test-data maze | '"$SLIT"' --none -n 8'
pause 1
timeout 3 "$TEST_DATA" maze | SLIT_FORCE_RENDER=1 "$SLIT" --none -n 8 2>/dev/null || true
pause 1

# --- 10. Color modes ---
section "Color modes"

echo '$ ./scripts/slit-test-data matrix | '"$SLIT"' --color=always -n 8'
pause 1
timeout 3 "$TEST_DATA" matrix | SLIT_FORCE_RENDER=1 "$SLIT" --color=always -n 8 2>/dev/null || true
pause 1

# --- 11. Spinner styles ---
section "Spinner: dots"

echo '$ seq 200 | '"$SLIT"' --spinner=dots -n 8'
pause 1
seq 200 | SLIT_FORCE_RENDER=1 "$SLIT" --spinner=dots -n 8 2>/dev/null || true
pause 1

section "Spinner: arrows"

echo '$ seq 200 | '"$SLIT"' --spinner=arrows -n 8'
pause 1
seq 200 | SLIT_FORCE_RENDER=1 "$SLIT" --spinner=arrows -n 8 2>/dev/null || true
pause 1

# --- 12. Progress bar ---
section "Progress bar (file with known size)"

echo '$ cat /usr/share/dict/words | '"$SLIT"' --box -n 8'
pause 1
if [ -f /usr/share/dict/words ]; then
    cat /usr/share/dict/words | SLIT_FORCE_RENDER=1 "$SLIT" --box -n 8 2>/dev/null || true
else
    seq 10000 | SLIT_FORCE_RENDER=1 "$SLIT" --box -n 8 2>/dev/null || true
fi
pause 1

# --- 13. Combined features ---
section "Combined: box + line numbers + timestamps + wrap"

echo '$ ./scripts/slit-test-data cyberdeck | '"$SLIT"' --box -l -t -n 10'
pause 1
timeout 5 "$TEST_DATA" cyberdeck | SLIT_FORCE_RENDER=1 "$SLIT" --box -l -t -n 10 2>/dev/null || true
pause 1

# --- 14. Tee to file ---
section "Tee to file (-o)"

OUTFILE="/tmp/slit-demo-output.log"
echo '$ ./scripts/slit-test-data aurora | '"$SLIT"' --compact -o /tmp/slit-demo-output.log -n 8'
pause 1
timeout 3 "$TEST_DATA" aurora | SLIT_FORCE_RENDER=1 "$SLIT" --compact -o "$OUTFILE" -n 8 2>/dev/null || true
pause 1
echo ""
echo "Output saved to $OUTFILE:"
wc -l "$OUTFILE"
rm -f "$OUTFILE"
pause 1

# --- End ---
echo ""
echo "========================================"
echo "  Demo complete!"
echo "========================================"
pause 2
