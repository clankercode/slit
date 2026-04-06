# CI & Release Plan for slit

## Overview

Set up GitHub Actions CI for automated testing on push/PR, and automated releases
on tag push or manual dispatch. Builds all three implementations (Go, Rust, C) for
Linux x86_64, with commented-out cross-compile scaffolding for macOS and Windows.

**C is the default implementation.** The C binary ships as `slit` in all packages.
Go and Rust binaries ship as `slit-go` and `slit-rust` respectively.

## Current State

- `ci.yml` exists: Go build/test only on ubuntu-latest
- Repo: `clankercode/slit` on GitHub
- Three implementations: `go/`, `rust/`, `c/`
- Version hardcoded as `0.1.0` in all three
- No release automation exists

## File Structure (new/modified)

```
.github/
  workflows/
    ci.yml              # REWRITE — full test matrix for all impls
    release.yml         # NEW — build, package, publish releases
  build/
    build-go.sh         # NEW — build Go binary with version injection
    build-rust.sh       # NEW — build Rust binary with version injection
    build-c.sh          # NEW — build C binary with version injection
    package.sh          # NEW — create tarball/deb/rpm/PKGBUILD artifacts
```

## Version Injection Strategy

- Parse version from `git describe --tags --always`
- If on a tag like `v0.2.0`: version = `0.2.0`
- If not on a tag: version = `0.2.0-4-gabcdef1` (tag + commits + hash)
- Inject via:
  - **Go**: `-ldflags "-X main.version=$VERSION"`
  - **Rust**: `CARGO_PKG_VERSION` env or `--config` flag
  - **C**: `-DVERSION="$VERSION"` compiler flag

## Binary Naming Convention

| Implementation | Binary name in packages |
|---------------|------------------------|
| C (default)   | `slit`                 |
| Go            | `slit-go`              |
| Rust          | `slit-rust`            |

All packages install the C binary as the primary `slit` command.

## ci.yml — Full Test Matrix

**Triggers:** push to main/master, PRs to main/master

### Job: test

**Runs on:** `ubuntu-latest`

Steps:
1. Checkout
2. Install Go, Rust, GCC, bats-core, Python3
3. Build all three: `just build-all`
4. Run Go unit tests: `just test-go`
5. Run Rust unit tests: `just test-rust`
6. Run C unit tests: `just test-c`
7. Run integration tests (Go binary): `SLIT_BIN=go/slit bats tests/integration`
8. Run integration tests (C binary): `SLIT_BIN=c/slit bats tests/integration`
9. Run integration tests (Rust binary): `SLIT_BIN=rust/target/release/slit bats tests/integration`
10. Run cross-impl comparison tests: `cd tests/cross-impl && python3 run.py`

## release.yml — Build, Package & Release

**Triggers:**
- Push tag matching `v*` → stable release
- `workflow_dispatch` with `prerelease` boolean input → manual release

### Job: build-binaries

**Runs on:** `ubuntu-latest`

Steps:
1. Checkout with full history (`fetch-depth: 0` for `git describe`)
2. Determine version: `git describe --tags --always | sed 's/^v//'`
3. Install toolchains (Go, Rust, GCC)
4. Build C binary → strip → rename to `slit`
5. Build Go binary → strip → rename to `slit-go`
6. Build Rust binary → strip → rename to `slit-rust`
7. Generate completions: `./go/slit completion bash > completions/slit.bash`, etc.
8. Generate man page: `./go/slit --generate-man > slit.1`
9. Upload all artifacts

### Job: package

**Needs:** `build-binaries`

Download all binaries, then produce:

1. **Tarballs** (per-impl):
   - `slit-<version>-linux-x86_64.tar.gz` — contains `slit` (C), man page, completions, README
   - `slit-go-<version>-linux-x86_64.tar.gz` — contains `slit-go`, same extras
   - `slit-rust-<version>-linux-x86_64.tar.gz` — contains `slit-rust`, same extras

2. **Debian packages** (per-impl):
   - `slit_<version>_amd64.deb` — C binary as `/usr/bin/slit`
   - `slit-go_<version>_amd64.deb` — Go binary as `/usr/bin/slit-go`
   - `slit-rust_<version>_amd64.deb` — Rust binary as `/usr/bin/slit-rust`
   - Each includes man page, completions, postinst/postrm scripts

3. **RPM packages** (per-impl):
   - `slit-<version>-1.x86_64.rpm` — C binary as `/usr/bin/slit`
   - `slit-go-<version>-1.x86_64.rpm`
   - `slit-rust-<version>-1.x86_64.rpm`

4. **PKGBUILD** (for AUR, attached to release but not auto-published):
   - `PKGBUILD` — builds C binary, version/sha256 filled in
   - `.SRCINFO` — generated from PKGBUILD

5. **Checksums:**
   - `SHA256SUMS.txt` — checksums of all release artifacts
   - `SHA256SUMS.txt.sig` — GPG-signed checksums (if signing key configured)

### Job: release

**Needs:** `package`

1. Download all artifacts
2. Generate changelog from `git log` since last tag
3. Create GitHub Release via `gh release create`:
   - Tag: `$VERSION`
   - Title: `slit $VERSION`
   - Body: auto-generated changelog + install instructions
   - `--prerelease` flag if triggered manually with prerelease=true
   - Attach all artifacts (tarballs, debs, rpms, PKGBUILD, checksums)

### Commented macOS Cross-Compile Block

```yaml
  # - target: macos-arm64
  #   os: macos-latest
  #   steps:
  #     - Build Go: GOOS=darwin GOARCH=arm64 go build ...
  #     - Build Rust: rustup target add aarch64-apple-darwin; cargo build --target ...
  #     - Build C: make build (native on macos runner)
  #     - Strip, rename, upload
  #
  # - target: macos-x86_64
  #   os: macos-13  # Intel runner
  #   steps: (same as arm64 with x86_64 targets)
```

### Commented Windows Cross-Compile Block

```yaml
  # - target: windows-x86_64
  #   os: windows-latest
  #   steps:
  #     - Build Go: GOOS=windows go build -o slit-go.exe ...
  #     - Build Rust: rustup target add x86_64-pc-windows-msvc; cargo build --target ...
  #     - Build C: make build (may need MinGW adjustments)
  #     - Rename to .exe, upload
```

## Implementation Tasks

1. **Create `.github/build/build-c.sh`** — version-aware C build script (default impl)
2. **Create `.github/build/build-go.sh`** — version-aware Go build script
3. **Create `.github/build/build-rust.sh`** — version-aware Rust build script
4. **Create `.github/build/package.sh`** — tarball/deb/rpm/PKGBUILD generation
5. **Rewrite `.github/workflows/ci.yml`** — full test matrix for all 3 impls + cross-impl
6. **Create `.github/workflows/release.yml`** — build, package, release pipeline
7. **Update version in source** — ensure all three impls read injected version correctly
8. **Test locally** — verify build scripts work with `just build-all` and version injection
9. **Push and verify** — open PR, confirm CI passes all jobs
10. **Tag v0.1.0** — verify release workflow triggers and publishes correctly
