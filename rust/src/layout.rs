#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ChromeCost {
    pub top_lines: u16,
    pub bottom_lines: u16,
    pub side_width: u16,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Layout {
    Box,
    Rounded,
    Compact,
    Minimal,
    None,
    Quote,
}

impl Layout {
    pub fn parse(s: &str) -> Option<Self> {
        match s {
            "box" => Some(Self::Box),
            "rounded" => Some(Self::Rounded),
            "compact" => Some(Self::Compact),
            "minimal" => Some(Self::Minimal),
            "none" => Some(Self::None),
            "quote" => Some(Self::Quote),
            _ => None,
        }
    }

    pub fn chrome_cost(&self) -> ChromeCost {
        match self {
            Self::Box => ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 4,
            },
            Self::Rounded => ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 4,
            },
            Self::Compact => ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 0,
            },
            Self::Minimal => ChromeCost {
                top_lines: 0,
                bottom_lines: 1,
                side_width: 0,
            },
            Self::None => ChromeCost {
                top_lines: 0,
                bottom_lines: 0,
                side_width: 0,
            },
            Self::Quote => ChromeCost {
                top_lines: 0,
                bottom_lines: 0,
                side_width: 2,
            },
        }
    }

    pub fn name(&self) -> &'static str {
        match self {
            Self::Box => "box",
            Self::Rounded => "rounded",
            Self::Compact => "compact",
            Self::Minimal => "minimal",
            Self::None => "none",
            Self::Quote => "quote",
        }
    }
}

pub fn render_layout(
    layout: Layout,
    title: &str,
    content_lines: &[String],
    status: &str,
    width: u16,
    quote_bg: &str,
) -> Vec<String> {
    match layout {
        Layout::Box => render_box_layout(
            title,
            content_lines,
            status,
            width,
            "┌",
            "┐",
            "└",
            "┘",
            "─",
            "│",
        ),
        Layout::Rounded => render_box_layout(
            title,
            content_lines,
            status,
            width,
            "╭",
            "╮",
            "╰",
            "╯",
            "─",
            "│",
        ),
        Layout::Compact => render_compact_layout(title, content_lines, status, width),
        Layout::Minimal => render_minimal_layout(content_lines, status),
        Layout::None => content_lines.to_vec(),
        Layout::Quote => render_quote_layout(title, content_lines, status, width, quote_bg),
    }
}

fn pad_right(s: &str, target_width: usize) -> String {
    let vis = crate::render::ansi_visible_width(s);
    if vis > target_width {
        crate::render::trim_line_ansi(s, target_width, "")
    } else if vis == target_width {
        s.to_string()
    } else {
        let mut result = s.to_string();
        for _ in 0..(target_width - vis) {
            result.push(' ');
        }
        result
    }
}

#[allow(clippy::too_many_arguments)]
fn render_box_layout(
    title: &str,
    content_lines: &[String],
    status: &str,
    width: u16,
    tl: &str,
    tr: &str,
    bl: &str,
    br: &str,
    h: &str,
    v: &str,
) -> Vec<String> {
    let mut lines = Vec::new();
    let w = width as usize;
    let content_width = w.saturating_sub(4);
    let horiz_inner = w.saturating_sub(2);

    let title_part = if !title.is_empty() {
        format!(" {} ", title)
    } else {
        String::new()
    };
    let title_part_width = unicode_width::UnicodeWidthStr::width(title_part.as_str());
    let remaining = horiz_inner.saturating_sub(title_part_width);
    let mut top = String::new();
    top.push_str(tl);
    top.push_str(&title_part);
    for _ in 0..remaining {
        top.push_str(h);
    }
    top.push_str(tr);
    lines.push(top);

    for line in content_lines {
        let padded = pad_right(line, content_width);
        lines.push(format!("{} {} {}", v, padded, v));
    }

    let status_part = if !status.is_empty() {
        format!(" {} ", status)
    } else {
        String::new()
    };
    let status_part_width = unicode_width::UnicodeWidthStr::width(status_part.as_str());
    let remaining = horiz_inner.saturating_sub(status_part_width);
    let mut bottom = String::new();
    bottom.push_str(bl);
    bottom.push_str(&status_part);
    for _ in 0..remaining {
        bottom.push_str(h);
    }
    bottom.push_str(br);
    lines.push(bottom);

    lines
}

fn render_compact_layout(
    title: &str,
    content_lines: &[String],
    status: &str,
    width: u16,
) -> Vec<String> {
    let mut lines = Vec::new();
    let top_bar = format!(
        "\x1b[48;2;75;75;75m\x1b[38;2;224;224;224;1m{}\x1b[0m",
        pad_right(title, width as usize)
    );
    lines.push(top_bar);
    lines.extend(content_lines.iter().cloned());
    lines.push(status.to_string());
    lines
}

fn render_minimal_layout(content_lines: &[String], status: &str) -> Vec<String> {
    let mut lines: Vec<String> = content_lines.to_vec();
    lines.push(status.to_string());
    lines
}

