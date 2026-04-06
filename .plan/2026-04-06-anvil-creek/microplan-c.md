# Micro Plan — C Implementation

Each step produces a testable increment. Steps are ordered by dependency.
C requires more steps than Go because everything — terminal control, rendering,
memory management, TOML parsing — is manual.

---

## Step 1: Project scaffold + Makefile + slit.h

**Goal**: `make` produces a `slit` binary that prints version and exits.

- `c/Makefile`: targets for `build`, `test`, `valgrind`, `clean`
  - CC=gcc, CFLAGS=`-Wall -Wextra -Werror -std=c11 -pedantic`
  - DEBUG flags: `-g -O0 -fsanitize=address,undefined`
  - VALGRIND target: `valgrind --leak-check=full --error-exitcode=1`
- `c/slit.h`: shared types and constants
  - `#define VERSION "0.1.0"`
  - `#define DEFAULT_MAX_LINES 50000`
  - `#define DEFAULT_LAYOUT LAYOUT_MINIMAL`
  - `#define RENDER_INTERVAL_MS 33`
  - `#define DEFAULT_SPINNER SPINNER_BRAILLE`
  - `enum layout_type`, `enum spinner_type`, `enum color_mode`, `enum tee_format`
  - `struct line_entry { char *text; time_t arrival; size_t line_num; size_t byte_len; }`
  - `struct slit_config` (all config fields with defaults)
  - `volatile sig_atomic_t` flag declarations for signals
- `c/main.c`: minimal `main()` that prints version
- **Test**: `make && ./slit --version` prints `slit 0.1.0`

## Step 2: CLI flags (getopt_long) + config struct

**Goal**: `./slit --help` shows all flags, all flags parse into config struct.

- `c/main.c`: hand-rolled `getopt_long_only()` loop parsing all flags from cli.md
  - Core: `-n`, `--max-lines`, `-o`, `-a`, `--tee-format`
  - Display: `-l`, `--color`, `-w`, `-t`, `--truncation-char`
  - Layout: `--layout`, `--box`, `--rounded`, `--compact`, `--minimal`, `--none`, `--quote`, `--quote-bg`
  - Progress: `--spinner`
  - Debug: `-d`, `--log-file`
  - Meta: `--help`, `--version`
  - Subcommand: `completion` (detect `argv[1] == "completion"`)
- `slit_config` struct populated with defaults, then overridden by flags
- Layout shortcut flags (`--box` etc.) resolve to `enum layout_type`
- `--help`: hand-formatted to match cli.md draft exactly
- `--version`: print `slit VERSION`
- Error on unknown flags → exit 1
- **Test**: `./slit --help` shows all flags; `./slit -n 20 --box --debug` parses without error

## Step 3: Ring buffer

**Goal**: Bounded circular buffer storing `LineEntry` structs. Unit tests pass.

- `c/buffer.h`: `struct ring_buffer` definition
  - `struct line_entry *entries` — heap-allocated array
  - `size_t capacity`, `size_t count`, `size_t head` (write position)
  - `size_t total_lines` — all lines ever seen (including evicted)
  - `size_t total_bytes` — all bytes ever read
- `c/buffer.c`:
  - `buffer_create(size_t capacity)` — allocates, returns initialized struct
  - `buffer_push(struct ring_buffer *b, struct line_entry entry)` — adds line, evicts oldest if full
  - `buffer_get(struct ring_buffer *b, size_t index)` — returns pointer to entry (offset from tail)
  - `buffer_count(struct ring_buffer *b)` — current count
  - `buffer_total_lines(struct ring_buffer *b)` — total including evicted
  - `buffer_total_bytes(struct ring_buffer *b)` — total bytes
  - `buffer_free(struct ring_buffer *b)` — frees entry strings + array
  - Push: `strdup(entry.text)`, free evicted entry's string
- `c/test/test_buffer.c`:
  - Test push/count/capacity
  - Test eviction (push 100 into capacity-10, verify last 10 kept)
  - Test total_lines increments past capacity
  - Test total_bytes tracking
- Makefile `test` target compiles and runs test binaries
- **Test**: `make test_buffer && ./test_buffer` → all pass

## Step 4: Terminal control (termios + /dev/tty)

**Goal**: Semi-raw mode enters/restores correctly. `/dev/tty` opened for keyboard input.

