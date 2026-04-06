package main

import (
	"fmt"
	"strings"
	"testing"
)

func BenchmarkRingBufferPush(b *testing.B) {
	rb := NewRingBuffer(50000)
	line := "this is a typical log line with some content"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		rb.Push(line)
	}
}

func BenchmarkRingBufferPushEvict(b *testing.B) {
	rb := NewRingBuffer(1000)
	line := "this is a typical log line with some content"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		rb.Push(line)
	}
}

func BenchmarkRingBufferLast(b *testing.B) {
	rb := NewRingBuffer(50000)
	for i := 0; i < 50000; i++ {
		rb.Push(fmt.Sprintf("line %d with some content", i))
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		rb.Last(20)
	}
}

func BenchmarkRingBufferLines(b *testing.B) {
	rb := NewRingBuffer(50000)
	for i := 0; i < 50000; i++ {
		rb.Push(fmt.Sprintf("line %d with some content", i))
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		rb.Lines()
	}
}

func BenchmarkTrimLine(b *testing.B) {
	line := strings.Repeat("x", 200)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		TrimLine(line, 80, "…")
	}
}

func BenchmarkTrimLineShort(b *testing.B) {
	line := "short line"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		TrimLine(line, 80, "…")
	}
}

func BenchmarkTrimLineANSI(b *testing.B) {
	line := "\x1b[31m" + strings.Repeat("x", 200) + "\x1b[0m"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		TrimLineANSI(line, 80, "…")
	}
}

func BenchmarkWrapLineANSI(b *testing.B) {
	line := strings.Repeat("x", 200)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		WrapLineANSI(line, 80)
	}
}

func BenchmarkWrapLineANSIColor(b *testing.B) {
	line := "\x1b[31m" + strings.Repeat("x", 200) + "\x1b[0m"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		WrapLineANSI(line, 80)
	}
}

func BenchmarkStripANSI(b *testing.B) {
	line := "\x1b[31mred\x1b[0m \x1b[32mgreen\x1b[0m \x1b[34mblue\x1b[0m normal text here"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		StripANSI(line)
	}
}

func BenchmarkStripANSIHeavy(b *testing.B) {
	parts := make([]string, 50)
	for i := range parts {
		parts[i] = fmt.Sprintf("\x1b[%dmtext%d\x1b[0m", 31+i%7, i)
	}
	line := strings.Join(parts, " ")
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		StripANSI(line)
	}
}

func BenchmarkVisibleWidth(b *testing.B) {
	line := "\x1b[31mhello world\x1b[0m some more text"
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		VisibleWidth(line)
	}
}

func BenchmarkFormatStatusLine(b *testing.B) {
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		FormatStatusLine("braille", i%10, false, 142, 4096, 8192, 80)
	}
}

func BenchmarkFormatStatusLineEOF(b *testing.B) {
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		FormatStatusLine("braille", 0, true, 142, 8192, 8192, 80)
	}
}

func BenchmarkRenderLayoutMinimal(b *testing.B) {
	layout := GetLayout("minimal")
	lines := make([]string, 20)
	for i := range lines {
		lines[i] = fmt.Sprintf("line %d: some content here", i)
	}
	content := strings.Join(lines, "\n")
	status := FormatStatusLine("braille", 0, false, 20, 1024, 0, 80)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		RenderLayout(layout, "slit", content, status, 80)
	}
}

func BenchmarkRenderLayoutBox(b *testing.B) {
	layout := GetLayout("box")
	lines := make([]string, 20)
	for i := range lines {
		lines[i] = fmt.Sprintf("line %d: some content here", i)
	}
	content := strings.Join(lines, "\n")
	status := FormatStatusLine("braille", 0, false, 20, 1024, 0, 76)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		RenderLayout(layout, "slit", content, status, 80)
	}
}

func BenchmarkFullRenderCycle(b *testing.B) {
	cfg := DefaultConfig()
	cfg.Lines = 20
	cfg.Layout = "minimal"
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 20
	m.isStderrTTY = true

	for i := 0; i < 100; i++ {
		m.buf.Push(fmt.Sprintf("line %d with typical content length", i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		m.View()
	}
}

func BenchmarkFullRenderCycleBox(b *testing.B) {
	cfg := DefaultConfig()
	cfg.Lines = 20
	cfg.Layout = "box"
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 20
	m.isStderrTTY = true

	for i := 0; i < 100; i++ {
		m.buf.Push(fmt.Sprintf("line %d with typical content length", i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		m.View()
	}
}

func BenchmarkFullRenderCycleWithANSI(b *testing.B) {
	cfg := DefaultConfig()
	cfg.Lines = 20
	cfg.Layout = "minimal"
	cfg.Color = "always"
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 20
	m.isStderrTTY = true

	for i := 0; i < 100; i++ {
		m.buf.Push(fmt.Sprintf("\x1b[3%dmline %d with colored content\x1b[0m", i%7+1, i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		m.View()
	}
}

func BenchmarkFullRenderCycleWrap(b *testing.B) {
	cfg := DefaultConfig()
	cfg.Lines = 20
	cfg.Layout = "minimal"
	cfg.Wrap = true
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 20
	m.isStderrTTY = true

	longLine := strings.Repeat("abcdefghij ", 20)
	for i := 0; i < 100; i++ {
		m.buf.Push(longLine)
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		m.View()
	}
}

func BenchmarkFullRenderCycleLineNumbers(b *testing.B) {
	cfg := DefaultConfig()
	cfg.Lines = 20
	cfg.Layout = "minimal"
	cfg.LineNumbers = true
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 20
	m.isStderrTTY = true

	for i := 0; i < 100; i++ {
		m.buf.Push(fmt.Sprintf("line %d with typical content", i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		m.View()
	}
}
