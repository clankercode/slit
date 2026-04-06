#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

log()  { echo -e "${GREEN}[BUILD]${NC} $*"; }
warn() { echo -e "${YELLOW}[BUILD]${NC} $*"; }
err()  { echo -e "${RED}[BUILD]${NC} $*" >&2; }

BINARIES=()

build_go() {
    local bin="$ROOT/go/slit"
    if [ -x "$bin" ]; then
        log "Go binary already exists: $bin"
    else
        log "Building Go implementation..."
        (cd "$ROOT/go" && go build -o slit .)
        log "Built: $bin"
    fi
    BINARIES+=("$bin")
}

build_rust() {
    local bin="$ROOT/rust/target/release/slit"
    if [ -x "$bin" ]; then
        log "Rust binary already exists: $bin"
    else
        log "Building Rust implementation..."
        (cd "$ROOT/rust" && cargo build --release)
        log "Built: $bin"
    fi
    BINARIES+=("$bin")
}

build_c() {
    local bin="$ROOT/c/slit"
    if [ -x "$bin" ]; then
        log "C binary already exists: $bin"
    else
        log "Building C implementation..."
        (cd "$ROOT/c" && make)
        log "Built: $bin"
    fi
    BINARIES+=("$bin")
}

verify() {
    local ok=true
    for bin in "${BINARIES[@]}"; do
        if [ ! -x "$bin" ]; then
            err "Binary not found or not executable: $bin"
            ok=false
        else
            log "Verified: $bin"
        fi
    done
    if [ "$ok" != true ]; then
        err "Some binaries missing or not executable"
        exit 1
    fi
}

echo "=== Building all slit implementations ==="
build_go
build_rust
build_c
echo ""
echo "=== Verifying binaries ==="
verify
echo ""
log "All binaries ready"
