package main

type RingBuffer struct {
	lines      []string
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
		lines:    make([]string, capacity),
		capacity: capacity,
	}
}

func (rb *RingBuffer) Push(line string) {
	rb.lines[rb.head] = line
	rb.head = (rb.head + 1) % rb.capacity
	if rb.count < rb.capacity {
		rb.count++
	}
	rb.totalCount++
	rb.totalBytes += int64(len(line))
}

func (rb *RingBuffer) Lines() []string {
	if rb.count == 0 {
		return []string{}
	}
	result := make([]string, rb.count)
	for i := 0; i < rb.count; i++ {
		result[i] = rb.lines[(rb.head+rb.capacity-rb.count+i)%rb.capacity]
	}
	return result
}

func (rb *RingBuffer) Last(n int) []string {
	if n <= 0 {
		return []string{}
	}
	if n > rb.count {
		n = rb.count
	}
	result := make([]string, n)
	for i := 0; i < n; i++ {
		result[i] = rb.lines[(rb.head+rb.capacity-n+i)%rb.capacity]
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