- `c/terminal.h`:
  - `struct terminal_state { int tty_fd; struct termios orig_termios; int raw_active; }`
  - Function declarations: `term_init()`, `term_raw_enter()`, `term_raw_restore()`, `term_get_size()`, `term_cleanup()`
- `c/terminal.c`:
  - `term_init()`: open `/dev/tty` with `open("/dev/tty", O_RDWR)`, save to state
  - `term_raw_enter(int fd)`:
    - `tcgetattr(fd, &orig_termios)`
    - Copy to raw: disable `ICANON | ECHO | ECHOE | ECHOK | ECHONL`
    - Keep `ISIG` (Ctrl-C → SIGINT), keep `OPOST | ONLCR`
    - Set `VMIN=1, VTIME=0`
    - `tcsetattr(fd, TCSADRAIN, &raw)`
  - `term_raw_restore()`: `tcsetattr(fd, TCSADRAIN, &orig_termios)`
  - `term_get_size(int fd, int *rows, int *cols)`: `ioctl(fd, TIOCGWINSZ, &ws)`
  - `term_cleanup()`: restore terminal, close `/dev/tty`
  - `isatty(STDERR_FILENO)` check — if false, return error (caller enters passthrough)
- `c/test/test_terminal.c`: test raw mode enter/restore cycle, verify `isatty` logic
- **Test**: `./slit` enters raw mode, terminal behaves normally after exit

## Step 5: Signal handling

**Goal**: All signals handled via deferred flags. Clean exit on SIGINT/SIGTERM.

- `c/terminal.c` (signal handling lives here):
  - `volatile sig_atomic_t sigint_flag = 0`
  - `volatile sig_atomic_t sigwinch_flag = 0`
  - `volatile sig_atomic_t sigtstp_flag = 0`
  - `void sigint_handler(int sig) { (void)sig; sigint_flag = 1; }`
  - `void sigwinch_handler(int sig) { (void)sig; sigwinch_flag = 1; }`
  - `void sigtstp_handler(int sig)` — restore terminal, raise SIGSTOP
  - `void sigcont_handler(int sig)` — re-enter raw mode
  - `signals_install()`:
    - `sigaction(SIGINT, ...)` → sets `sigint_flag`
    - `signal(SIGPIPE, SIG_IGN)`
    - `sigaction(SIGWINCH, ...)` → sets `sigwinch_flag`
    - `sigaction(SIGTERM, ...)` → sets `sigint_flag` (same exit path)
    - `sigaction(SIGHUP, ...)` → sets `sigint_flag`
    - `sigaction(SIGTSTP, ...)` → restore terminal, `raise(SIGSTOP)`
    - `sigaction(SIGCONT, ...)` → re-enter raw mode
  - Use `struct sigaction` with `SA_RESTART` where appropriate
- **Test**: `./slit` running, send `kill -INT $PID` → clean exit; send `kill -WINCH $PID` → no crash

## Step 6: Passthrough mode + minimal event loop

**Goal**: `echo -e "line1\nline2\nline3" | ./slit -n 2` shows last 2 lines + status on stderr. Passthrough data on stdout. `q` or Ctrl-C quits.

- `c/main.c` reworked into event loop:
  - Startup: parse flags → init terminal → install signals → init buffer → enter raw mode
  - If `!isatty(STDERR_FILENO)` and no `SLIT_FORCE_RENDER` env → passthrough mode
  - Passthrough: `while (getline(&line, &len, stdin) != -1) { fputs(line, stdout); }`
  - Event loop with `poll()`:
    ```c
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;   fds[0].events = POLLIN;
    fds[1].fd = tty_fd;         fds[1].events = POLLIN;
    while (!sigint_flag && !eof_reached) {
        int ret = poll(fds, 2, RENDER_INTERVAL_MS);
        if (fds[0].revents & POLLIN)  /* read line, push to buffer, write to stdout */
        if (fds[1].revents & POLLIN)  /* read key, check for 'q' */
        if (sigwinch_flag)            /* update term size */ 
        render_frame();               /* render current buffer to stderr */
    }
    ```
  - Stdin reading: `getline()` or `read()` line-by-line
  - Render to stderr: `fprintf(stderr, ...)` with ANSI escapes
  - Minimal layout only: N content lines + 1 status line
  - Status line: `"Streaming... (N lines)  q:quit"`
  - EOF → show `"Done. (N lines)"` → exit after brief pause
  - Stdout passthrough: each line written to stdout as it arrives
  - Shutdown: restore terminal, free buffer, exit
  - `SLIT_FORCE_RENDER=1` env var overrides `isatty` check
