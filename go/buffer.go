package main

import (
	"time"
)

type LineEntry struct {
	Text    string
	Time    time.Time
	LineNum int
}

type RingBuffer struct {
	entries    []LineEntry
	capacity   int
	head       int
	count      int
	totalCount int
	totalBytes int64
}

func NewRingBuffer(capacity int) *RingBuffer {
	if capacity <= 0 {
		panic("ring buffer capacity must be > 0")
	}
	return &RingBuffer{
		entries:  make([]LineEntry, capacity),
		capacity: capacity,
	}
}

func (rb *RingBuffer) Push(text string) {
	entry := LineEntry{
		Text:    text,
		Time:    time.Now(),
		LineNum: rb.totalCount + 1,
	}
	rb.entries[rb.head] = entry
	rb.head = (rb.head + 1) % rb.capacity
	if rb.count < rb.capacity {
		rb.count++
	}
	rb.totalCount++
	rb.totalBytes += int64(len(text))
}

func (rb *RingBuffer) Lines() []LineEntry {
	if rb.count == 0 {
		return []LineEntry{}
	}
	result := make([]LineEntry, rb.count)
	for i := 0; i < rb.count; i++ {
		result[i] = rb.entries[(rb.head+rb.capacity-rb.count+i)%rb.capacity]
	}
	return result
}

func (rb *RingBuffer) Last(n int) []LineEntry {
	if n <= 0 {
		return []LineEntry{}
	}
	if n > rb.count {
		n = rb.count
	}
	result := make([]LineEntry, n)
	for i := 0; i < n; i++ {
		result[i] = rb.entries[(rb.head+rb.capacity-n+i)%rb.capacity]
	}
	return result
}

func (rb *RingBuffer) Len() int {
	return rb.count
}

func (rb *RingBuffer) Cap() int {
	return rb.capacity
}

func (rb *RingBuffer) TotalCount() int {
	return rb.totalCount
}

func (rb *RingBuffer) TotalBytes() int64 {
	return rb.totalBytes
}

func (rb *RingBuffer) IsEmpty() bool {
	return rb.count == 0
}
