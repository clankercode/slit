use crate::buffer::LineEntry;
use crate::buffer::RingBuffer;
use crate::config::Config;
use crate::debug::DebugLogger;
use crate::layout;
use crate::layout::Layout;
use crate::render;
use crate::spinner;
use crate::tee::TeeWriter;
use crossterm::event::{Event, EventStream, KeyCode, KeyModifiers};
use crossterm::terminal::{disable_raw_mode, enable_raw_mode};
use crossterm::execute;
use futures::StreamExt;
use ratatui::backend::CrosstermBackend;
use tokio::io::AsyncBufReadExt;
use ratatui::text::Line as RatLine;
use ratatui::widgets::Paragraph;
use ratatui::Terminal;
use ratatui::TerminalOptions;
use ratatui::Viewport;
use std::io;
use std::time::Duration;
use tokio::signal::unix::{signal, SignalKind};

pub struct App {
    config: Config,
    buffer: RingBuffer,
    width: u16,
    height: u16,
    content_lines: usize,
    eof: bool,
    spinner_frame: usize,
    tick_count: usize,
    file_size: u64,
    is_stderr_tty: bool,
    tee_writer: Option<TeeWriter>,
    debug_logger: Option<DebugLogger>,
    dirty: bool,
    needs_reinit: bool,
}

struct RawModeGuard;

impl Drop for RawModeGuard {
    fn drop(&mut self) {
        let _ = disable_raw_mode();
        let _ = execute!(io::stderr(), crossterm::terminal::SetTitle(""));
    }
}

pub fn is_stderr_tty() -> bool {
    if std::env::var("SLIT_FORCE_RENDER").as_deref() == Ok("1") {
        return true;
    }
    unsafe { libc::isatty(libc::STDERR_FILENO) != 0 }
}

fn create_terminal(height: u16) -> anyhow::Result<Terminal<CrosstermBackend<io::Stderr>>> {
    let backend = CrosstermBackend::new(io::stderr());
    let terminal = Terminal::with_options(
        backend,
        TerminalOptions {
            viewport: Viewport::Inline(height),
        },
    )?;
    Ok(terminal)
}

impl App {
    pub fn new(config: Config) -> Self {
        let file_size = get_file_size();
        let is_stderr_tty = is_stderr_tty();
        let tee_writer = config.output.as_ref().and_then(|p| TeeWriter::new(p, config.append));
        let debug_logger = if config.debug {
            DebugLogger::new(config.log_file.as_deref())
        } else {
            None
        };

        Self {
            buffer: RingBuffer::new(config.max_lines),
            config,
            width: 0,
            height: 0,
            content_lines: 10,
            eof: false,
            spinner_frame: 0,
            tick_count: 0,
            file_size,
            is_stderr_tty,
            tee_writer,
            debug_logger,
            dirty: false,
            needs_reinit: false,
        }
    }

    fn viewport_height(&self) -> u16 {
        let layout = self.get_layout();
        let chrome = layout.chrome_cost();
        (self.content_lines as u16 + chrome.top_lines + chrome.bottom_lines).max(1)
    }

    pub async fn run(&mut self) -> anyhow::Result<()> {
        let (tx, mut rx) = tokio::sync::mpsc::channel::<Option<String>>(1024);

        let stdin_tx = tx.clone();
        tokio::spawn(async move {
            let reader = tokio::io::BufReader::new(tokio::io::stdin());
            let mut lines = reader.lines();
            loop {
                match lines.next_line().await {
                    Ok(Some(line)) => {
                        if stdin_tx.send(Some(line)).await.is_err() {
                            break;
                        }
                    }
                    Ok(None) => {
                        let _ = stdin_tx.send(None).await;
                        break;
                    }
                    Err(_) => {
                        let _ = stdin_tx.send(None).await;
                        break;
                    }
                }
            }
        });

        drop(tx);

        let _guard = RawModeGuard;
        enable_raw_mode()?;

        let (w, h) = crossterm::terminal::size()?;
        self.width = w;
        self.height = h;
        self.update_content_lines();

        let mut terminal = create_terminal(self.viewport_height())?;

        let should_set_title = self.is_stderr_tty && self.config.layout != "none";
        if should_set_title {
            execute!(io::stderr(), crossterm::terminal::SetTitle("slit"))?;
        }

        if let Some(ref dl) = self.debug_logger {
            dl.log(&format!(
                "init: fileSize={}, layout={}, lines={}",
                self.file_size, self.config.layout, self.config.lines
            ));
        }

        let mut events = EventStream::new();
        let mut tick = tokio::time::interval(Duration::from_millis(10));
        tick.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);

