package main

import (
	"testing"
)

func TestGetSpinnerFrameBraille(t *testing.T) {
	frames := SpinnerFrames["braille"]
	if len(frames) != 10 {
		t.Fatalf("expected 10 braille frames, got %d", len(frames))
	}
	f := GetSpinnerFrame("braille", 0)
	if f != "⠋" {
		t.Errorf("expected first braille frame '⠋', got %q", f)
	}
	f = GetSpinnerFrame("braille", 1)
	if f != "⠙" {
		t.Errorf("expected second braille frame '⠙', got %q", f)
	}
}

func TestGetSpinnerFrameDots(t *testing.T) {
	frames := SpinnerFrames["dots"]
	if len(frames) != 8 {
		t.Fatalf("expected 8 dots frames, got %d", len(frames))
	}
}

func TestGetSpinnerFrameArrows(t *testing.T) {
	frames := SpinnerFrames["arrows"]
	if len(frames) != 8 {
		t.Fatalf("expected 8 arrows frames, got %d", len(frames))
	}
}

func TestGetSpinnerFrameOff(t *testing.T) {
	frames := SpinnerFrames["off"]
	if len(frames) != 0 {
		t.Fatalf("expected 0 off frames, got %d", len(frames))
	}
	f := GetSpinnerFrame("off", 0)
	if f != "" {
		t.Errorf("expected empty string for off, got %q", f)
	}
}

func TestGetSpinnerFrameWraps(t *testing.T) {
	frames := SpinnerFrames["braille"]
	f := GetSpinnerFrame("braille", len(frames))
	if f != frames[0] {
		t.Errorf("expected wrap to first frame, got %q", f)
	}
}

func TestGetSpinnerFrameUnknown(t *testing.T) {
	f := GetSpinnerFrame("nonexistent", 0)
	if f != "" {
		t.Errorf("expected empty string for unknown, got %q", f)
	}
}

func TestFormatStatusLineStreaming(t *testing.T) {
	line := FormatStatusLine("braille", 0, false, 42, 1024, 0, 80)
	if len(line) == 0 {
		t.Error("expected non-empty status line")
	}
	if len([]rune(line)) > 80 {
		t.Errorf("status line exceeds width: %d runes", len([]rune(line)))
	}
}

func TestFormatStatusLineDone(t *testing.T) {
	line := FormatStatusLine("braille", 0, true, 42, 1024, 0, 80)
	if len(line) == 0 {
		t.Error("expected non-empty status line")
	}
}

func TestFormatStatusLineProgress(t *testing.T) {
	line := FormatStatusLine("braille", 0, false, 42, 512, 1024, 80)
	if len(line) == 0 {
		t.Error("expected non-empty status line")
	}
}

func TestFormatStatusLineProgressFull(t *testing.T) {
	line := FormatStatusLine("braille", 0, true, 42, 1024, 1024, 80)
	if len(line) == 0 {
		t.Error("expected non-empty status line")
	}
}

func TestFormatStatusLineZeroWidth(t *testing.T) {
	line := FormatStatusLine("braille", 0, false, 42, 1024, 0, 0)
	if len(line) == 0 {
		t.Error("expected non-empty status line even with zero width")
	}
}

func TestFormatStatusLineNarrowWidth(t *testing.T) {
	line := FormatStatusLine("braille", 0, false, 42, 1024, 0, 10)
	runes := []rune(line)
	if len(runes) > 10 {
		t.Errorf("expected max 10 runes, got %d: %q", len(runes), line)
	}
}

func TestFormatStatusLineSpinnerOff(t *testing.T) {
	line := FormatStatusLine("off", 0, false, 42, 1024, 0, 80)
	if len(line) == 0 {
		t.Error("expected non-empty status line")
	}
}