- **Test**: `seq 1 100 | ./slit -n 5` shows lines 96-100 + status on stderr, exits 0

## Step 7: Line trimming + truncation indicator

**Goal**: Lines trimmed to terminal width, truncation char appended when trimmed.

- `c/render.h`:
  - `size_t trim_line(const char *src, char *dst, size_t dst_size, size_t width, const char *trunc_char)`
  - `size_t visible_strlen(const char *s)` — strlen minus ANSI sequences
- `c/render.c`:
  - `visible_strlen()`: state machine scanning for `\x1b[`, skipping to `m`, counting visible chars only
  - `trim_line()`:
    - Walk `src` char-by-char
    - Track visible column position (ANSI sequences pass through without counting)
    - When visible position reaches `width - strlen(trunc_char)`, stop copying
    - Append `trunc_char` if line was truncated
    - Return number of visible chars written
  - Width = `term_width - layout_side_cost` (initially 0 for minimal)
  - `--truncation-char` configurable (default `"…"`)
- `c/test/test_render.c`:
  - Test trim of plain string to various widths
  - Test trim of ANSI-colored string (escape sequences preserved)
  - Test no truncation when line fits
  - Test `visible_strlen` with and without ANSI
  - Test custom truncation char
- **Test**: `make test_render && ./test_render` → all pass

## Step 8: Layout system — minimal + none

**Goal**: Formal layout framework. `--layout=minimal` (default) and `--layout=none` render correctly.

- `c/layout.h`:
  - `struct layout_chrome { int top_lines; int bottom_lines; int side_width; }`
  - `layout_chrome layout_get_chrome(enum layout_type type)`
  - `void layout_render_top(enum layout_type type, int width, const char *title)`
  - `void layout_render_content_line(enum layout_type type, int width, int line_idx, const char *line)`
  - `void layout_render_bottom(enum layout_type type, int width, const char *status)`
  - `int layout_content_width(enum layout_type type, int term_width)`
  - `int layout_total_height(enum layout_type type, int content_lines)`
- `c/layout.c`:
  - Chrome cost table:
    ```c
    {LAYOUT_MINIMAL, .top_lines=0, .bottom_lines=1, .side_width=0},
    {LAYOUT_NONE,    .top_lines=0, .bottom_lines=0, .side_width=0},
    ```
  - Minimal: content lines + status line (already working from step 6)
  - None: just content lines, no status, spinner disabled
  - `layout_content_width()` = `term_width - side_width`
  - `layout_total_height()` = `content_lines + top_lines + bottom_lines`
- Render function writes full frame to stderr:
  - Clear pane area: `\x1b[${total_height}A` (move up) then `\x1b[0J` (clear down)
  - Write top chrome (if any)
  - Write N content lines (trimmed via `trim_line`)
  - Write bottom chrome / status (if any)
- **Test**: `seq 1 100 | ./slit -n 5 --minimal` shows 5 lines + status; `seq 1 100 | ./slit -n 5 --none` shows 5 lines only

## Step 9: Layout system — box + rounded

**Goal**: `--layout=box` and `--layout=rounded` render with full borders.

- Update `c/layout.c` chrome cost table:
  ```c
  {LAYOUT_BOX,     .top_lines=1, .bottom_lines=1, .side_width=4},
  {LAYOUT_ROUNDED, .top_lines=1, .bottom_lines=1, .side_width=4},
  ```
- Box layout rendering:
  - Top: `┌─ slit ─${dashes}┐` (title embedded in top border)
  - Content: `│ ${line}${spaces} │` (pad to `width - 4`)
  - Bottom: `└─ ${status} ─${dashes}┘` (status in bottom border)
  - Characters: `─ │ ┌ ┐ └ ┘`
- Rounded layout: same structure, different corner characters
  - Characters: `─ │ ╭ ╮ ╰ ╯`
- Side padding: 1 space inside each vertical border = 4 chars total cost
- Title truncated if wider than available space
- Status right-padded with `─` to fill border
- `--box` shortcut → `LAYOUT_BOX`, `--rounded` → `LAYOUT_ROUNDED`
- **Test**: `seq 1 100 | ./slit -n 5 --box` shows bordered box; `seq 1 100 | ./slit -n 5 --rounded` shows rounded borders

