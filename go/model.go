package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"golang.org/x/term"
)

type lineMsg string

type eofMsg struct{}

type spinnerTickMsg time.Time

type titleSetMsg struct{}

type model struct {
	cfg          *Config
	buf          *RingBuffer
	pendingLines []LineEntry
	width        int
	height       int
	contentLines int
	eof          bool
	spinnerFrame int
	spinnerTick  int
	fileSize     int64
	isStderrTTY  bool
	teeWriter    *TeeWriter
	debugLogger  *DebugLogger
	titleSet     bool
}

func newModel(cfg *Config) model {
	fileSize := int64(0)
	if info, err := os.Stdin.Stat(); err == nil {
		if (info.Mode() & os.ModeType) == 0 {
			fileSize = info.Size()
		}
	}

	isStderrTTY := term.IsTerminal(int(os.Stderr.Fd()))
	forceRender := os.Getenv("SLIT_FORCE_RENDER") == "1"
	if forceRender {
		isStderrTTY = true
	}

	var tw *TeeWriter
	if cfg.Output != "" {
		tw = NewTeeWriter(cfg.Output, cfg.Append)
	}

	var dl *DebugLogger
	if cfg.Debug {
		dl = NewDebugLogger(cfg.LogFile)
	}

	return model{
		cfg:         cfg,
		buf:         NewRingBuffer(cfg.MaxLines),
		fileSize:    fileSize,
		isStderrTTY: isStderrTTY,
		teeWriter:   tw,
		debugLogger: dl,
	}
}

func setWindowTitleCmd(title string) tea.Cmd {
	return func() tea.Msg {
		os.Stderr.WriteString(fmt.Sprintf("\x1b]0;%s\x07", title))
		return titleSetMsg{}
	}
}

func (m model) Init() tea.Cmd {
	cmds := []tea.Cmd{tickSpinner()}
	if m.isStderrTTY && m.cfg.Layout != "none" {
		cmds = append(cmds, setWindowTitleCmd("slit"))
	}
	if m.debugLogger != nil {
		m.debugLogger.Log("init: fileSize=%d, layout=%s, lines=%d", m.fileSize, m.cfg.Layout, m.cfg.Lines)
	}
	return tea.Batch(cmds...)
}

func tickSpinner() tea.Cmd {
	return tea.Tick(10*time.Millisecond, func(t time.Time) tea.Msg {
		return spinnerTickMsg(t)
	})
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
		layout := GetLayout(m.cfg.Layout)
		if m.cfg.Lines <= 0 {
			available := msg.Height - layout.TopLines - layout.BottomLines
			m.contentLines = max(10, available*2/3)
		} else {
			m.contentLines = m.cfg.Lines
		}
		if m.contentLines < 1 {
			m.contentLines = 1
		}
		if m.debugLogger != nil {
			m.debugLogger.Log("window-size: %dx%d, contentLines=%d, layout=%s", msg.Width, msg.Height, m.contentLines, m.cfg.Layout)
		}
		return m, nil

	case titleSetMsg:
		m.titleSet = true
		return m, nil

	case lineMsg:
		entry := LineEntry{
			Text:    string(msg),
			Time:    time.Now(),
			LineNum: m.buf.TotalCount() + len(m.pendingLines) + 1,
		}
		m.pendingLines = append(m.pendingLines, entry)
		if m.teeWriter != nil && m.cfg.TeeFormat == "raw" {
			m.teeWriter.WriteLine(string(msg))
		}
		if m.teeWriter != nil && m.cfg.TeeFormat == "display" {
			m.formatAndWriteDisplayTee(entry)
		}
		return m, nil

	case spinnerTickMsg:
		if m.eof {
			return m, nil
		}
		m.spinnerTick++
		if m.spinnerTick%10 == 0 {
			m.spinnerFrame++
		}
		for _, entry := range m.pendingLines {
			m.buf.Push(entry.Text)
		}
		m.pendingLines = nil
		return m, tickSpinner()

	case eofMsg:
		m.eof = true
		if m.teeWriter != nil {
			m.teeWriter.Close()
		}
		if m.debugLogger != nil {
			m.debugLogger.Log("eof: totalLines=%d, totalBytes=%d", m.buf.TotalCount(), m.buf.TotalBytes())
			m.debugLogger.Close()
		}
		return m, tea.Tick(200*time.Millisecond, func(t time.Time) tea.Msg {
			return tea.Quit()
		})

	case tea.KeyMsg:
		switch msg.String() {
		case "q", "Q", "ctrl+c":
			if m.teeWriter != nil {
				m.teeWriter.Close()
			}
			if m.debugLogger != nil {
				m.debugLogger.Close()
			}
			return m, tea.Quit
		}
	}

	return m, nil
}

