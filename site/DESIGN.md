# slit site — design brief

A reviewer should be able to open `index.html` and `downloads.html` in a browser and verify every item below.

---

## Aesthetic direction

**Small, sleek, powerful, useful, bold, visceral. Not ostentatious.**

- Font: JetBrains Mono throughout — headings, body, code, nav, everything. No exceptions.
- Background: near-black `#060606`. No gradients on the background.
- Single accent color: electric cyan `#00e5ff`. Used sparingly: borders on interactive/chrome elements, accent text, the slit-cut line, hover states.
- Zero rounded corners anywhere in the UI.
- All structural borders are 1px, in `#1a1a1a` or `#242424`.
- No drop shadows except the box-shadow trick used internally.
- Secondary language colors (only in implementation cards and source sections): C = `#aac8ff`, Rust = `#ff8c5a`, Go = `#79c9e8`.

---

## Hero animation — `index.html`

The landing page must open directly into the animation. There is no static initial state.

### Sequence (total ~5 seconds)

**Phase 1 — stream fills (0–400ms)**
- Full-screen dark background covered in dim green-tinted terminal text (`#1c301c`).
- Text is 90 lines pre-loaded synchronously, then new lines added at ~14/sec.
- Four dark panels (same `#060606` color as background) are at 2% inset each side — barely visible, so almost the full viewport shows the stream.
- Nav is transparent (no background).

**Phase 2 — panels squeeze (400–2300ms)**
- All four panels animate inward simultaneously with cubic ease-in-out.
- Final resting position: **top 32%, bottom 32%, left 10%, right 10%**.
- This creates a horizontal letterbox "slit" shape: ~80% wide × ~36% tall.
- The stream text slows from 70ms/line to 220ms/line as panels close.
- The visible "window" into the stream is clearly the shape of a slit.

**Phase 3 — chrome appears (2400–2600ms)**
- A 1px cyan border fades in, outlining the clip window exactly.
- A header bar appears at the top of the window: `$ tail -f pipeline.log | slit --box` in dim cyan.
- A status bar appears at the bottom of the window with: braille spinner character, line count, `│` separator, MB/s rate, and keybinding hints (`q quit  ^C exit  -b --box  -r --rounded  -c --compact`).
- Stream text is visible through the transparent chrome body.

**Phase 4 — spinner + counter (2600–3500ms)**
- Braille spinner cycles: `⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏` at 80ms per frame.
- Line counter eases from 0 to `47.3k lines` over 900ms.
- MB/s readout eases from `0.0` to `9.8 MB/s` in sync.

**Phase 5 — fade out (3500–4000ms)**
- All stream elements (stream bg, all four panels, chrome) fade to opacity 0 together.
- Spinner interval is cleared.

**Phase 6 — SLIT title (4100–4600ms)**
- `SLIT` appears in `clamp(7rem, 20vw, 18rem)` JetBrains Mono 800 weight, color `#efefef`.
- Enters by easing up 28px and fading in simultaneously.
- Letter-spacing `-0.05em`.

**Phase 7 — slit cut (4550–4830ms)**
- A 2px horizontal cyan line (`#00e5ff`) scales from `scaleX(0)` to `scaleX(1)` left-to-right through the middle of the "SLIT" title.
- The line extends 8% beyond each edge of the word.
- It uses `transform-origin: left center`.
- Once fully drawn, the line stays visible (it is a permanent design element of the title).

**Phase 8 — tagline + install (from 4600ms)**
- Tagline appears below: `stream viewer for the terminal` in `0.8rem`, all-caps, letter-spaced, `#444` color.
- Install command block appears: `go install github.com/clankercode/slit/go@latest` in accent cyan, with a `copy` button.
- Scroll indicator (text "scroll" + a pulsing 1px vertical line) fades in at 60% opacity.

### Interaction after animation

- `copy` button: copies the install command to clipboard, changes text to `copied` in green for 2s.
- Nav becomes opaque (dark bg + bottom border) after scrolling past 60px.
- All sections below the fold have scroll-reveal: fade in + translateY(16px → 0) as they enter the viewport.

---

## Page sections — `index.html`

### Nav (fixed, full-width)
Links: features · implementations · usage · downloads · github ↗  
Logo: `slit` in 800-weight, `-0.04em` letter-spacing, near-white.  
Initially transparent. Opaque + backdrop-blur after scroll.

### Intro (`#intro`)
- Eyebrow: `what is slit`
- H2: `Your terminal output / is going nowhere fast. / Now it is.` — "Now it is." in accent cyan.
- Lead paragraph explaining the core mechanic: fixed-height viewport, live, up to 100fps, pipe anything, press `q` to quit.

