package main

import (
	"regexp"
	"unicode/utf8"
)

var ansiCSI = regexp.MustCompile(`\x1b\[[0-9;]*[a-zA-Z]`)
var ansiOSC = regexp.MustCompile(`\x1b\].*?\x07`)

func TrimLine(line string, width int, truncChar string) string {
	if utf8.RuneCountInString(line) <= width {
		return line
	}
	targetWidth := width - utf8.RuneCountInString(truncChar)
	runes := []rune(line)
	return string(runes[:targetWidth]) + truncChar
}

func WrapLine(line string, width int) []string {
	if width <= 0 {
		return []string{line}
	}
	if line == "" {
		return []string{""}
	}
	runes := []rune(line)
	var result []string
	for i := 0; i < len(runes); i += width {
		end := i + width
		if end > len(runes) {
			end = len(runes)
		}
		chunk := string(runes[i:end])
		result = append(result, chunk)
	}
	return result
}

func StripANSI(s string) string {
	s = ansiCSI.ReplaceAllString(s, "")
	s = ansiOSC.ReplaceAllString(s, "")
	return s
}

func VisibleWidth(s string) int {
	return utf8.RuneCountInString(StripANSI(s))
}

func TrimLineANSI(line string, width int, truncChar string) string {
	if VisibleWidth(line) <= width {
		return line
	}
	targetVisible := width - utf8.RuneCountInString(truncChar)

	visible := 0
	var resultRunes []rune
	i := 0
	runes := []rune(line)

	for i < len(runes) {
		if runes[i] == '\x1b' {
			var seqRunes []rune
			seqRunes = append(seqRunes, runes[i])
			i++
			if i < len(runes) && runes[i] == '[' {
				seqRunes = append(seqRunes, runes[i])
				i++
				for i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					if (runes[i] >= 'A' && runes[i] <= 'Z') || (runes[i] >= 'a' && runes[i] <= 'z') {
						i++
						break
					}
					i++
				}
				resultRunes = append(resultRunes, seqRunes...)
			} else if i < len(runes) && runes[i] == ']' {
				seqRunes = append(seqRunes, runes[i])
				i++
				for i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					if runes[i] == '\x07' {
						i++
						break
					}
					i++
				}
				resultRunes = append(resultRunes, seqRunes...)
			} else {
				if i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					i++
				}
				resultRunes = append(resultRunes, seqRunes...)
			}
			continue
		}

		if visible < targetVisible {
			resultRunes = append(resultRunes, runes[i])
			visible++
			i++
		} else {
			break
		}
	}

	return string(resultRunes) + truncChar
}

func WrapLineANSI(line string, width int) []string {
	if width <= 0 {
		return []string{line}
	}
	if line == "" {
		return []string{""}
	}

	runes := []rune(line)
	var result []string
	var currentLine []rune
	var pendingANSI [][]rune
	var activeSGR []rune
	visible := 0
	i := 0

	for i < len(runes) {
		// Check if we need to wrap before processing this character
		if visible >= width {
			// Don't add pending ANSI to current line - carry it forward
			if len(activeSGR) > 0 {
				currentLine = append(currentLine, []rune("\x1b[0m")...)
			}
			result = append(result, string(currentLine))
			currentLine = []rune{}
			if len(activeSGR) > 0 {
				currentLine = append(currentLine, activeSGR...)
			}
			// pendingANSI is carried forward but not added yet
			visible = 0
		}

		if runes[i] == '\x1b' {
			var seqRunes []rune
			seqRunes = append(seqRunes, runes[i])
			i++
			if i < len(runes) && runes[i] == '[' {
				seqRunes = append(seqRunes, runes[i])
				i++
				for i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					if (runes[i] >= 'A' && runes[i] <= 'Z') || (runes[i] >= 'a' && runes[i] <= 'z') {
						if runes[i] == 'm' {
							if len(seqRunes) == 3 && seqRunes[2] == '0' {
								activeSGR = nil
							} else {
								activeSGR = seqRunes
							}
						}
						i++
						break
					}
					i++
				}
			} else if i < len(runes) && runes[i] == ']' {
				seqRunes = append(seqRunes, runes[i])
				i++
				for i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					if runes[i] == '\x07' {
						i++
						break
					}
					i++
				}
			} else {
				if i < len(runes) {
					seqRunes = append(seqRunes, runes[i])
					i++
				}
			}
			pendingANSI = append(pendingANSI, seqRunes)
			continue
		}

		// This is a visible character - add pending ANSI first
		for _, seq := range pendingANSI {
			currentLine = append(currentLine, seq...)
		}
		pendingANSI = nil
		currentLine = append(currentLine, runes[i])
		visible++
		i++
	}

	if len(currentLine) > 0 || len(result) == 0 {
		for _, seq := range pendingANSI {
			currentLine = append(currentLine, seq...)
		}
		result = append(result, string(currentLine))
	}

	return result
}
