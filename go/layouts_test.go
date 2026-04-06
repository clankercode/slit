package main

import (
	"strings"
	"testing"
)

func TestGetLayoutAll(t *testing.T) {
	names := []string{"box", "rounded", "compact", "minimal", "none", "quote"}
	for _, name := range names {
		l := GetLayout(name)
		if l.Name != name {
			t.Errorf("expected name %q, got %q", name, l.Name)
		}
	}
}

func TestGetLayoutPanics(t *testing.T) {
	defer func() {
		r := recover()
		if r == nil {
			t.Error("expected panic for unknown layout")
		}
	}()
	GetLayout("nonexistent")
}

func TestLayoutChromeCosts(t *testing.T) {
	tests := []struct {
		name      string
		topLines  int
		botLines  int
		sideWidth int
	}{
		{"box", 1, 1, 4},
		{"rounded", 1, 1, 4},
		{"compact", 1, 1, 0},
		{"minimal", 0, 1, 0},
		{"none", 0, 0, 0},
		{"quote", 0, 0, 2},
	}
	for _, tt := range tests {
		l := GetLayout(tt.name)
		if l.TopLines != tt.topLines {
			t.Errorf("%s: expected topLines %d, got %d", tt.name, tt.topLines, l.TopLines)
		}
		if l.BottomLines != tt.botLines {
			t.Errorf("%s: expected bottomLines %d, got %d", tt.name, tt.botLines, l.BottomLines)
		}
		if l.SideWidth != tt.sideWidth {
			t.Errorf("%s: expected sideWidth %d, got %d", tt.name, tt.sideWidth, l.SideWidth)
		}
	}
}

func TestRenderLayoutBox(t *testing.T) {
	layout := GetLayout("box")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "┌") || !strings.Contains(result, "┐") {
		t.Errorf("box layout missing corners: %q", result)
	}
	if !strings.Contains(result, "└") || !strings.Contains(result, "┘") {
		t.Errorf("box layout missing bottom corners: %q", result)
	}
	if !strings.Contains(result, "hello") {
		t.Errorf("box layout missing content: %q", result)
	}
	if !strings.Contains(result, "slit") {
		t.Errorf("box layout missing title: %q", result)
	}
}

func TestRenderLayoutRounded(t *testing.T) {
	layout := GetLayout("rounded")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "╭") || !strings.Contains(result, "╮") {
		t.Errorf("rounded layout missing corners: %q", result)
	}
	if !strings.Contains(result, "╰") || !strings.Contains(result, "╯") {
		t.Errorf("rounded layout missing bottom corners: %q", result)
	}
}

func TestRenderLayoutMinimal(t *testing.T) {
	layout := GetLayout("minimal")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "hello") {
		t.Errorf("minimal layout missing content: %q", result)
	}
	if !strings.Contains(result, "status") {
		t.Errorf("minimal layout missing status: %q", result)
	}
	if strings.Contains(result, "┌") || strings.Contains(result, "╭") {
		t.Errorf("minimal layout should not have borders: %q", result)
	}
}

func TestRenderLayoutNone(t *testing.T) {
	layout := GetLayout("none")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "hello") {
		t.Errorf("none layout missing content: %q", result)
	}
	if strings.Contains(result, "status") {
		t.Errorf("none layout should not show status: %q", result)
	}
}

func TestRenderLayoutQuote(t *testing.T) {
	layout := GetLayout("quote")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "▌") {
		t.Errorf("quote layout missing marker: %q", result)
	}
	if !strings.Contains(result, "hello") {
		t.Errorf("quote layout missing content: %q", result)
	}
}

func TestRenderLayoutCompact(t *testing.T) {
	layout := GetLayout("compact")
	result := RenderLayout(layout, "slit", "hello", "status", 40)
	if !strings.Contains(result, "hello") {
		t.Errorf("compact layout missing content: %q", result)
	}
	if !strings.Contains(result, "status") {
		t.Errorf("compact layout missing status: %q", result)
	}
}

func TestRenderLayoutBoxWidthAlignment(t *testing.T) {
	layout := GetLayout("box")
	result := RenderLayout(layout, "slit", "content", "status", 40)
	lines := strings.Split(result, "\n")
	for _, line := range lines {
		if line == "" {
			continue
		}
		runeCount := len([]rune(line))
		if runeCount != 40 {
			t.Errorf("expected all lines to be 40 runes, got %d: %q", runeCount, line)
		}
	}
}

func TestPadRight(t *testing.T) {
	result := padRight("hi", 5)
	if result != "hi   " {
		t.Errorf("expected 'hi   ', got %q", result)
	}
}

func TestPadRightOverflow(t *testing.T) {
	result := padRight("hello world", 5)
	if result != "hello" {
		t.Errorf("expected 'hello', got %q", result)
	}
}

func TestPadRightExact(t *testing.T) {
	result := padRight("hello", 5)
	if result != "hello" {
		t.Errorf("expected 'hello', got %q", result)
	}
}