        let mut sigtstp_stream = signal(SignalKind::from_raw(libc::SIGTSTP)).ok();

        loop {
            tokio::select! {
                line = rx.recv() => {
                    match line {
                        Some(Some(text)) => {
                            self.handle_line(text);
                            self.dirty = true;
                        }
                        Some(None) | None => {
                            if !self.eof {
                                self.handle_eof();
                                self.dirty = true;
                            }
                        }
                    }
                }
                event = events.next() => {
                    if let Some(Ok(event)) = event {
                        match event {
                            Event::Key(key) => {
                                if key.code == KeyCode::Char('q') || key.code == KeyCode::Char('Q')
                                    || (key.code == KeyCode::Char('c')
                                        && key.modifiers.contains(KeyModifiers::CONTROL))
                                {
                                    self.close_tee();
                                    return Ok(());
                                }
                            }
                            Event::Resize(w, h) => {
                                self.width = w;
                                self.height = h;
                                self.update_content_lines();
                                self.needs_reinit = true;
                                self.dirty = true;
                            }
                            _ => {}
                        }
                    }
                }
                _ = tick.tick() => {
                    self.tick_count += 1;
                    if !self.eof && self.tick_count.is_multiple_of(10) {
                        self.spinner_frame += 1;
                    }
                    self.dirty = true;
                    if self.needs_reinit {
                        let new_h = self.viewport_height();
                        drop(terminal);
                        terminal = create_terminal(new_h)?;
                        self.needs_reinit = false;
                    }
                    if self.dirty {
                        if let Err(e) = self.do_render(&mut terminal) {
                            if let Some(ref dl) = self.debug_logger {
                                dl.log(&format!("render error: {}", e));
                            }
                        }
                        self.dirty = false;
                    }
                    if self.eof {
                        tokio::time::sleep(Duration::from_millis(200)).await;
                        return Ok(());
                    }
                }
                _ = async {
                    if let Some(ref mut s) = sigtstp_stream {
                        s.recv().await
                    } else {
                        std::future::pending().await
                    }
                } => {
                    drop(terminal);
                    let _ = disable_raw_mode();
                    unsafe { libc::raise(libc::SIGTSTP); }
                    enable_raw_mode()?;
                    terminal = create_terminal(self.viewport_height())?;
                    self.needs_reinit = false;
                    self.dirty = true;
                }
            }
        }
    }

    fn handle_line(&mut self, text: String) {
        if let Some(ref mut tw) = self.tee_writer {
            if self.config.tee_format == "raw" {
                tw.write_line(&text);
            }
        }

        let line_num = self.buffer.total_count() + 1;
        self.buffer.push(text.clone());

        if self.config.tee_format == "display" {
            let ts_prefix = if self.config.timestamp {
                if let Some(entry) = self.buffer.last_entry() {
                    let dt = chrono::DateTime::<chrono::Local>::from(entry.time);
                    format!("{} ", dt.format("%H:%M:%S"))
                } else {
                    String::new()
                }
            } else {
                String::new()
            };
            let ln_prefix = if self.config.line_numbers {
                let pad_w = self.line_num_pad_width();
                format!("{:>width$} ", line_num, width = pad_w)
            } else {
                String::new()
            };
            let line_text = if self.should_strip() {
                render::strip_ansi(&text)
            } else {
                text.clone()
            };
            let display = format!("{}{}{}", ts_prefix, ln_prefix, line_text);
            if let Some(ref mut tw) = self.tee_writer {
                tw.write_line(&display);
            }
        }

        if let Some(ref dl) = self.debug_logger {
            dl.log(&format!(
                "line: total={}, bytes={}",
                self.buffer.total_count(),
                self.buffer.total_bytes()
            ));
        }
    }

    fn handle_eof(&mut self) {
        self.eof = true;
        self.close_tee();
        if let Some(ref dl) = self.debug_logger {
            dl.log(&format!(
                "eof: totalLines={}, totalBytes={}",
                self.buffer.total_count(),
                self.buffer.total_bytes()
            ));
        }
    }

    fn close_tee(&mut self) {
        if let Some(ref mut tw) = self.tee_writer {
            tw.close();
        }
    }

    fn update_content_lines(&mut self) {
        if self.config.lines == 0 {
            let layout = self.get_layout();
            let chrome = layout.chrome_cost();
            let term_h = self.height as usize;
            let available = term_h
                .saturating_sub(chrome.top_lines as usize)
                .saturating_sub(chrome.bottom_lines as usize);
            self.content_lines = (available * 2 / 3).max(10);
        } else {
            self.content_lines = self.config.lines;
        }
    }

    fn get_layout(&self) -> Layout {
        Layout::parse(&self.config.layout).unwrap_or(Layout::Minimal)
    }

    fn gutter_width(&self) -> usize {
        let mut w = 0;
        if self.config.line_numbers {
            let max_num = self.buffer.total_count().max(1);
            w += digits(max_num) + 1;
        }
        if self.config.timestamp {
            w += 9;
        }
        w
    }

    fn line_num_pad_width(&self) -> usize {
        if !self.config.line_numbers {
            return 0;
        }
        let max_num = self.buffer.total_count().max(1);
        digits(max_num)
    }

    fn should_strip(&self) -> bool {
        self.config.color == "never" || (self.config.color == "auto" && !self.is_stderr_tty)
    }

    fn do_render(
        &self,
        terminal: &mut Terminal<CrosstermBackend<io::Stderr>>,
    ) -> anyhow::Result<()> {
        if self.width == 0 {
            return Ok(());
        }

        let layout = self.get_layout();
        let chrome = layout.chrome_cost();
        let content_width = (self.width as usize).saturating_sub(chrome.side_width as usize).max(1);
        let gutter_w = self.gutter_width();
        let data_width = content_width.saturating_sub(gutter_w).max(1);

        let mut data_lines = self.content_lines;
        if layout == Layout::Quote {
            data_lines = data_lines.saturating_sub(2);
        }

        let should_strip = self.should_strip();

        let entries: Vec<&LineEntry> = self.buffer.last_n(data_lines);
        let formatted = self.format_entries(&entries, data_width, should_strip);

        let display_lines: Vec<String> = if formatted.len() > data_lines {
            formatted[formatted.len() - data_lines..].to_vec()
        } else {
            formatted
        };

        let status = if layout != Layout::None {
            let status_width = if chrome.side_width > 0 {
                (self.width as usize).saturating_sub(chrome.side_width as usize)
            } else {
                content_width
            };
            spinner::format_status_line(
                &self.config.spinner,
                self.spinner_frame,
                self.eof,
                self.buffer.total_count(),
                self.buffer.total_bytes(),
                self.file_size,
                status_width,
            )
        } else {
            String::new()
        };

        let all_lines =
            layout::render_layout(layout, "slit", &display_lines, &status, self.width);

        let rat_lines: Vec<RatLine> = all_lines
            .iter()
            .map(|l| RatLine::from(l.as_str()))
            .collect();

        let paragraph = Paragraph::new(rat_lines);

        terminal.draw(|f| {
            let size = f.area();
            f.render_widget(paragraph, size);
        })?;

        Ok(())
    }

    fn format_entries(
        &self,
        entries: &[&LineEntry],
        data_width: usize,
        should_strip: bool,
    ) -> Vec<String> {
        let mut formatted = Vec::new();
        let pad_w = self.line_num_pad_width();

        for entry in entries {
            let line = if should_strip {
                render::strip_ansi(&entry.text)
            } else {
                entry.text.clone()
            };

            let sublines = if self.config.wrap {
                if should_strip {
                    render::wrap_line(&line, data_width)
                } else {
                    render::wrap_line_ansi(&line, data_width)
                }
            } else if should_strip {
                vec![render::trim_line(&line, data_width, &self.config.truncation_char)]
            } else {
                vec![render::trim_line_ansi(&line, data_width, &self.config.truncation_char)]
            };

            for (i, sub) in sublines.iter().enumerate() {
                let mut sb = String::new();
                if self.config.timestamp {
                    let dt = chrono::DateTime::<chrono::Local>::from(entry.time);
                    sb.push_str(&dt.format("%H:%M:%S").to_string());
                    sb.push(' ');
                }
                if self.config.line_numbers {
                    if i == 0 {
                        sb.push_str(&format!("{:>width$} ", entry.line_num, width = pad_w));
                    } else {
                        sb.push_str(&format!("{:>width$} ", "", width = pad_w));
                    }
                }
                sb.push_str(sub);
                formatted.push(sb);
            }
        }
        formatted
    }
}