## Step 10: Layout system — compact + quote

**Goal**: `--layout=compact` and `--layout=quote` render correctly.

- Update chrome cost table:
  ```c
  {LAYOUT_COMPACT, .top_lines=1, .bottom_lines=1, .side_width=0},
  {LAYOUT_QUOTE,   .top_lines=0, .bottom_lines=0, .side_width=2},
  ```
- Compact layout:
  - Top: reverse-video bar `"\x1b[7m slit ${spaces}\x1b[0m"` (fills full width)
  - Content: plain lines, no side borders
  - Bottom: status line (same as minimal)
- Quote layout:
  - Left bar char `▌` (or `│`) prepended to every line: `"▌ ${line}"`
  - Title is first content line: `"▌ slit"`
  - Status is last content line: `"▌ ${status_text}"`
  - Effective data lines: N - 2 (title + status consume from content budget)
  - `--quote-bg=color|off`:
    - If color: ANSI background set on each line: `"\x1b[48;2;R;G;Bm▌ ${line}\x1b[0m"`
    - Parse hex color `#RRGGBB` → RGB values
    - `off` (default): no background, just the bar character
  - `--compact` → `LAYOUT_COMPACT`, `--quote` → `LAYOUT_QUOTE`
- **Test**: `seq 1 100 | ./slit -n 5 --compact` shows top bar; `seq 1 100 | ./slit -n 5 --quote` shows left bar; `seq 1 100 | ./slit -n 5 --quote --quote-bg=#1a1a2e` shows background tint

## Step 11: Spinner styles

**Goal**: Animated spinner in status line. `--spinner=braille|dots|arrows|off`.

- `c/spinner.h`:
  - Spinner frame arrays as `static const char *[]`
  - `const char *spinner_frame(enum spinner_type type, int frame_idx)`
  - `int spinner_frame_count(enum spinner_type type)`
- `c/spinner.c`:
  - Braille: `{ "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏" }` (10 frames)
  - Dots: `{ "⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷" }` (8 frames)
  - Arrows: `{ "←", "↖", "↑", "↗", "→", "↘", "↓", "↙" }` (8 frames)
  - Off: `{ "" }` (single empty frame)
  - Frame advancement: `frame_idx = (frame_idx + 1) % spinner_frame_count(type)`
  - Advance once per render tick (every ~33ms)
- **Test**: `echo hi | ./slit --spinner=braille` shows animated spinner; `echo hi | ./slit --spinner=off` shows no spinner

## Step 12: Status line — full implementation

**Goal**: Complete status line with spinner + label + line count + keybinding hints.

- `c/render.c` — `render_status_line()`:
  - Format: `"<spinner> <label> (<count>)  <keys>"`
  - Components:
    - Spinner: frame from step 11
    - Label: `"Streaming..."` while active, `"Done."` after EOF
    - Count: `"(N lines)"` or `"(N lines, X.XKB/MB)"` with byte count
    - Byte formatting: human-readable (KB, MB, GB)
    - Keybinding hints: `"q:quit"` right-aligned
  - Right-align hints: pad middle with spaces so `q:quit` lands at RHS
  - Width-aware: truncate label/count if terminal very narrow
- After EOF: spinner stops (freeze on last frame or blank), label → `"Done."`
- Layout integration:
  - Box/rounded: status embedded in bottom border
  - Compact/minimal: status as separate line
  - Quote: status as last content line inside bar
  - None: no status line rendered
- Byte count from `buffer_total_bytes()`
- Line count from `buffer_total_lines()`
- **Test**: `seq 1 100 | ./slit -n 5` shows `"⠋ Streaming... (100 lines)    q:quit"`

## Step 13: Progress bar

**Goal**: Progress bar shown when stdin is a regular file with known size.

- `c/render.c` — `render_progress_bar()`:
  - Only when: `fstat(STDIN_FILENO, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0`
  - Calculate: `progress = (float)total_bytes / st.st_size`
  - Render: `"[====      ]"` — proportional fill
  - Bar width: configurable portion of status line (e.g. 20 chars)
  - Fill char: `=`, empty char: ` `
  - After EOF: `"[===================]"` (full)
