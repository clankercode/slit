package main

import (
	"fmt"
	"strings"

	"github.com/mattn/go-runewidth"
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

func formatBytesHuman(bytes int64) string {
	if bytes < 1024 {
		return fmt.Sprintf("%dB", bytes)
	} else if bytes < 1024*1024 {
		return fmt.Sprintf("%.1fKB", float64(bytes)/1024.0)
	} else if bytes < 1024*1024*1024 {
		return fmt.Sprintf("%.1fMB", float64(bytes)/(1024.0*1024.0))
	}
	return fmt.Sprintf("%.1fGB", float64(bytes)/(1024.0*1024.0*1024.0))
}

func FormatStatusLine(spinnerStyle string, frame int, eof bool, lineCount int, totalBytes int64, fileSize int64, width int) string {
	spinner := GetSpinnerFrame(spinnerStyle, frame)
	byteStr := formatBytesHuman(totalBytes)

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
		line = fmt.Sprintf("Done. (%d lines, %s)%s%s", lineCount, byteStr, progressPart, keysPart)
	} else {
		line = fmt.Sprintf("%s Streaming... (%d lines, %s)%s%s", spinner, lineCount, byteStr, progressPart, keysPart)
	}

	if width <= 0 {
		return line
	}

	lineWidth := runewidth.StringWidth(line)
	if lineWidth > width {
		return TrimLineANSI(line, width, "")
	}
	if lineWidth < width {
		return line + strings.Repeat(" ", width-lineWidth)
	}
	return line
}