fn digits(n: u64) -> usize {
    if n == 0 {
        return 1;
    }
    let mut count = 0;
    let mut n = n;
    while n > 0 {
        n /= 10;
        count += 1;
    }
    count
}

fn get_file_size() -> u64 {
    use std::os::unix::fs::MetadataExt;
    if let Ok(meta) = std::fs::metadata("/dev/stdin") {
        if meta.is_file() {
            return meta.size();
        }
    }
    0
}

pub async fn passthrough(config: &crate::config::Config) -> anyhow::Result<()> {
    use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};

    // Determine n: 0=pipe-all, positive=head+tail
    // If --lines/-n was not explicitly passed, default to 10 (head+tail mode).
    // If it was passed as 0, pipe everything through.
    // If it was passed as a positive number, use that for head+tail.
    let n: usize = if !config.lines_from_cli {
        10
    } else {
        config.lines
    };

    let mut tw = config.output.as_ref().and_then(|p| {
        crate::tee::TeeWriter::new(p, config.append)
    });

    let stdin = tokio::io::stdin();
    let reader = BufReader::new(stdin);
    let mut lines = reader.lines();
    let mut out = tokio::io::stdout();

    if n == 0 {
        // -n 0: pipe everything through
        while let Some(line) = lines.next_line().await? {
            out.write_all(line.as_bytes()).await?;
            out.write_all(b"\n").await?;
            if let Some(ref mut tw) = tw { tw.write_line(&line); }
        }
        out.flush().await?;
        if let Some(mut tw) = tw { tw.close(); }
        return Ok(());
    }

    // Phase 1: head — emit first n lines immediately
    let mut total: usize = 0;
    while total < n {
        match lines.next_line().await? {
            None => break,
            Some(line) => {
                out.write_all(line.as_bytes()).await?;
                out.write_all(b"\n").await?;
                out.flush().await?;
                if let Some(ref mut tw) = tw { tw.write_line(&line); }
                total += 1;
            }
        }
    }

    // Phase 2: circular tail buffer — keep last n lines from remainder
    let mut tail_buf: Vec<String> = vec![String::new(); n];
    let mut tail_head: usize = 0;
    let mut tail_count: usize = 0;

    while let Some(line) = lines.next_line().await? {
        total += 1;
        if let Some(ref mut tw) = tw { tw.write_line(&line); }
        if tail_count < n {
            tail_buf[(tail_head + tail_count) % n] = line;
            tail_count += 1;
        } else {
            tail_buf[tail_head] = line;
            tail_head = (tail_head + 1) % n;
        }
    }

    // Phase 3: separator + tail
    let omitted = total.saturating_sub(n).saturating_sub(tail_count);
    if omitted > 0 {
        let sep = format!("... [{} lines omitted] ...\n", omitted);
        out.write_all(sep.as_bytes()).await?;
    }
    for i in 0..tail_count {
        let idx = (tail_head + i) % n;
        out.write_all(tail_buf[idx].as_bytes()).await?;
        out.write_all(b"\n").await?;
    }

    out.flush().await?;
    if let Some(mut tw) = tw { tw.close(); }
    Ok(())
}