- Integration into status line (step 12):
  - Status format becomes: `"<spinner> <label> (<count>) [<progress>]  <keys>"`
  - Progress bar omitted for pipes (no known size)
- **Test**: Create a temp file, `cat tempfile | ./slit` shows progress bar filling

## Step 14: Line numbers

**Goal**: `-l` flag shows dim-colored right-aligned line numbers.

- `c/render.c` — `format_line_with_number()`:
  - Right-align line number in gutter
  - Gutter width: `max(3, digits_in(buffer_total_lines()))` chars
  - Format: `"\x1b[2m%*zu\x1b[0m "` (dim ANSI, right-aligned, then space)
  - Line number from `line_entry.line_num` (1-indexed, total count)
- Gutter width calculated once per render based on current `total_lines`
- Gutter width reduces content width: `content_width -= gutter_width`
- Affects trim width calculation
- **Test**: `seq 1 100 | ./slit -n 5 -l` shows right-aligned dim line numbers

## Step 15: ANSI color passthrough

**Goal**: `--color=auto|always|never` handles colored input correctly.

- `c/render.c`:
  - `strip_ansi(const char *src, char *dst, size_t dst_size)` — state machine:
    - State 0: normal char → copy to dst
    - State 1: saw `\x1b` → enter escape
    - State 2: saw `[` → enter CSI sequence
    - State 3: in CSI → accumulate until `@-~` terminator
    - On `never`: strip all CSI sequences
    - On `always`: pass through everything
    - On `auto`: `isatty(STDERR_FILENO)` → pass through; else strip
  - Update `visible_strlen()` to be robust (already done in step 7)
  - Update `trim_line()` to handle ANSI correctly (already done in step 7)
- Ensure ANSI sequences don't count toward visible width in any render path
- `--color` flag resolution: default `auto`, overrides from CLI
- **Test**: `printf "\033[31mred\033[0m normal\n" | ./slit --color=always` shows colored output; `printf "\033[31mred\033[0m normal\n" | ./slit --color=never 2>&1 | cat` shows plain text

## Step 16: Timestamp prefix

**Goal**: `-t` flag prepends `HH:MM:SS` to each line.

- `line_entry.arrival` already stored as `time_t` (set on push)
- `c/render.c` — `format_timestamp()`:
  - `localtime_r(&entry->arrival, &tm)`
  - `strftime(buf, sizeof(buf), "%H:%M:%S", &tm)`
  - Format: `"\x1b[2m%s\x1b[0m "` (dim, then space)
- Timestamp width: 8 chars (`HH:MM:SS`) + 1 space = 9 chars
- Reduces content width: `content_width -= 9`
- Order with line numbers: timestamp first, then line number, then content
  - `HH:MM:SS  123 content...`
- Combined gutter: timestamp width + line number gutter width
- **Test**: `seq 1 5 | ./slit -t` shows timestamps; `seq 1 5 | ./slit -t -l` shows both

## Step 17: Wrap mode (ANSI-aware)

**Goal**: `--wrap` wraps long lines instead of truncating.

- `c/render.c` — `wrap_line()`:
  - `int wrap_line(const char *line, size_t width, char **out_lines, int *out_count)`
  - Walk input, tracking visible column
  - When column reaches `width`: insert `\n`, reset column, continue
  - ANSI sequences pass through without incrementing column
  - Allocate array of strings for wrapped sub-lines
  - Returns number of sub-lines produced
- Each wrapped sub-line counts against visible line budget
  - If one logical line wraps to 3 sub-lines, it consumes 3 of N visible slots
  - Only last N visible sub-lines shown (scroll within wrapped lines)
- When `--wrap` is off: use `trim_line()` (truncate) as before
- Memory: caller must free wrapped line array
- **Test**: `python3 -c "print('x'*200)" | ./slit -n 5 --wrap` shows wrapped lines

## Step 18: Tee to file

**Goal**: `-o file` writes input to file, `-a` appends, `--tee-format=raw|display`.

- `c/tee.h`:
  - `struct tee_writer { FILE *fp; enum tee_format format; }`
  - `tee_open(const char *path, int append, enum tee_format format)`
  - `tee_write_line(struct tee_writer *tw, const char *raw_line, const char *display_line)`
  - `tee_close(struct tee_writer *tw)`
