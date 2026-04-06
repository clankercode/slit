package main

import (
	"testing"
)

func TestTrimLine_short(t *testing.T) {
	got := TrimLine("hi", 10, "…")
	if got != "hi" {
		t.Errorf("TrimLine(%q, 10, …) = %q, want %q", "hi", got, "hi")
	}
}

func TestTrimLine_exact(t *testing.T) {
	got := TrimLine("hello", 5, "…")
	if got != "hello" {
		t.Errorf("TrimLine(%q, 5, …) = %q, want %q", "hello", got, "hello")
	}
}

func TestTrimLine_long(t *testing.T) {
	got := TrimLine("hello world", 8, "…")
	want := "hello w…"
	if got != want {
		t.Errorf("TrimLine(%q, 8, …) = %q, want %q", "hello world", got, want)
	}
}

func TestTrimLine_unicode(t *testing.T) {
	got := TrimLine("héllo wörld", 8, "…")
	want := "héllo w…"
	if got != want {
		t.Errorf("TrimLine(%q, 8, …) = %q, want %q", "héllo wörld", got, want)
	}
}

func TestWrapLine_short(t *testing.T) {
	got := WrapLine("hi", 10)
	if len(got) != 1 || got[0] != "hi" {
		t.Errorf("WrapLine(%q, 10) = %v, want [%q]", "hi", got, "hi")
	}
}

func TestWrapLine_exact(t *testing.T) {
	got := WrapLine("hello", 5)
	if len(got) != 1 || got[0] != "hello" {
		t.Errorf("WrapLine(%q, 5) = %v, want [%q]", "hello", got, "hello")
	}
}

func TestWrapLine_long(t *testing.T) {
	got := WrapLine("hello world foo", 6)
	want := []string{"hello ", "world ", "foo"}
	if len(got) != len(want) {
		t.Fatalf("WrapLine length = %d, want %d", len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("WrapLine[%d] = %q, want %q", i, got[i], want[i])
		}
	}
}

func TestWrapLine_empty(t *testing.T) {
	got := WrapLine("", 10)
	if len(got) != 1 || got[0] != "" {
		t.Errorf("WrapLine(%q, 10) = %v, want [%q]", "", got, "")
	}
}

func TestFormatLineWithNumber(t *testing.T) {
	got := FormatLineWithNumber("hello", 42, 4)
	want := "  42 hello"
	if got != want {
		t.Errorf("FormatLineWithNumber(%q, 42, 4) = %q, want %q", "hello", got, want)
	}
}

func TestFormatLineWithTimestamp(t *testing.T) {
	got := FormatLineWithTimestamp("hello", "12:34:56")
	want := "12:34:56 hello"
	if got != want {
		t.Errorf("FormatLineWithTimestamp(%q, %q) = %q, want %q", "hello", "12:34:56", got, want)
	}
}

func TestStripANSI(t *testing.T) {
	input := "\x1b[31mred\x1b[0m text"
	got := StripANSI(input)
	want := "red text"
	if got != want {
		t.Errorf("StripANSI(%q) = %q, want %q", input, got, want)
	}
}

func TestStripANSI_noAnsi(t *testing.T) {
	input := "plain text"
	got := StripANSI(input)
	if got != input {
		t.Errorf("StripANSI(%q) = %q, want %q", input, got, input)
	}
}

func TestVisibleWidth(t *testing.T) {
	got := VisibleWidth("\x1b[31mhello\x1b[0m")
	want := 5
	if got != want {
		t.Errorf("VisibleWidth() = %d, want %d", got, want)
	}
}

func TestTrimLineANSI_noAnsi(t *testing.T) {
	got := TrimLineANSI("hello world", 8, "…")
	want := "hello w…"
	if got != want {
		t.Errorf("TrimLineANSI(%q, 8, …) = %q, want %q", "hello world", got, want)
	}
}

func TestTrimLineANSI_withAnsi(t *testing.T) {
	input := "\x1b[31mhello world\x1b[0m"
	got := TrimLineANSI(input, 8, "…")
	if VisibleWidth(got) != 8 {
		t.Errorf("TrimLineANSI visible width = %d, want 8; got %q", VisibleWidth(got), got)
	}
	stripped := StripANSI(got)
	wantStripped := "hello w…"
	if stripped != wantStripped {
		t.Errorf("TrimLineANSI stripped = %q, want %q", stripped, wantStripped)
	}
}