func (m model) View() string {
	if m.width == 0 {
		return ""
	}

	layout := GetLayout(m.cfg.Layout)
	contentWidth := m.width - layout.SideWidth
	if contentWidth < 1 {
		contentWidth = 1
	}

	gutterWidth := m.gutterWidth()

	dataWidth := contentWidth - gutterWidth
	if dataWidth < 1 {
		dataWidth = 1
	}

	dataLines := m.contentLines
	if layout.Name == "quote" {
		dataLines = m.contentLines - 2
	}
	if dataLines < 0 {
		dataLines = 0
	}

	shouldStrip := m.cfg.Color == "never" || (m.cfg.Color == "auto" && !m.isStderrTTY)

	entries := m.buf.Last(dataLines)

	formattedLines := m.formatEntries(entries, dataWidth, shouldStrip)

	if len(formattedLines) > dataLines {
		start := len(formattedLines) - dataLines
		formattedLines = formattedLines[start:]
	}

	content := strings.Join(formattedLines, "\n")

	status := ""
	if layout.Name != "none" {
		statusWidth := contentWidth
		if layout.Name == "box" || layout.Name == "rounded" {
			statusWidth = m.width - 4
		}
		status = FormatStatusLine(
			m.cfg.Spinner,
			m.spinnerFrame,
			m.eof,
			m.buf.TotalCount(),
			m.buf.TotalBytes(),
			m.fileSize,
			statusWidth,
		)
	}

	title := "slit"

	return RenderLayout(layout, title, content, status, m.width, m.cfg.QuoteBg)
}

func (m model) formatAndWriteDisplayTee(entry LineEntry) {
	line := entry.Text
	shouldStrip := m.cfg.Color == "never" || (m.cfg.Color == "auto" && !m.isStderrTTY)
	if shouldStrip {
		line = StripANSI(line)
	}

	padW := m.lineNumPadWidth()
	var sb strings.Builder
	if m.cfg.Timestamp {
		ts := entry.Time.Format("15:04:05")
		sb.WriteString(ts)
		sb.WriteString(" ")
	}
	if m.cfg.LineNumbers {
		numStr := strconv.Itoa(entry.LineNum)
		sb.WriteString(fmt.Sprintf("%*s ", padW, numStr))
	}
	sb.WriteString(line)
	m.teeWriter.WriteLine(sb.String())
}

func (m model) gutterWidth() int {
	w := 0
	if m.cfg.LineNumbers {
		maxNum := m.buf.TotalCount()
		if maxNum < 1 {
			maxNum = 1
		}
		w += len(strconv.Itoa(maxNum)) + 1
	}
	if m.cfg.Timestamp {
		w += 9
	}
	return w
}

func (m model) lineNumPadWidth() int {
	if !m.cfg.LineNumbers {
		return 0
	}
	maxNum := m.buf.TotalCount()
	if maxNum < 1 {
		maxNum = 1
	}
	return len(strconv.Itoa(maxNum))
}

func (m model) formatEntries(entries []LineEntry, dataWidth int, shouldStrip bool) []string {
	var formatted []string
	for _, entry := range entries {
		line := entry.Text

		if shouldStrip {
			line = StripANSI(line)
		}

		var sublines []string
		if m.cfg.Wrap {
			if shouldStrip {
				sublines = WrapLine(line, dataWidth)
			} else {
				sublines = WrapLineANSI(line, dataWidth)
			}
		} else {
			if shouldStrip {
				sublines = []string{TrimLine(line, dataWidth, m.cfg.TruncationChar)}
			} else {
				sublines = []string{TrimLineANSI(line, dataWidth, m.cfg.TruncationChar)}
			}
		}

		padW := m.lineNumPadWidth()
		for i, sub := range sublines {
			var sb strings.Builder
			if m.cfg.Timestamp {
				ts := entry.Time.Format("15:04:05")
				sb.WriteString(ts)
				sb.WriteString(" ")
			}
			if m.cfg.LineNumbers {
				if i == 0 {
					numStr := strconv.Itoa(entry.LineNum)
					sb.WriteString(fmt.Sprintf("%*s ", padW, numStr))
				} else {
					sb.WriteString(strings.Repeat(" ", padW+1))
				}
			}
			sb.WriteString(sub)
			formatted = append(formatted, sb.String())
		}
	}
	return formatted
}
