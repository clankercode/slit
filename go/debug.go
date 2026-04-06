package main

import (
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"time"
)

type DebugLogger struct {
	mu   sync.Mutex
	file *os.File
}

func NewDebugLogger(path string) *DebugLogger {
	if path == "" {
		path = filepath.Join(os.TempDir(), fmt.Sprintf("slit-%d.log", os.Getpid()))
	}

	f, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		return nil
	}

	return &DebugLogger{file: f}
}

func (dl *DebugLogger) Log(format string, args ...interface{}) {
	if dl == nil || dl.file == nil {
		return
	}
	dl.mu.Lock()
	defer dl.mu.Unlock()
	ts := time.Now().Format("15:04:05.000")
	msg := fmt.Sprintf(format, args...)
	fmt.Fprintf(dl.file, "%s %s\n", ts, msg)
}

func (dl *DebugLogger) Close() {
	if dl == nil || dl.file == nil {
		return
	}
	dl.file.Close()
	dl.file = nil
}
