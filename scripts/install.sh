#!/bin/sh
set -eu

REPO="clankercode/slit"
BINARY="slit"
INSTALL_DIR="${SLIT_BIN:-${HOME}/.local/bin}"
TMPDIR="${TMPDIR:-/tmp}"

main() {
    need_cmd curl
    need_cmd tar
    need_cmd sha256sum

    os="$(detect_os)"
    arch="$(detect_arch)"
    impl="${1:-c}"

    case "$impl" in
        c) artifact="slit-c-${os}-${arch}.tar.gz" ;;
        rust) artifact="slit-rust-${os}-${arch}.tar.gz" ;;
        go) artifact="slit-go-${os}-${arch}.tar.gz" ;;
        *) err "unknown implementation: $impl (use c, rust, or go)" ;;
    esac

    printf '  slit installer — %s implementation (%s/%s)\n\n' "$impl" "$os" "$arch"

    download_url="https://github.com/${REPO}/releases/latest/download/${artifact}"
    checksums_url="https://github.com/${REPO}/releases/latest/download/SHA256SUMS"

    tmp="$(mktemp -d "${TMPDIR}/slit-install.XXXXXX")"
    trap 'rm -rf "$tmp"' EXIT

    printf '  downloading %s...\n' "$artifact"
    curl -sL -o "$tmp/$artifact" "$download_url"

    printf '  downloading checksums...\n'
    curl -sL -o "$tmp/SHA256SUMS" "$checksums_url"

    printf '  verifying checksum...\n'
    expected="$(grep "  ${artifact}$" "$tmp/SHA256SUMS" | cut -d' ' -f1)"
    if [ -z "$expected" ]; then
        err "checksum not found for $artifact"
    fi
    actual="$(sha256sum "$tmp/$artifact" | cut -d' ' -f1)"
    if [ "$expected" != "$actual" ]; then
        err "checksum mismatch\n  expected: $expected\n  actual:   $actual"
    fi

    printf '  extracting...\n'
    tar -xzf "$tmp/$artifact" -C "$tmp"

    mkdir -p "$INSTALL_DIR"
    mv "$tmp/$BINARY" "$INSTALL_DIR/$BINARY"
    chmod 755 "$INSTALL_DIR/$BINARY"

    printf '\n  installed %s to %s/%s\n' "$impl" "$INSTALL_DIR" "$BINARY"

    case ":$PATH:" in
        *":$INSTALL_DIR:"*)
            ;;
        *)
            printf '\n  add %s to your PATH:\n' "$INSTALL_DIR"
            printf '    export PATH="%s:\$PATH"\n' "$INSTALL_DIR"
            ;;
    esac

    printf '\n  done. run: slit --version\n'
}

detect_os() {
    uname="$(uname -s)"
    case "$uname" in
        Linux*) printf 'linux' ;;
        Darwin*) printf 'darwin' ;;
        *) err "unsupported OS: $uname" ;;
    esac
}

detect_arch() {
    uname="$(uname -m)"
    case "$uname" in
        x86_64|amd64) printf 'x86_64' ;;
        aarch64|arm64) printf 'arm64' ;;
        *) err "unsupported architecture: $uname" ;;
    esac
}

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        err "required command not found: $1"
    fi
}

err() {
    printf '\n  error: %s\n' "$1" >&2
    exit 1
}

main "$@"
