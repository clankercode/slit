package main

import (
	"os"
	"path/filepath"

	"github.com/BurntSushi/toml"
)

type FileConfig struct {
	Display struct {
		Layout         string `toml:"layout"`
		Lines          int    `toml:"lines"`
		LineNumbers    bool   `toml:"line_numbers"`
		Color          string `toml:"color"`
		Wrap           bool   `toml:"wrap"`
		Timestamp      bool   `toml:"timestamp"`
		TruncationChar string `toml:"truncation_char"`
	} `toml:"display"`
	Buffer struct {
		MaxLines int `toml:"max_lines"`
	} `toml:"buffer"`
	Spinner struct {
		Style string `toml:"style"`
	} `toml:"spinner"`
	Quote struct {
		Bg string `toml:"bg"`
	} `toml:"quote"`
	Debug struct {
		Enabled bool   `toml:"enabled"`
		LogFile string `toml:"log_file"`
	} `toml:"debug"`
}

type Config struct {
	Lines          int
	MaxLines       int
	Output         string
	Append         bool
	TeeFormat      string
	LineNumbers    bool
	Color          string
	Wrap           bool
	Timestamp      bool
	TruncationChar string
	Layout         string
	QuoteBg        string
	Spinner        string
	Debug          bool
	LogFile        string
}

func DefaultConfig() Config {
	return Config{
		Lines:          0,
		MaxLines:       50000,
		Output:         "",
		Append:         false,
		TeeFormat:      "raw",
		LineNumbers:    false,
		Color:          "auto",
		Wrap:           false,
		Timestamp:      false,
		TruncationChar: "…",
		Layout:         "minimal",
		QuoteBg:        "off",
		Spinner:        "braille",
		Debug:          false,
		LogFile:        "",
	}
}

func LoadConfigFile() (FileConfig, error) {
	var cfg FileConfig

	dir := os.Getenv("XDG_CONFIG_HOME")
	if dir == "" {
		home, err := os.UserHomeDir()
		if err != nil {
			return cfg, nil
		}
		dir = filepath.Join(home, ".config")
	}

	path := filepath.Join(dir, "slit", "config.toml")

	if _, err := os.Stat(path); os.IsNotExist(err) {
		return cfg, nil
	}

	if _, err := toml.DecodeFile(path, &cfg); err != nil {
		return cfg, err
	}

	return cfg, nil
}

func MergeConfig(defaults Config, file FileConfig) Config {
	result := defaults

	if file.Display.Layout != "" {
		result.Layout = file.Display.Layout
	}
	if file.Display.Lines > 0 {
		result.Lines = file.Display.Lines
	}
	if file.Display.LineNumbers {
		result.LineNumbers = file.Display.LineNumbers
	}
	if file.Display.Color != "" {
		result.Color = file.Display.Color
	}
	if file.Display.Wrap {
		result.Wrap = file.Display.Wrap
	}
	if file.Display.Timestamp {
		result.Timestamp = file.Display.Timestamp
	}
	if file.Display.TruncationChar != "" {
		result.TruncationChar = file.Display.TruncationChar
	}
	if file.Buffer.MaxLines > 0 {
		result.MaxLines = file.Buffer.MaxLines
	}
	if file.Spinner.Style != "" {
		result.Spinner = file.Spinner.Style
	}
	if file.Quote.Bg != "" {
		result.QuoteBg = file.Quote.Bg
	}
	if file.Debug.Enabled {
		result.Debug = file.Debug.Enabled
	}
	if file.Debug.LogFile != "" {
		result.LogFile = file.Debug.LogFile
	}

	return result
}

func ApplyFileConfig(base Config) Config {
	file, err := LoadConfigFile()
	if err != nil {
		return base
	}
	return MergeConfig(base, file)
}