### Features (`#features`)
- Eyebrow: `features`
- H2: `Built for streams. / Not afterthoughts.`
- 3×2 grid (6 cells), each bordered, no corner radius.
- Numbered 01–06. Each has title, body text, and a `→ one-liner` in accent cyan.
- Features: Streaming-native / Six layouts / Pipe-safe passthrough / Capture while viewing / Progress aware / Resize-resilient.

### Implementations (`#implementations`)
- Eyebrow: `implementations`
- H2: `Pick your weapon.`
- 3-column grid: C · Rust · Go. Each column has language name in its own color, stat rows (binary size, throughput, lines/sec, peak RAM, deps), and a `download binary →` link.
- Benchmark note below: `Benchmarks: 1M lines, plain input, Linux x86_64. C wins on throughput; Rust on balance; Go on features.`

**Exact benchmark values to verify:**

| impl | binary | throughput | lines/sec | peak RAM | deps |
|------|--------|------------|-----------|----------|------|
| C | 64 KB | 151.8 MB/s | 151,741 | 14.5 MB | libc only |
| Rust | 2.2 MB | 77.5 MB/s | 528,000 | 22.0 MB | ratatui + tokio |
| Go | 6.0 MB | 9.8 MB/s | 66,600 | 33.4 MB | bubbletea + cobra |

### Usage (`#examples`)
- Eyebrow: `usage`
- H2: `Pipe it in.`
- 5 examples, each with a label and a `<pre>` code block:
  1. `make 2>&1 | slit --box -o build.log`
  2. `tail -f /var/log/syslog | slit --compact -l -t`
  3. `pytest tests/ | slit | grep FAILED > failures.log`
  4. `./etl-job | slit --rounded -w -l | kafka-producer --topic events`
  5. Config file example (`~/.config/slit/config.toml`)
- Code blocks have syntax coloring: prompts dim, flags in accent cyan, args in `#aad4ff`.

### Download CTA
- Vertical accent line from top edge.
- `v0.2.3` eyebrow.
- H2: `Get slit.`
- Three buttons: `all downloads →` (primary, filled cyan), `view source ↗`, `releases ↗`.

### Footer
- Left: `slit v0.2.3 — CC0-1.0 license`
- Right links: github · releases · issues

---

## Downloads page — `downloads.html`

### Structure
- Sticky nav (same style as index, always opaque on this page).
- Eyebrow: `downloads`, H1: `Get slit.`
- Lead paragraph: "Pre-built binaries for Linux x86_64..."

### Version row
- Shows `v0.2.3` tag (cyan border), changelog link, "latest release" label.

### Platform tabs
- Three tabs: `Linux x86_64` · `Linux arm64` · `source`
- Active tab: accent color + faint cyan background.
- Only one panel visible at a time.

### Linux x86_64 panel
Table with columns: implementation · file · size · sha256 · download button.  
Rows: C (primary download button), Rust, Go.  
C row uses primary (filled) download button. Rust and Go use secondary.

### Linux arm64 panel
Shows "arm64 binaries coming soon" message with a build-from-source link.

### Source panel
Table with: Go (`go install`), Rust (`cargo install`), C (link to GitHub). Each row has a copy button.

### Build from source
3-column grid showing exact commands for C (`make`), Rust (`cargo build --release`), Go (`go build -o slit ./...`). Each with `./slit --version` verification step showing expected output.

### Verify section
Instructions for `sha256sum -c slit-linux-x64.sha256`.

### Quick start banner
Full-width block with `go install github.com/clankercode/slit/go@latest` + `copy command` primary button.

---

## What a reviewer should check

1. **Animation plays automatically** on page load with no user interaction required.
2. **The slit-cut line** visibly draws left-to-right through the center of the word "SLIT" after the title appears.
3. **The clip window is letterbox-shaped** (much wider than tall) at the end of Phase 2 — it should look like a slit in the screen.
4. **Stream text is visible** through the clip window as it shrinks — you can read (partial) terminal output lines inside the frame.
5. **The braille spinner animates** (10 frames, 80ms each) inside the status bar.
6. **Line counter animates** from 0 to 47.3k, MB/s from 0.0 to 9.8.
7. **Nav becomes opaque** when scrolling down past the hero.
8. **Copy button works** — copies the `go install` command, shows `copied` feedback.
9. **Platform tabs** on downloads page switch panels correctly.
10. **No rounded corners** anywhere.
11. **No font other than JetBrains Mono** anywhere on either page.
12. **Benchmark numbers** match the table above exactly.
