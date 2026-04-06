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

func TestWrapLineANSI_noAnsi(t *testing.T) {
	got := WrapLineANSI("hello world", 5)
	want := []string{"hello", " worl", "d"}
	if len(got) != len(want) {
		t.Fatalf("WrapLineANSI length = %d, want %d", len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("WrapLineANSI[%d] = %q, want %q", i, got[i], want[i])
		}
	}
}

func TestWrapLineANSI_withAnsi(t *testing.T) {
	input := "\x1b[31mhello world\x1b[0m"
	got := WrapLineANSI(input, 5)
	want := []string{"\x1b[31mhello\x1b[0m", "\x1b[31m worl\x1b[0m", "\x1b[31md\x1b[0m"}
	if len(got) != len(want) {
		t.Fatalf("WrapLineANSI length = %d, want %d", len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("WrapLineANSI[%d] = %q, want %q", i, got[i], want[i])
		}
	}
}

func TestWrapLineANSI_carryColor(t *testing.T) {
	input := "ab\x1b[31mcdef"
	got := WrapLineANSI(input, 3)
	// ANSI sequences are part of the input, so they're included with the visible char that follows
	// The color code appears before 'c', so both get added to the same line
	want := []string{"ab\x1b[31mc\x1b[0m", "\x1b[31mdef"}
	if len(got) != len(want) {
		t.Fatalf("WrapLineANSI length = %d, want %d; got %v", len(got), len(want), got)
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("WrapLineANSI[%d] = %q, want %q", i, got[i], want[i])
		}
	}
}

func TestWrapLineANSI_empty(t *testing.T) {
	got := WrapLineANSI("", 10)
	if len(got) != 1 || got[0] != "" {
		t.Errorf("WrapLineANSI(%q, 10) = %v, want [%q]", "", got, "")
	}
}

func TestWrapLineANSI_zeroWidth(t *testing.T) {
	got := WrapLineANSI("hello", 0)
	if len(got) != 1 || got[0] != "hello" {
		t.Errorf("WrapLineANSI(%q, 0) = %v, want [%q]", "hello", got, "hello")
	}
}

func TestTrimLine_widthOne(t *testing.T) {
	got := TrimLine("hello", 1, "…")
	if got != "…" {
		t.Errorf("TrimLine(%q, 1, …) = %q, want %q", "hello", got, "…")
	}
}

func TestTrimLine_customTruncChar(t *testing.T) {
	got := TrimLine("hello world", 8, ">")
	want := "hello w>"
	if got != want {
		t.Errorf("TrimLine with '>' = %q, want %q", got, want)
	}
}

func TestTrimLine_multiByteTruncChar(t *testing.T) {
	got := TrimLine("hello world", 8, ">>")
	want := "hello >>"
	if got != want {
		t.Errorf("TrimLine with '>>' = %q, want %q", got, want)
	}
}

func TestStripANSI_multipleCodes(t *testing.T) {
	input := "\x1b[1;31mbold red\x1b[0m \x1b[32mgreen\x1b[0m"
	got := StripANSI(input)
	want := "bold red green"
	if got != want {
		t.Errorf("StripANSI(%q) = %q, want %q", input, got, want)
	}
}

func TestStripANSI_oscSequence(t *testing.T) {
	input := "\x1b]0;title\x07content"
	got := StripANSI(input)
	want := "content"
	if got != want {
		t.Errorf("StripANSI(%q) = %q, want %q", input, got, want)
	}
}

func TestVisibleWidth_plain(t *testing.T) {
	got := VisibleWidth("hello world")
	if got != 11 {
		t.Errorf("VisibleWidth('hello world') = %d, want 11", got)
	}
}

func TestVisibleWidth_unicode(t *testing.T) {
	got := VisibleWidth("hello 世界")
	if got != 8 {
		t.Errorf("VisibleWidth('hello 世界') = %d, want 8", got)
	}
}

func TestWrapLine_exactSplit(t *testing.T) {
	got := WrapLine("abcdefghij", 5)
	want := []string{"abcde", "fghij"}
	if len(got) != len(want) {
		t.Fatalf("WrapLine length = %d, want %d", len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Errorf("WrapLine[%d] = %q, want %q", i, got[i], want[i])
		}
	}
}

func TestTrimLineANSI_preservesColor(t *testing.T) {
	input := "\x1b[31mhello\x1b[0m"
	got := TrimLineANSI(input, 5, "…")
	if got != input {
		t.Errorf("TrimLineANSI should not truncate: got %q, want %q", got, input)
	}
}

func TestTrimLineANSI_truncatesWithColor(t *testing.T) {
	input := "\x1b[31mhello world\x1b[0m"
	got := TrimLineANSI(input, 5, "…")
	stripped := StripANSI(got)
	if stripped != "hell…" {
		t.Errorf("TrimLineANSI stripped = %q, want 'hell…'", stripped)
	}
	if VisibleWidth(got) != 5 {
		t.Errorf("TrimLineANSI visible width = %d, want 5; got %q", VisibleWidth(got), got)
	}
}

func TestWrapLineANSI_multipleColors(t *testing.T) {
	input := "\x1b[31mred\x1b[0m\x1b[32mgreen\x1b[0m\x1b[34mblue\x1b[0m"
	got := WrapLineANSI(input, 5)
	for _, line := range got {
		_ = line
	}
	if len(got) < 2 {
		t.Errorf("expected multiple wrapped lines, got %d", len(got))
	}
}
