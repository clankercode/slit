package main

import (
	"regexp"

	"github.com/mattn/go-runewidth"
)

var ansiCSI = regexp.MustCompile(`\x1b\[[0-9;]*[a-zA-Z]`)
var ansiOSC = regexp.MustCompile(`\x1b\].*?\x07`)

func TrimLine(line string, width int, truncChar string) string {
	if runewidth.StringWidth(line) <= width {
		return line
	}
	targetWidth := width - runewidth.StringWidth(truncChar)
	runes := []rune(line)
	vis := 0
	end := 0
	for end < len(runes) && vis+runewidth.RuneWidth(runes[end]) <= targetWidth {
		vis += runewidth.RuneWidth(runes[end])
		end++
	}
	return string(runes[:end]) + truncChar
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
	i := 0
	for i < len(runes) {
		vis := 0
		start := i
		for i < len(runes) && vis+runewidth.RuneWidth(runes[i]) <= width {
			vis += runewidth.RuneWidth(runes[i])
			i++
		}
		if i == start {
			i++
		}
		result = append(result, string(runes[start:i]))
	}
	return result
}

func StripANSI(s string) string {
	s = ansiCSI.ReplaceAllString(s, "")
	s = ansiOSC.ReplaceAllString(s, "")
	return s
}

func VisibleWidth(s string) int {
	return runewidth.StringWidth(StripANSI(s))
}

func TrimLineANSI(line string, width int, truncChar string) string {
	if VisibleWidth(line) <= width {
		return line
	}
	targetVisible := width - runewidth.StringWidth(truncChar)

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
			visible += runewidth.RuneWidth(runes[i])
			i++
		} else {
			break
		}
	}

	for {
		n := len(resultRunes)
		if n == 0 {
			break
		}
		last := resultRunes[n-1]
		isTerm := (last >= 'A' && last <= 'Z') || (last >= 'a' && last <= 'z') || last == '\x07'
		if !isTerm {
			break
		}
		j := n - 1
		for j >= 0 && resultRunes[j] != '\x1b' {
			j--
		}
		if j < 0 {
			break
		}
		seq := resultRunes[j:n]
		valid := false
		if len(seq) >= 3 && seq[1] == '[' {
			allParams := true
			for _, r := range seq[2 : len(seq)-1] {
				if !((r >= '0' && r <= '9') || r == ';') {
					allParams = false
					break
				}
			}
			if allParams {
				valid = true
			}
		} else if len(seq) >= 3 && seq[1] == ']' && last == '\x07' {
			valid = true
		}
		if !valid {
			break
		}
		resultRunes = resultRunes[:j]
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
							if len(seqRunes) == 4 && seqRunes[2] == '0' {
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

		if visible >= width {
			if len(activeSGR) > 0 {
				currentLine = append(currentLine, []rune("\x1b[0m")...)
			}
			result = append(result, string(currentLine))
			currentLine = []rune{}
			if len(activeSGR) > 0 {
				currentLine = append(currentLine, activeSGR...)
			}
			visible = 0
		}

		for _, seq := range pendingANSI {
			currentLine = append(currentLine, seq...)
		}
		pendingANSI = nil
		currentLine = append(currentLine, runes[i])
		visible += runewidth.RuneWidth(runes[i])
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
