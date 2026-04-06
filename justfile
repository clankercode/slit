version := "0.1.0"

default: build

build: build-go

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

test: test-go

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
    cd rust && cargo run --release -- {{args}}

run-c *args:
    cd c && make build && ./slit {{args}}

completions: build
    mkdir -p completions
    ./go/slit completion bash > completions/slit.bash
    ./go/slit completion zsh > completions/_slit
    ./go/slit completion fish > completions/slit.fish

install-go: build-go
    mkdir -p ~/.local/bin
    cp go/slit ~/.local/bin/slit

install-rust: build-rust
    mkdir -p ~/.local/bin
    cp rust/target/release/slit ~/.local/bin/slit

install-c: build-c
    mkdir -p ~/.local/bin
    cp c/slit ~/.local/bin/slit

benchmark: build-all
    ./scripts/benchmark --ansi

man: build
    ./go/slit --generate-man > slit.1
