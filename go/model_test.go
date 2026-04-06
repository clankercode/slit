package main

import (
	"testing"

	tea "github.com/charmbracelet/bubbletea"
)

func testModel() model {
	cfg := DefaultConfig()
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 10
	return m
}

func TestQuitOnKeyQ(t *testing.T) {
	m := testModel()

	_, cmd := m.Update(tea.KeyMsg{
		Type:  tea.KeyRunes,
		Runes: []rune{'q'},
	})

	if cmd == nil {
		t.Fatal("expected non-nil cmd for 'q' key, got nil")
	}

	quitCmd := tea.Quit
	if cmd() != quitCmd() {
		t.Errorf("expected tea.Quit cmd for 'q' key, got %v", cmd)
	}
}

func TestQuitOnCtrlC(t *testing.T) {
	m := testModel()

	_, cmd := m.Update(tea.KeyMsg{
		Type: tea.KeyCtrlC,
	})

	if cmd == nil {
		t.Fatal("expected non-nil cmd for ctrl+c, got nil")
	}

	quitCmd := tea.Quit
	if cmd() != quitCmd() {
		t.Errorf("expected tea.Quit cmd for ctrl+c, got %v", cmd)
	}
}

func TestQuitClosesTeeWriter(t *testing.T) {
	tmp := t.TempDir()
	cfg := DefaultConfig()
	cfg.Output = tmp + "/tee.log"
	m := newModel(&cfg)
	m.width = 80
	m.height = 24
	m.contentLines = 10

	if m.teeWriter == nil {
		t.Fatal("expected teeWriter to be initialized")
	}

	_, cmd := m.Update(tea.KeyMsg{
		Type:  tea.KeyRunes,
		Runes: []rune{'q'},
	})

	quitCmd := tea.Quit
	if cmd() != quitCmd() {
		t.Errorf("expected tea.Quit cmd, got %v", cmd)
	}
}

func TestOtherKeysIgnored(t *testing.T) {
	m := testModel()

	_, cmd := m.Update(tea.KeyMsg{
		Type:  tea.KeyRunes,
		Runes: []rune{'a'},
	})

	if cmd != nil {
		t.Errorf("expected nil cmd for non-quit key 'a', got %v", cmd)
	}
}