- `c/tee.c`:
  - `tee_open()`: `fopen(path, append ? "a" : "w")`, store format
  - `tee_write_line()`:
    - `raw` format: write `raw_line` (original line from stdin)
    - `display` format: write `display_line` (formatted with line numbers, timestamps, etc.)
    - **Critical**: only write once per line, not on every render frame
    - Write on push to buffer (once per new line), not during render
  - `tee_close()`: `fclose(fp)`
- Integration in main event loop:
  - On new line from stdin: `tee_write_line()` called immediately
  - Display format includes line numbers/timestamps if enabled
  - Close on shutdown
- Stdout passthrough continues regardless of tee
- **Test**: `seq 1 10 | ./slit -o /tmp/test.log && diff <(seq 1 10) /tmp/test.log`; `seq 1 10 | ./slit -o /tmp/test.log --tee-format=display -l && head -1 /tmp/test.log` shows line number

## Step 19: Window title

**Goal**: Terminal title set to `slit` on startup, restored on exit.

- `c/terminal.c`:
  - `term_set_title(const char *title)`: write `"\x1b]0;%s\x07"` to tty_fd
  - `term_restore_title()`: write `"\x1b]0;\x07"` to tty_fd (clear title)
- Call `term_set_title("slit")` after entering raw mode
- Call `term_restore_title()` in shutdown sequence
- Shutdown: title restore → terminal restore → close tty → exit
- **Test**: `echo hi | ./slit` — terminal title changes to "slit" then clears on exit

## Step 20: Render debouncing

**Goal**: Cap at ~30fps even if input arrives faster. No flicker.

- `c/render.c` (or `c/main.c`):
  - Track `struct timespec last_render_time`
  - `should_render()`:
    - `clock_gettime(CLOCK_MONOTONIC, &now)`
    - If `elapsed_ms(last_render_time, now) >= 33` → render, update timestamp
    - If new lines arrived but interval not elapsed → skip
  - Force render on: EOF, SIGWINCH, keypress
- Event loop modification:
  - `poll()` already has 33ms timeout (provides natural tick)
  - On each poll return: if stdin had data, buffer lines but don't render immediately
  - On timeout (no data): render if dirty flag set
  - `dirty` flag set when new lines pushed, cleared after render
- **Test**: `cat /dev/urandom | fold | ./slit -n 5` — no excessive flicker

## Step 21: Debug logging

**Goal**: `--debug` writes diagnostic info to log file.

- `c/debug.h`:
  - `void debug_init(const char *log_path, int enabled)`
  - `void debug_log(const char *fmt, ...)`
  - `void debug_close(void)`
- `c/debug.c`:
  - `debug_init()`: `fopen(log_path, "w")`, store `FILE *`
  - `debug_log()`: `va_list` → `vfprintf(fp, fmt, args)` → `fflush(fp)`
  - Default path: `/tmp/slit-$PID.log` (expand `$PID` to actual pid)
  - Log entries include timestamp: `"[HH:MM:SS.ms] message\n"`
  - Log points:
    - Startup: config resolution, terminal size, layout type
    - Per-render: line count, render time, dirty flag
    - Signals: SIGWINCH, SIGINT, SIGTSTP
    - Buffer: push, eviction events
    - EOF detection
    - Shutdown sequence
  - `debug_close()`: `fclose(fp)`
- Enabled via `--debug` flag or `debug.enabled = true` in config
- Configurable via `--log-file`
- No-op macros when disabled (avoid overhead)
- **Test**: `echo hi | ./slit --debug && cat /tmp/slit-*.log | head -5` shows log entries

## Step 22: Config file loading (hand-rolled TOML)

**Goal**: `~/.config/slit/config.toml` loaded, merged with CLI flags (CLI wins).

- `c/config.h`:
  - `int config_load_file(const char *path, struct slit_config *cfg)`
- `c/config.c`:
  - Hand-rolled TOML parser (minimal):
    - Skip blank lines and `# comment` lines
    - Parse `[section]` headers: `display`, `buffer`, `spinner`, `quote`, `debug`
    - Parse `key = "value"` and `key = 123` and `key = true/false` pairs
    - Map to `slit_config` fields by section + key
    - Only flat key-value pairs (no nested tables)
  - Config path resolution:
    - `$XDG_CONFIG_HOME/slit/config.toml` if `XDG_CONFIG_HOME` set
    - Else `$HOME/.config/slit/config.toml`
    - `getenv("XDG_CONFIG_HOME")`, `getenv("HOME")`
  - Merge priority: built-in defaults → config file → CLI flags
  - Implementation: load config file first, then CLI flags overwrite
  - Missing config file is not an error
