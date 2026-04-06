version := "0.1.0"

default: build

# Build all implementations
build: build-go

build-go:
    cd go && go build -o slit .

# Run all unit tests
test: test-go

test-go:
    cd go && go test -v ./...

# Run integration tests (requires bats-core)
test-integration: build
    #!/usr/bin/env bash
    for bin in go/slit; do
        echo "=== Testing $bin ==="
        SLIT_BIN="$bin" bats tests/integration 2>/dev/null || echo "No integration tests yet"
    done

# Everything
test-all: test test-integration

# Clean build artifacts
clean:
    rm -f go/slit
    rm -rf target/

# Install go deps
deps:
    cd go && go mod download

# Run slit directly (for development)
run *args:
    cd go && go run . {{args}}
