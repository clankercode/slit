version := "0.2.1"

default: build

build: build-c

build-all: build-go build-rust build-c
    @echo ""
    @echo "=== Binary Stats ==="
    @printf "%-8s %-10s %s\n" "Impl" "Size" "Type"
    @printf "%-8s %-10s %s\n" "----" "----" "----"
    @printf "%-8s %-10s %s\n" "go"   "$(ls -lh go/slit | awk '{print $5}')" "$(file -b go/slit | cut -d, -f1)"
    @printf "%-8s %-10s %s\n" "rust"  "$(ls -lh rust/target/release/slit | awk '{print $5}')" "$(file -b rust/target/release/slit | cut -d, -f1)"
    @printf "%-8s %-10s %s\n" "c"     "$(ls -lh c/slit | awk '{print $5}')" "$(file -b c/slit | cut -d, -f1)"

build-go:
    cd go && go build -o slit .

build-rust:
    cd rust && cargo build --release

build-c:
    cd c && make build

test: test-c

test-go:
    cd go && go test -v ./...

test-rust:
    cd rust && cargo test --verbose

test-c:
    cd c && make test

test-integration: build
    SLIT_BIN=go/slit bats tests/integration

test-all: test test-integration

clean:
    rm -f go/slit
    cd rust && cargo clean
    cd c && make clean

deps:
    cd go && go mod download

run *args:
    cd go && go run . {{args}}

run-go *args:
    cd go && go run . {{args}}

run-rust *args:
    cd rust && cargo build --release && ./target/release/slit {{args}}

run-c *args:
    cd c && make build && ./slit {{args}}

completions: completions-c completions-go completions-rust

completions-c: build-c
    mkdir -p completions/c
    ./c/slit completion bash > completions/c/slit.bash
    ./c/slit completion zsh > completions/c/_slit
    ./c/slit completion fish > completions/c/slit.fish

completions-go: build-go
    mkdir -p completions/go
    ./go/slit completion bash > completions/go/slit.bash
    ./go/slit completion zsh > completions/go/_slit
    ./go/slit completion fish > completions/go/slit.fish

completions-rust: build-rust
    mkdir -p completions/rust
    ./rust/target/release/slit completion bash > completions/rust/slit.bash
    ./rust/target/release/slit completion zsh > completions/rust/_slit
    ./rust/target/release/slit completion fish > completions/rust/slit.fish

install-go: build-go
    mkdir -p ~/.local/bin
    mkdir -p ~/.local/share/man/man1
    cp go/slit ~/.local/bin/slit
    ./go/slit --generate-man > ~/.local/share/man/man1/slit.1

install-rust: build-rust
    mkdir -p ~/.local/bin
    mkdir -p ~/.local/share/man/man1
    cp rust/target/release/slit ~/.local/bin/slit
    ./rust/target/release/slit --generate-man > ~/.local/share/man/man1/slit.1

install-c: build-c
    mkdir -p ~/.local/bin
    mkdir -p ~/.local/share/man/man1
    cp c/slit ~/.local/bin/slit
    ./c/slit --generate-man > ~/.local/share/man/man1/slit.1

benchmark: build-all
    ./scripts/benchmark --ansi

man: man-c man-go man-rust

man-c: build-c
    mkdir -p man/c
    ./c/slit --generate-man > man/c/slit.1

man-go: build-go
    mkdir -p man/go
    ./go/slit --generate-man > man/go/slit.1

man-rust: build-rust
    mkdir -p man/rust
    ./rust/target/release/slit --generate-man > man/rust/slit.1
