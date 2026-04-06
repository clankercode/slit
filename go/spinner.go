package main

import (
	"fmt"
	"strings"
)

var SpinnerFrames = map[string][]string{
	"braille": {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"},
	"dots":    {"⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"},
	"arrows":  {"←", "↖", "↑", "↗", "→", "↘", "↓", "↙"},
	"off":     {},
}

func GetSpinnerFrame(style string, frame int) string {
	frames, ok := SpinnerFrames[style]
	if !ok || len(frames) == 0 {
		return ""
	}
	return frames[frame%len(frames)]
}

func SpinnerFrameCount(style string) int {
	return len(SpinnerFrames[style])
}

func FormatStatusLine(spinnerStyle string, frame int, eof bool, lineCount int, totalBytes int64, fileSize int64, width int) string {
	spinner := GetSpinnerFrame(spinnerStyle, frame)

	progressPart := ""
	if fileSize > 0 {
		barWidth := 10
		filled := int(float64(barWidth) * float64(totalBytes) / float64(fileSize))
		if filled > barWidth {
			filled = barWidth
		}
		bar := strings.Repeat("=", filled) + strings.Repeat(" ", barWidth-filled)
		progressPart = fmt.Sprintf(" [%s]", bar)
	}

	keysPart := ""
	if spinnerStyle != "off" {
		keysPart = "  q:quit"
	}

	var line string
	if eof {
		line = fmt.Sprintf("Done. (%d lines)%s%s", lineCount, progressPart, keysPart)
	} else {
		line = fmt.Sprintf("%s Streaming... (%d lines)%s%s", spinner, lineCount, progressPart, keysPart)
	}

	if width <= 0 {
		return line
	}

	lineRunes := []rune(line)
	if len(lineRunes) > width {
		return string(lineRunes[:width])
	}
	if len(lineRunes) < width {
		return line + strings.Repeat(" ", width-len(lineRunes))
	}
	return line
}
