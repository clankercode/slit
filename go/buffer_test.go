package main

import (
	"testing"
)

func TestRingPushAndRetrieve(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("line1")
	rb.Push("line2")
	rb.Push("line3")
	lines := rb.Lines()
	if len(lines) != 3 {
		t.Fatalf("expected 3 lines, got %d", len(lines))
	}
	expected := []string{"line1", "line2", "line3"}
	for i, exp := range expected {
		if lines[i].Text != exp {
			t.Errorf("lines[%d] = %q, want %q", i, lines[i].Text, exp)
		}
	}
}

func TestRingOverflow(t *testing.T) {
	rb := NewRingBuffer(5)
	for i := 0; i < 7; i++ {
		rb.Push("line" + string(rune('0'+i)))
	}
	lines := rb.Lines()
	if len(lines) != 5 {
		t.Fatalf("expected 5 lines, got %d", len(lines))
	}
	expected := []string{"line2", "line3", "line4", "line5", "line6"}
	for i, exp := range expected {
		if lines[i].Text != exp {
			t.Errorf("lines[%d] = %q, want %q", i, lines[i].Text, exp)
		}
	}
}

func TestRingFIFOOrder(t *testing.T) {
	rb := NewRingBuffer(10)
	rb.Push("a")
	rb.Push("b")
	rb.Push("c")
	lines := rb.Lines()
	if lines[0].Text != "a" || lines[1].Text != "b" || lines[2].Text != "c" {
		t.Errorf("expected FIFO order [a b c], got %v", lines)
	}
}

func TestRingLast(t *testing.T) {
	rb := NewRingBuffer(10)
	for i := 0; i < 10; i++ {
		rb.Push(string(rune('a' + i)))
	}
	last := rb.Last(3)
	if len(last) != 3 {
		t.Fatalf("expected 3, got %d", len(last))
	}
	if last[0].Text != "h" || last[1].Text != "i" || last[2].Text != "j" {
		t.Errorf("expected [h i j], got %v", last)
	}
}

func TestRingLastGreaterThanCount(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("x")
	rb.Push("y")
	last := rb.Last(10)
	if len(last) != 2 {
		t.Fatalf("expected 2, got %d", len(last))
	}
	if last[0].Text != "x" || last[1].Text != "y" {
		t.Errorf("expected [x y], got %v", last)
	}
}

func TestRingLastZero(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("a")
	last := rb.Last(0)
	if len(last) != 0 {
		t.Errorf("expected empty, got %v", last)
	}
	last = rb.Last(-1)
	if len(last) != 0 {
		t.Errorf("expected empty for negative, got %v", last)
	}
}

func TestRingTotalCount(t *testing.T) {
	rb := NewRingBuffer(5)
	for i := 0; i < 10; i++ {
		rb.Push("line")
	}
	if rb.TotalCount() != 10 {
		t.Errorf("expected totalCount 10, got %d", rb.TotalCount())
	}
}

func TestRingTotalBytes(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("hello")
	rb.Push("world")
	expected := int64(len("hello") + len("world"))
	if rb.TotalBytes() != expected {
		t.Errorf("expected totalBytes %d, got %d", expected, rb.TotalBytes())
	}
}

func TestRingEmpty(t *testing.T) {
	rb := NewRingBuffer(5)
	if !rb.IsEmpty() {
		t.Error("expected IsEmpty true")
	}
	if rb.Len() != 0 {
		t.Errorf("expected Len 0, got %d", rb.Len())
	}
	lines := rb.Lines()
	if len(lines) != 0 {
		t.Errorf("expected empty slice, got %v", lines)
	}
}

func TestRingCapacityOne(t *testing.T) {
	rb := NewRingBuffer(1)
	rb.Push("first")
	rb.Push("second")
	if rb.Len() != 1 {
		t.Fatalf("expected Len 1, got %d", rb.Len())
	}
	lines := rb.Lines()
	if lines[0].Text != "second" {
		t.Errorf("expected 'second', got %q", lines[0].Text)
	}
	if rb.TotalCount() != 2 {
		t.Errorf("expected TotalCount 2, got %d", rb.TotalCount())
	}
	if rb.Cap() != 1 {
		t.Errorf("expected Cap 1, got %d", rb.Cap())
	}
}

func TestRingPanicZeroCapacity(t *testing.T) {
	defer func() {
		r := recover()
		if r == nil {
			t.Error("expected panic for capacity 0")
		}
	}()
	NewRingBuffer(0)
}

func TestRingPanicNegativeCapacity(t *testing.T) {
	defer func() {
		r := recover()
		if r == nil {
			t.Error("expected panic for negative capacity")
		}
	}()
	NewRingBuffer(-3)
}

func TestRingLineNum(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("a")
	rb.Push("b")
	rb.Push("c")
	lines := rb.Lines()
	if lines[0].LineNum != 1 {
		t.Errorf("expected LineNum 1, got %d", lines[0].LineNum)
	}
	if lines[1].LineNum != 2 {
		t.Errorf("expected LineNum 2, got %d", lines[1].LineNum)
	}
	if lines[2].LineNum != 3 {
		t.Errorf("expected LineNum 3, got %d", lines[2].LineNum)
	}
}

func TestRingLineNumOverflow(t *testing.T) {
	rb := NewRingBuffer(3)
	for i := 0; i < 5; i++ {
		rb.Push("x")
	}
	lines := rb.Lines()
	if lines[0].LineNum != 3 {
		t.Errorf("expected LineNum 3, got %d", lines[0].LineNum)
	}
	if lines[2].LineNum != 5 {
		t.Errorf("expected LineNum 5, got %d", lines[2].LineNum)
	}
}

func TestRingTimestamp(t *testing.T) {
	rb := NewRingBuffer(5)
	rb.Push("a")
	lines := rb.Lines()
	if lines[0].Time.IsZero() {
		t.Error("expected non-zero timestamp")
	}
}
