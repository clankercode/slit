package main

import (
	"fmt"
	"strings"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

type lineMsg string

type eofMsg struct{}

type model struct {
	cfg          *Config
	buf          *RingBuffer
	width        int
	height       int
	contentLines int
	eof          bool
}

func (m model) Init() tea.Cmd {
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
		if m.cfg.Lines <= 0 {
			m.contentLines = msg.Height - 1
		} else {
			m.contentLines = m.cfg.Lines
		}
		if m.contentLines < 1 {
			m.contentLines = 1
		}
		return m, nil
	case lineMsg:
		m.buf.Push(string(msg))
		return m, nil
	case eofMsg:
		m.eof = true
		return m, tea.Quit
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			return m, tea.Quit
		}
	}
	return m, nil
}

func (m model) View() string {
	if m.width == 0 {
		return ""
	}

	lines := m.buf.Last(m.contentLines)
	content := strings.Join(lines, "\n")

	hintStyle := lipgloss.NewStyle().Faint(true)

	var status string
	if m.eof {
		status = fmt.Sprintf("Done. (%d lines)  %s", m.buf.Len(), hintStyle.Render("q:quit"))
	} else {
		status = fmt.Sprintf("Streaming... (%d lines)  %s", m.buf.Len(), hintStyle.Render("q:quit"))
	}

	if content == "" {
		return status
	}
	return content + "\n" + status
}
