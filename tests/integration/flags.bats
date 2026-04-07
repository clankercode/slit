#!/usr/bin/env bats

load helpers

@test "invalid --color value exits with error" {
    run_slit --color=invalid
    assert_error_exit
    [[ "$output" =~ "invalid" ]] || [[ "$output" =~ "error" ]]
}

@test "invalid --spinner value exits with error" {
    run_slit --spinner=invalid
    assert_error_exit
    [[ "$output" =~ "invalid" ]] || [[ "$output" =~ "error" ]]
}

@test "invalid --layout value exits with error" {
    run_slit --layout=invalid
    assert_error_exit
    [[ "$output" =~ "invalid" ]] || [[ "$output" =~ "error" ]]
}

@test "invalid --tee-format value exits with error" {
    run_slit --tee-format=invalid
    assert_error_exit
    [[ "$output" =~ "invalid" ]] || [[ "$output" =~ "error" ]]
}

@test "--help exits 0" {
    run_slit --help
    assert_exit_code 0
    [[ "$output" =~ "Usage:" ]]
}

@test "--version exits 0 and shows version" {
    run_slit --version
    assert_exit_code 0
    [[ "$output" =~ "slit" ]]
}

@test "--generate-man exits 0 and produces groff-like output" {
    run_slit --generate-man
    assert_exit_code 0
    [[ "$output" =~ \.TH ]]
}

@test "--box shortcut is valid" {
    run_slit --box
    assert_exit_code 0
}

@test "--rounded shortcut is valid" {
    run_slit --rounded
    assert_exit_code 0
}

@test "--compact shortcut is valid" {
    run_slit --compact
    assert_exit_code 0
}

@test "--minimal shortcut is valid" {
    run_slit --minimal
    assert_exit_code 0
}

@test "--none shortcut is valid" {
    run_slit --none
    assert_exit_code 0
}

@test "--quote shortcut is valid" {
    run_slit --quote
    assert_exit_code 0
}
