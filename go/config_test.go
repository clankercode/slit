package main

import (
	"os"
	"path/filepath"
	"testing"
)

func TestDefaultConfig(t *testing.T) {
	cfg := DefaultConfig()
	if cfg.Layout != "minimal" {
		t.Errorf("expected default layout 'minimal', got %q", cfg.Layout)
	}
	if cfg.Color != "auto" {
		t.Errorf("expected default color 'auto', got %q", cfg.Color)
	}
	if cfg.Spinner != "braille" {
		t.Errorf("expected default spinner 'braille', got %q", cfg.Spinner)
	}
	if cfg.MaxLines != 50000 {
		t.Errorf("expected default maxLines 50000, got %d", cfg.MaxLines)
	}
	if cfg.TruncationChar != "…" {
		t.Errorf("expected default truncationChar '…', got %q", cfg.TruncationChar)
	}
	if cfg.Wrap {
		t.Error("expected default wrap false")
	}
	if cfg.LineNumbers {
		t.Error("expected default lineNumbers false")
	}
	if cfg.Timestamp {
		t.Error("expected default timestamp false")
	}
	if cfg.Debug {
		t.Error("expected default debug false")
	}
}

func TestMergeConfigOverrides(t *testing.T) {
	base := DefaultConfig()

	file := FileConfig{}
	file.Display.Layout = "box"
	file.Display.Color = "never"
	file.Display.Wrap = true
	file.Display.LineNumbers = true
	file.Display.Timestamp = true
	file.Display.TruncationChar = ">"
	file.Buffer.MaxLines = 1000
	file.Spinner.Style = "dots"
	file.Quote.Bg = "#1a1a2e"
	file.Debug.Enabled = true
	file.Debug.LogFile = "/tmp/test.log"

	result := MergeConfig(base, file)

	if result.Layout != "box" {
		t.Errorf("expected layout 'box', got %q", result.Layout)
	}
	if result.Color != "never" {
		t.Errorf("expected color 'never', got %q", result.Color)
	}
	if !result.Wrap {
		t.Error("expected wrap true")
	}
	if !result.LineNumbers {
		t.Error("expected lineNumbers true")
	}
	if !result.Timestamp {
		t.Error("expected timestamp true")
	}
	if result.TruncationChar != ">" {
		t.Errorf("expected truncationChar '>', got %q", result.TruncationChar)
	}
	if result.MaxLines != 1000 {
		t.Errorf("expected maxLines 1000, got %d", result.MaxLines)
	}
	if result.Spinner != "dots" {
		t.Errorf("expected spinner 'dots', got %q", result.Spinner)
	}
	if result.QuoteBg != "#1a1a2e" {
		t.Errorf("expected quoteBg '#1a1a2e', got %q", result.QuoteBg)
	}
	if !result.Debug {
		t.Error("expected debug true")
	}
	if result.LogFile != "/tmp/test.log" {
		t.Errorf("expected logFile '/tmp/test.log', got %q", result.LogFile)
	}
}

func TestMergeConfigNoOverride(t *testing.T) {
	base := DefaultConfig()
	file := FileConfig{}
	result := MergeConfig(base, file)

	if result.Layout != "minimal" {
		t.Errorf("expected layout to stay 'minimal', got %q", result.Layout)
	}
	if result.MaxLines != 50000 {
		t.Errorf("expected maxLines to stay 50000, got %d", result.MaxLines)
	}
}

func TestMergeConfigPartial(t *testing.T) {
	base := DefaultConfig()
	file := FileConfig{}
	file.Display.Layout = "box"
	result := MergeConfig(base, file)

	if result.Layout != "box" {
		t.Errorf("expected layout 'box', got %q", result.Layout)
	}
	if result.Color != "auto" {
		t.Errorf("expected color to stay 'auto', got %q", result.Color)
	}
}

func TestLoadConfigFileMissing(t *testing.T) {
	dir := t.TempDir()
	os.Setenv("XDG_CONFIG_HOME", dir)
	defer os.Unsetenv("XDG_CONFIG_HOME")

	cfg, err := LoadConfigFile()
	if err != nil {
		t.Errorf("expected no error for missing config, got %v", err)
	}
	if cfg.Display.Layout != "" {
		t.Errorf("expected empty layout, got %q", cfg.Display.Layout)
	}
}

