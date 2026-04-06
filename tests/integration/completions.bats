#!/usr/bin/env bats

load helpers

@test "completion bash produces non-empty output" {
    run_slit completion bash
    assert_exit_code 0
    [ -n "$output" ]
    [[ "$output" =~ "completion" ]]
}

@test "completion zsh produces non-empty output" {
    run_slit completion zsh
    assert_exit_code 0
    [ -n "$output" ]
}

@test "completion fish produces non-empty output" {
    run_slit completion fish
    assert_exit_code 0
    [ -n "$output" ]
}

@test "completion with unsupported shell exits 1" {
    run_slit completion csh
    assert_exit_code 1
    [[ "$output" =~ "unsupported shell" ]]
}