- `c/test/test_config.c`:
  - Test parsing `[display]` section with string/int/bool values
  - Test merge: config sets layout=box, CLI sets --rounded → result is rounded
  - Test missing file returns defaults
  - Test malformed file doesn't crash
- **Test**: `make test_config && ./test_config` → all pass; write temp config, verify `./slit` picks it up

## Step 23: Done summary + clean exit

**Goal**: EOF shows summary, terminal fully restored, correct exit codes.

- EOF handling in event loop:
  - Detect EOF: `getline()` returns -1, or `read()` returns 0
  - Set `eof_reached = 1`
  - Render final frame: `"Done. (N lines) [===========]  q:quit"`
  - Progress bar shows full if applicable
  - Spinner freezes (or hides)
  - Brief pause: `poll()` with 100ms timeout so user sees summary
  - Then: shutdown sequence
- Shutdown sequence (called from all exit paths):
  1. Restore window title
  2. Restore terminal mode (`term_raw_restore()`)
  3. Close `/dev/tty` (`term_cleanup()`)
  4. Close tee file (`tee_close()`)
  5. Close debug log (`debug_close()`)
  6. Free ring buffer (`buffer_free()`)
- Exit codes:
  - 0: clean EOF or user quit via `q`
  - 1: error (invalid flags, terminal init failure, etc.)
  - Signal exit: restore terminal first, then re-raise
- `atexit()` handler for terminal restore (belt-and-suspenders)
- **Test**: `echo done | ./slit; echo $?` → 0; `./slit --bogus; echo $?` → 1

## Step 24: SIGTSTP suspend/resume

**Goal**: Ctrl-Z suspends slit cleanly, re-renders on resume.

- `c/terminal.c` — SIGTSTP/SIGCONT handlers:
  - SIGTSTP handler:
    1. Restore terminal mode
    2. Clear slit's rendered area (optional — just restore terminal)
    3. Set `sigtstp_flag = 1`
  - SIGCONT handler:
    1. Re-enter raw mode
    2. Re-fetch terminal size (may have changed)
    3. Set dirty flag → re-render
- Main loop checks `sigtstp_flag`:
  ```c
  if (sigtstp_flag) {
      term_raw_restore();
      raise(SIGSTOP);   /* actually suspend */
      /* resumes here after SIGCONT */
      sigtstp_flag = 0;
      term_raw_enter();
      /* SIGCONT handler already set dirty flag */
  }
  ```
- Alternative: use `sigaction` with `SA_RESTART` and handle in main loop
- After resume: full re-render since terminal content may be stale
- **Test**: Run `slit`, press Ctrl-Z, `fg` → slit re-renders correctly

## Step 25: Shell completions

**Goal**: `slit completion bash|zsh|fish` outputs valid completion scripts.

- `c/completion.h`:
  - `void completion_print(const char *shell)`
- `c/completion.c`:
  - Embed completion scripts as string literals (`static const char bash_completion[] = ...`)
  - Three scripts: bash, zsh, fish
  - Hand-written scripts matching the flag definitions in cli.md
  - Enum completions: `--color` → `auto always never`, etc.
  - File path completions: `-o`, `--log-file`
  - `completion_print()`: strcmp shell name, fwrite appropriate script to stdout
- `main.c`: detect `argv[1] == "completion"`, call `completion_print(argv[2])`
- Error on unknown shell: print `"supported: bash, zsh, fish"` to stderr, exit 1
- **Test**: `./slit completion bash | head -5` produces bash script; `./slit completion bogus` exits 1

## Step 26: Integration tests + Makefile polish

**Goal**: Build, test, and valgrind infrastructure wired up. All integration tests pass.

- Makefile polish:
  - `make` / `make build`: produces `c/slit`
  - `make test`: compiles and runs all `test/test_*.c` binaries
  - `make valgrind`: runs tests under valgrind
  - `make clean`: remove objects and binary
  - `make install`: copy to `/usr/local/bin`
  - Object file compilation: each `.c` → `.o`, link into `slit`
  - Test compilation: `test/test_%.c` + source files → `test/test_%`