func TestLoadConfigFileExists(t *testing.T) {
	dir := t.TempDir()
	os.Setenv("XDG_CONFIG_HOME", dir)
	defer os.Unsetenv("XDG_CONFIG_HOME")

	cfgDir := filepath.Join(dir, "slit")
	os.MkdirAll(cfgDir, 0755)

	configContent := `
[display]
layout = "box"
color = "never"
wrap = true

[buffer]
max_lines = 5000

[spinner]
style = "arrows"

[debug]
enabled = true
log_file = "/tmp/custom.log"
`
	os.WriteFile(filepath.Join(cfgDir, "config.toml"), []byte(configContent), 0644)

	cfg, err := LoadConfigFile()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if cfg.Display.Layout != "box" {
		t.Errorf("expected layout 'box', got %q", cfg.Display.Layout)
	}
	if cfg.Display.Color != "never" {
		t.Errorf("expected color 'never', got %q", cfg.Display.Color)
	}
	if !cfg.Display.Wrap {
		t.Error("expected wrap true")
	}
	if cfg.Buffer.MaxLines != 5000 {
		t.Errorf("expected max_lines 5000, got %d", cfg.Buffer.MaxLines)
	}
	if cfg.Spinner.Style != "arrows" {
		t.Errorf("expected spinner 'arrows', got %q", cfg.Spinner.Style)
	}
	if !cfg.Debug.Enabled {
		t.Error("expected debug enabled true")
	}
	if cfg.Debug.LogFile != "/tmp/custom.log" {
		t.Errorf("expected log_file '/tmp/custom.log', got %q", cfg.Debug.LogFile)
	}
}

func TestApplyFileConfigNoFile(t *testing.T) {
	dir := t.TempDir()
	os.Setenv("XDG_CONFIG_HOME", dir)
	defer os.Unsetenv("XDG_CONFIG_HOME")

	base := DefaultConfig()
	result := ApplyFileConfig(base)

	if result.Layout != "minimal" {
		t.Errorf("expected layout to stay 'minimal', got %q", result.Layout)
	}
}

func TestLoadConfigFileInvalidTOML(t *testing.T) {
	dir := t.TempDir()
	os.Setenv("XDG_CONFIG_HOME", dir)
	defer os.Unsetenv("XDG_CONFIG_HOME")

	cfgDir := filepath.Join(dir, "slit")
	os.MkdirAll(cfgDir, 0755)

	os.WriteFile(filepath.Join(cfgDir, "config.toml"), []byte("invalid [toml {{{"), 0644)

	_, err := LoadConfigFile()
	if err == nil {
		t.Error("expected error for invalid TOML")
	}
}

func TestValidateConfigRejectsInvalidLayout(t *testing.T) {
	cfg := DefaultConfig()
	cfg.Layout = "bogus"
	err := ValidateConfig(&cfg)
	if err == nil {
		t.Error("expected error for invalid layout")
	}
}

func TestValidateConfigRejectsInvalidColor(t *testing.T) {
	cfg := DefaultConfig()
	cfg.Color = "burgundy"
	err := ValidateConfig(&cfg)
	if err == nil {
		t.Error("expected error for invalid color")
	}
}

func TestValidateConfigRejectsInvalidSpinner(t *testing.T) {
	cfg := DefaultConfig()
	cfg.Spinner = "spinny"
	err := ValidateConfig(&cfg)
	if err == nil {
		t.Error("expected error for invalid spinner")
	}
}

func TestValidateConfigRejectsInvalidTeeFormat(t *testing.T) {
	cfg := DefaultConfig()
	cfg.TeeFormat = "pdf"
	err := ValidateConfig(&cfg)
	if err == nil {
		t.Error("expected error for invalid tee-format")
	}
}

func TestValidateConfigAcceptsValidValues(t *testing.T) {
	cfg := DefaultConfig()
	err := ValidateConfig(&cfg)
	if err != nil {
		t.Errorf("expected no error for valid config, got %v", err)
	}
}

func TestApplyFileConfigRejectsInvalidLayout(t *testing.T) {
	dir := t.TempDir()
	os.Setenv("XDG_CONFIG_HOME", dir)
	defer os.Unsetenv("XDG_CONFIG_HOME")

	cfgDir := filepath.Join(dir, "slit")
	os.MkdirAll(cfgDir, 0755)

	configContent := `
[display]
layout = "nonexistent"
`
	os.WriteFile(filepath.Join(cfgDir, "config.toml"), []byte(configContent), 0644)

	base := DefaultConfig()
	result := ApplyFileConfig(base)

	if result.Layout != "minimal" {
		t.Errorf("expected layout to stay 'minimal' after invalid config, got %q", result.Layout)
	}
}