fn render_quote_layout(
    title: &str,
    content_lines: &[String],
    status: &str,
    width: u16,
    quote_bg: &str,
) -> Vec<String> {
    let use_bg = quote_bg != "off" && quote_bg.starts_with('#') && quote_bg.len() >= 7;
    let (r, g, b) = if use_bg {
        let hex = &quote_bg[1..];
        let rv = u8::from_str_radix(&hex[0..2], 16).unwrap_or(0);
        let gv = u8::from_str_radix(&hex[2..4], 16).unwrap_or(0);
        let bv = u8::from_str_radix(&hex[4..6], 16).unwrap_or(0);
        (rv, gv, bv)
    } else {
        (0, 0, 0)
    };

    let bg_start = format!("\x1b[48;2;{};{};{}m", r, g, b);
    let bg_end = "\x1b[0m".to_string();
    let bar = "▌";
    let w = width as usize;

    let mut lines = Vec::new();

    let title_line = format!("{} {}", bar, title);
    if use_bg {
        let vis = crate::render::ansi_visible_width(&title_line);
        let pad = w.saturating_sub(vis);
        lines.push(format!(
            "{}{}{}{}",
            bg_start,
            title_line,
            " ".repeat(pad),
            bg_end
        ));
    } else {
        lines.push(title_line);
    }

    for line in content_lines {
        let content_line = format!("{} {}", bar, line);
        if use_bg {
            let vis = crate::render::ansi_visible_width(&content_line);
            let pad = w.saturating_sub(vis);
            lines.push(format!(
                "{}{}{}{}",
                bg_start,
                content_line,
                " ".repeat(pad),
                bg_end
            ));
        } else {
            lines.push(content_line);
        }
    }

    let status_line = format!("{} {}", bar, status);
    if use_bg {
        let vis = crate::render::ansi_visible_width(&status_line);
        let pad = w.saturating_sub(vis);
        lines.push(format!(
            "{}{}{}{}",
            bg_start,
            status_line,
            " ".repeat(pad),
            bg_end
        ));
    } else {
        lines.push(status_line);
    }

    lines
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_layout_from_str() {
        assert_eq!(Layout::parse("box"), Some(Layout::Box));
        assert_eq!(Layout::parse("rounded"), Some(Layout::Rounded));
        assert_eq!(Layout::parse("compact"), Some(Layout::Compact));
        assert_eq!(Layout::parse("minimal"), Some(Layout::Minimal));
        assert_eq!(Layout::parse("none"), Some(Layout::None));
        assert_eq!(Layout::parse("quote"), Some(Layout::Quote));
        assert_eq!(Layout::parse("invalid"), None);
    }

    #[test]
    fn test_chrome_costs() {
        assert_eq!(
            Layout::Box.chrome_cost(),
            ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 4
            }
        );
        assert_eq!(
            Layout::Rounded.chrome_cost(),
            ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 4
            }
        );
        assert_eq!(
            Layout::Compact.chrome_cost(),
            ChromeCost {
                top_lines: 1,
                bottom_lines: 1,
                side_width: 0
            }
        );
        assert_eq!(
            Layout::Minimal.chrome_cost(),
            ChromeCost {
                top_lines: 0,
                bottom_lines: 1,
                side_width: 0
            }
        );
        assert_eq!(
            Layout::None.chrome_cost(),
            ChromeCost {
                top_lines: 0,
                bottom_lines: 0,
                side_width: 0
            }
        );
        assert_eq!(
            Layout::Quote.chrome_cost(),
            ChromeCost {
                top_lines: 0,
                bottom_lines: 0,
                side_width: 2
            }
        );
    }

    #[test]
    fn test_minimal_layout_render() {
        let lines = vec!["line1".to_string(), "line2".to_string()];
        let result = render_layout(Layout::Minimal, "slit", &lines, "status", 80, "off");
        assert_eq!(result.len(), 3);
        assert_eq!(result[0], "line1");
        assert_eq!(result[1], "line2");
        assert_eq!(result[2], "status");
    }

    #[test]
    fn test_render_layout_box() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::Box, "slit", &lines, "ok", 20, "off");
        assert!(result[0].contains("┌"));
        assert!(result[0].contains("┐"));
        assert!(result.last().unwrap().contains("└"));
        assert!(result.last().unwrap().contains("┘"));
        assert!(result.iter().any(|l| l.contains("hello")));
    }

    #[test]
    fn test_render_layout_rounded() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::Rounded, "slit", &lines, "ok", 20, "off");
        assert!(result[0].contains("╭"));
        assert!(result[0].contains("╮"));
    }

    #[test]
    fn test_render_layout_none() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::None, "slit", &lines, "status", 20, "off");
        assert_eq!(result.len(), 1);
        assert_eq!(result[0], "hello");
    }

    #[test]
    fn test_render_layout_quote() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::Quote, "slit", &lines, "status", 20, "off");
        assert!(result.iter().any(|l| l.contains("▌")));
    }

    #[test]
    fn test_render_layout_compact() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::Compact, "slit", &lines, "status", 20, "off");
        assert!(result.len() >= 3);
        assert!(result.iter().any(|l| l.contains("hello")));
    }

    #[test]
    fn test_box_width_alignment() {
        let lines = vec!["hello".to_string()];
        let result = render_layout(Layout::Box, "slit", &lines, "ok", 20, "off");
        for line in &result {
            assert_eq!(
                unicode_width::UnicodeWidthStr::width(line.as_str()),
                20,
                "box line width mismatch: {:?}",
                line
            );
        }
    }

    #[test]
    fn test_pad_right_overflow() {
        let s = "hello world this is long";
        let result = pad_right(s, 10);
        assert!(crate::render::ansi_visible_width(&result) <= 10);
    }

    #[test]
    fn test_pad_right_exact() {
        let result = pad_right("hello", 5);
        assert_eq!(result, "hello");
    }

    #[test]
    fn test_pad_right_basic() {
        let result = pad_right("hi", 5);
        assert_eq!(result, "hi   ");
    }
}
