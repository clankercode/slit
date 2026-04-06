package main

import (
	"os"
)

type TeeWriter struct {
	file   *os.File
	path   string
	append bool
}

func NewTeeWriter(path string, append bool) *TeeWriter {
	flags := os.O_CREATE | os.O_WRONLY
	if append {
		flags |= os.O_APPEND
	} else {
		flags |= os.O_TRUNC
	}

	f, err := os.OpenFile(path, flags, 0644)
	if err != nil {
		return nil
	}

	return &TeeWriter{
		file:   f,
		path:   path,
		append: append,
	}
}

func (tw *TeeWriter) WriteLine(line string) {
	if tw == nil || tw.file == nil {
		return
	}
	tw.file.WriteString(line + "\n")
}

func (tw *TeeWriter) Close() {
	if tw == nil || tw.file == nil {
		return
	}
	tw.file.Close()
	tw.file = nil
}
