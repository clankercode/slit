version := "0.1.0"

default: build

build: build-go

build-go:
    cd go && go build -o slit .

test: test-go

test-go:
    cd go && go test -v ./...

test-integration: build
    SLIT_BIN=go/slit bats tests/integration

test-all: test test-integration

clean:
    rm -f go/slit
    rm -rf target/

deps:
    cd go && go mod download

run *args:
    cd go && go run . {{args}}

completions: build
    mkdir -p completions
    ./go/slit completion bash > completions/slit.bash
    ./go/slit completion zsh > completions/_slit
    ./go/slit completion fish > completions/slit.fish

man: build
    ./go/slit --generate-man > slit.1
