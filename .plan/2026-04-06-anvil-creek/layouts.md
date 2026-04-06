# Layout Specifications

## Layout: box

Full borders with box-drawing characters. Title in top border, status in bottom border.

```
┌─ slit ─────────────────────────────────────┐
│ line 1...                                  │
│ line 2...                                  │
│ ...                                        │
│ line N...                                  │
└─ ⠋ Streaming... (142) q:quit ─────────────┘
```

- Top chrome: 1 line (border with title)
- Bottom chrome: 1 line (border with status)
- Side cost: 2 chars (left `│` + right `│`) + 2 chars padding = 4 chars total
- Content width: `term_width - 4`
- Content height: N (the `-n` value)
- Uses: `─ │ ┌ ┐ └ ┘`

## Layout: rounded

Same as box but with rounded corners.

```
╭─ slit ─────────────────────────────────────╮
│ line 1...                                  │
│ line 2...                                  │
│ ...                                        │
│ line N...                                  │
╰─ ⠋ Streaming... (142) q:quit ─────────────╯
```

- Same dimensions as box
- Uses: `─ │ ╭ ╮ ╰ ╯`

## Layout: compact

Colored top bar (reverse video / background color) + bottom status line. No side borders.

```
█ slit                                       
  line 1...
  ...
  line N...
⠋ Streaming... (142)                   q:quit
```

- Top chrome: 1 line (colored bar)
- Bottom chrome: 1 line (status)
- Side cost: 0
- Content width: `term_width`
- Content height: N
- Top bar: `lipgloss.NewStyle().Background(color).Width(termWidth).Render("slit")`
- Color: configurable via config (default: dark grey bg, light text)

## Layout: minimal (DEFAULT)

No top chrome. Just the status line below content.

```
line 1...
line 2...
...
line N...
⠋ Streaming... (142)                   q:quit
```

- Top chrome: 0
- Bottom chrome: 1 line (status)
- Side cost: 0
- Content width: `term_width`
- Content height: N
- Maximum content area

## Layout: none

No chrome whatsoever. Pure streaming output.

```
line 1...
line 2...
...
line N...
```

- Top chrome: 0
- Bottom chrome: 0
- Side cost: 0
- Content width: `term_width`
- Content height: N
- No spinner, no status, no keybindings
- Spinner disabled automatically in this mode

## Layout: quote

Left vertical bar on every line. Status line integrated inside the bar area.

```
▌ slit
▌ line 1...
▌ line 2...
▌ ...
▌ line N...
▌ ⠋ Streaming... (142)            q:quit
```

With optional background tint (configurable: `--quote-bg=color|off`):

```
█ slit
█ line 1...
█ line 2...
█ ...
█ line N...
█ ⠋ Streaming... (142)            q:quit
```

- Top chrome: 0 (title is first content line inside bar)
- Bottom chrome: 0 (status is last content line inside bar)
- Side cost: ~2 chars (left bar + 1 space)
- Content width: `term_width - 2`
- Content height: N - 2 (title + status take from content budget)
  - Actually: the title and status lines are rendered inside the N-line area
  - Effective data lines visible: N - 2
- Left bar char: `▌` or `│` (configurable)
- Background: optional, configurable color (default: off)
- **Note**: background tint may conflict with ANSI-colored content passthrough

## Chrome Cost Summary

| Layout | Extra lines (outside N) | Side width cost | Effective data lines |
|--------|------------------------|-----------------|---------------------|
| box | +2 | ~4 chars (2 border + 2 padding) | N |
| rounded | +2 | ~4 chars (2 border + 2 padding) | N |
| compact | +2 | 0 | N |
| minimal | +1 | 0 | N |
| none | 0 | 0 | N |
| quote | 0 | ~2 chars | N - 2 |

## Status Line Format

Shared across all layouts (except `none`). Single line:

```
⠋ Streaming... (142 lines) [====      ]  q:quit
```

| Component | Position | Content |
|-----------|----------|---------|
| Spinner | LHS | Braille animation frame |
| Label | LHS | "Streaming..." or "Done." |
| Line count | LHS | "(142 lines)" or "(142 lines, 1.2MB)" |
| Progress bar | LHS | `[====      ]` if file size known |
| Keybinding hints | RHS | `q:quit` (expandable in v2) |

After EOF:
```
Done. (142 lines) [===================]  q:quit
```

## Layout Selection

- CLI: `--layout=box|rounded|compact|minimal|none|quote`
- Shortcut flags: `--box`, `--rounded`, `--compact`, `--minimal`, `--none`, `--quote`
- Config: `layout = "minimal"` in config.toml
- Priority: CLI flag > config > default (minimal)
