package main

import (
	"fmt"
	"strings"
	"unicode/utf8"

	"github.com/charmbracelet/lipgloss"
)

type Layout struct {
	Name        string
	TopLines    int
	BottomLines int
	SideWidth   int
}

var Layouts = map[string]Layout{
	"box":     {Name: "box", TopLines: 1, BottomLines: 1, SideWidth: 4},
	"rounded": {Name: "rounded", TopLines: 1, BottomLines: 1, SideWidth: 4},
	"compact": {Name: "compact", TopLines: 1, BottomLines: 1, SideWidth: 0},
	"minimal": {Name: "minimal", TopLines: 0, BottomLines: 1, SideWidth: 0},
	"none":    {Name: "none", TopLines: 0, BottomLines: 0, SideWidth: 0},
	"quote":   {Name: "quote", TopLines: 0, BottomLines: 0, SideWidth: 2},
}

func GetLayout(name string) Layout {
	l, ok := Layouts[name]
	if !ok {
		panic(fmt.Sprintf("unknown layout: %q", name))
	}
	return l
}

func padRight(s string, width int) string {
	vis := VisibleWidth(s)
	if vis > width {
		return TrimLineANSI(s, width, "")
	}
	return s + strings.Repeat(" ", width-vis)
}

func RenderLayout(layout Layout, title string, content string, status string, width int) string {
	switch layout.Name {
	case "box":
		return renderBox(title, content, status, width, "┌", "┐", "└", "┘", "─", "│")
	case "rounded":
		return renderBox(title, content, status, width, "╭", "╮", "╰", "╯", "─", "│")
	case "compact":
		return renderCompact(title, content, status, width)
	case "minimal":
		return renderMinimal(content, status)
	case "none":
		return content
	case "quote":
		return renderQuote(title, content, status)
	default:
		return content
	}
}

func renderBox(title string, content string, status string, width int, tl string, tr string, bl string, br string, h string, v string) string {
	contentWidth := width - 4
	var sb strings.Builder

	titlePart := ""
	if title != "" {
		titlePart = " " + title + " "
	}
	horizInner := width - 2
	remaining := horizInner - utf8.RuneCountInString(titlePart)
	if remaining < 0 {
		remaining = 0
	}
	sb.WriteString(tl + titlePart + strings.Repeat(h, remaining) + tr + "\n")

	lines := strings.Split(content, "\n")
	for _, line := range lines {
		padded := padRight(line, contentWidth)
		sb.WriteString(v + " " + padded + " " + v + "\n")
	}

	statusPart := ""
	if status != "" {
		statusPart = " " + status + " "
	}
	remaining = horizInner - utf8.RuneCountInString(statusPart)
	if remaining < 0 {
		remaining = 0
	}
	sb.WriteString(bl + statusPart + strings.Repeat(h, remaining) + br)

	return sb.String()
}

func renderCompact(title string, content string, status string, width int) string {
	topBar := lipgloss.NewStyle().
		Background(lipgloss.Color("#4B4B4B")).
		Foreground(lipgloss.Color("#E0E0E0")).
		Bold(true).
		Width(width).
		Render(title)

	return topBar + "\n" + content + "\n" + status
}

func renderMinimal(content string, status string) string {
	return content + "\n" + status
}

func renderQuote(title string, content string, status string) string {
	var sb strings.Builder
	sb.WriteString("▌ " + title + "\n")
	lines := strings.Split(content, "\n")
	for _, line := range lines {
		sb.WriteString("▌ " + line + "\n")
	}
	sb.WriteString("▌ " + status)
	return sb.String()
}
