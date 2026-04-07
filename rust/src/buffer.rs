use std::collections::VecDeque;
use std::time::SystemTime;

pub struct LineEntry {
    pub text: String,
    pub time: SystemTime,
    pub line_num: u64,
}

pub struct RingBuffer {
    entries: VecDeque<LineEntry>,
    capacity: usize,
    total_count: u64,
    total_bytes: u64,
}

impl RingBuffer {
    pub fn new(capacity: usize) -> Self {
        assert!(capacity > 0, "ring buffer capacity must be > 0");
        Self {
            entries: VecDeque::with_capacity(capacity),
            capacity,
            total_count: 0,
            total_bytes: 0,
        }
    }

    pub fn push(&mut self, text: String) {
        let byte_len = text.len() as u64;
        if self.entries.len() == self.capacity {
            self.entries.pop_front();
        }
        self.total_bytes += byte_len;
        let entry = LineEntry {
            time: SystemTime::now(),
            line_num: self.total_count + 1,
            text,
        };
        self.entries.push_back(entry);
        self.total_count += 1;
    }

    pub fn last_entry(&self) -> Option<&LineEntry> {
        self.entries.back()
    }

    pub fn last_n(&self, n: usize) -> Vec<&LineEntry> {
        let skip = self.entries.len().saturating_sub(n);
        self.entries.iter().skip(skip).collect()
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn capacity(&self) -> usize {
        self.capacity
    }

    pub fn total_count(&self) -> u64 {
        self.total_count
    }

    pub fn total_bytes(&self) -> u64 {
        self.total_bytes
    }

    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_push_and_len() {
        let mut buf = RingBuffer::new(3);
        assert!(buf.is_empty());
        buf.push("line1".to_string());
        buf.push("line2".to_string());
        buf.push("line3".to_string());
        assert_eq!(buf.len(), 3);
        assert_eq!(buf.total_count(), 3);
    }

    #[test]
    fn test_eviction() {
        let mut buf = RingBuffer::new(2);
        buf.push("a".to_string());
        buf.push("b".to_string());
        buf.push("c".to_string());
        assert_eq!(buf.len(), 2);
        assert_eq!(buf.total_count(), 3);
        let last = buf.last_n(2);
        assert_eq!(last[0].text, "b");
        assert_eq!(last[1].text, "c");
    }

    #[test]
    fn test_total_bytes() {
        let mut buf = RingBuffer::new(10);
        buf.push("hello".to_string());
        buf.push("world".to_string());
        assert_eq!(buf.total_bytes(), 10);
        buf.push("x".to_string());
        assert_eq!(buf.total_bytes(), 11);
    }

    #[test]
    fn test_total_bytes_cumulative_after_eviction() {
        let mut buf = RingBuffer::new(3);
        buf.push("short".to_string()); // 5, cumulative=5
        assert_eq!(buf.total_bytes(), 5);
        buf.push("a very long line here".to_string()); // 21, cumulative=26
        assert_eq!(buf.total_bytes(), 26);
        buf.push("x".to_string()); // 1, cumulative=27
        assert_eq!(buf.total_bytes(), 27);
        buf.push("yy".to_string()); // 2, evicts "short", cumulative=29
        assert_eq!(buf.total_bytes(), 29);
    }

    #[test]
    fn test_last_n() {
        let mut buf = RingBuffer::new(100);
        for i in 0..10 {
            buf.push(format!("line{}", i));
        }
        let last3 = buf.last_n(3);
        assert_eq!(last3.len(), 3);
        assert_eq!(last3[0].text, "line7");
        assert_eq!(last3[1].text, "line8");
        assert_eq!(last3[2].text, "line9");
    }

    #[test]
    fn test_line_numbers() {
        let mut buf = RingBuffer::new(2);
        buf.push("a".to_string());
        buf.push("b".to_string());
        buf.push("c".to_string());
        let last = buf.last_n(2);
        assert_eq!(last[0].line_num, 2);
        assert_eq!(last[1].line_num, 3);
    }

    #[test]
    #[should_panic]
    fn test_zero_capacity_panics() {
        RingBuffer::new(0);
    }
}