- Test framework: minimal hand-rolled assertions in `c/test/test.h`:
  ```c
  #define ASSERT(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while(0)
  #define ASSERT_STREQ(a, b) ASSERT(strcmp((a), (b)) == 0)
  #define TEST_RESULT() return failures > 0 ? 1 : 0
  ```
- Existing tests: `test_buffer.c`, `test_render.c`, `test_config.c`
- Valgrind suppression file for known libc noise (optional)
- Verify all test cases from testing.md pass with `SLIT_BIN=c/slit`
- **Test**: `make test && make valgrind && SLIT_BIN=c/slit bats tests/integration`

---

## Dependency Graph

```
Step 1 (Scaffold) ──► Step 2 (CLI) ──► Step 3 (Buffer)
                                              │
                                              ▼
                                       Step 4 (Terminal) ──► Step 5 (Signals)
                                                                  │
                                                                  ▼
                                                         Step 6 (Event Loop)
                                                                  │
                                                     ┌────────────┤
                                                     ▼            │
                                              Step 7 (Trim)       │
                                                     │            │
                                                     ▼            │
                                         Step 8 (Layout: min+none)
                                                     │
                                          ┌──────────┼──────────┐
                                          ▼                     ▼
                                  Step 9 (Box+Rounded)   Step 10 (Compact+Quote)
                                          │                     │
                                          └──────────┬──────────┘
                                                     ▼
                                              Step 11 (Spinner)
                                                     │
                                                     ▼
                                              Step 12 (Status Line) ──► Step 13 (Progress)
                                                     │
                                ┌────────────────────┼──────────────────┐
                                ▼                    ▼                  ▼
                         Step 14 (LineNum)   Step 15 (ANSI)    Step 16 (Timestamp)
                                │                    │                  │
                                └────────┬───────────┘                  │
                                         ▼                              │
                                  Step 17 (Wrap) ◄──────────────────────┘
                                         │
                                         ▼
                                  Step 18 (Tee)
                                         │
                              ┌──────────┼──────────┐
                              ▼          ▼          ▼
                      Step 19 (Title) Step 20 (Debounce) Step 21 (Debug)
                              │          │          │
                              └──────────┼──────────┘
                                         ▼
                                  Step 22 (Config)
                                         │
                                         ▼
                                  Step 23 (Exit/Cleanup)
                                         │
                                         ▼
                                  Step 24 (SIGTSTP)
                                         │
                                         ▼
                                  Step 25 (Completions)
                                         │
                                         ▼
                                  Step 26 (Integration)
```

Steps 9 and 10 are independent of each other (can be done in either order) after step 8.
Steps 14, 15, 16 are independent of each other and can be done in any order after step 12.
Steps 19, 20, 21 are independent of each other and can be done in any order after step 18.

---

## C-Specific Implementation Notes

### Memory Management
- Every `strdup()` must have a matching `free()`
- `buffer_free()` must free each entry's text string, then the entries array
- Wrap mode allocations: caller frees the array of wrapped line strings
- Tee writer: file closed in shutdown, even on error paths

### Error Handling
- System calls: check return values, `perror()` on failure
- `malloc()`/`calloc()`: check for NULL
- `getline()`: returns -1 on EOF/error, free line buffer
- Terminal init failure: print error to stderr, exit 1

### Portability
- POSIX.1-2001 minimum (Linux, macOS, BSD)
- No GNU extensions (`#ifdef __linux__` for Linux-specific if needed)
- Use `_POSIX_C_SOURCE 200112L` or `_DEFAULT_SOURCE`
- `ioctl(TIOCGWINSZ)` is not POSIX but universally available; fallback: environment `COLUMNS`/`LINES`

### UTF-8 Handling
- Truncation must respect UTF-8 boundaries (don't split multi-byte chars)
- `trim_line()`: walk bytes, detect continuation bytes (`(c & 0xC0) == 0x80`)
- Truncation char `…` is 3 bytes (UTF-8: `\xE2\x80\xA6`)

### Debugging Tips
- Compile with `-fsanitize=address,undefined` during development
- Use valgrind to catch leaks: `valgrind --leak-check=full ./slit`
- Debug logging (`--debug`) writes to `/tmp/slit-$PID.log` for runtime diagnostics
- Test with `SLIT_FORCE_RENDER=1` to test rendering in non-tty environments
