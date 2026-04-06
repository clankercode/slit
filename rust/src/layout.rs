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
        Layout::Quote => render_quote_layout(title, content_lines, status),
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

fn render_quote_layout(title: &str, content_lines: &[String], status: &str) -> Vec<String> {
    let mut lines = Vec::new();
    lines.push(format!("▌ {}", title));
    for line in content_lines {
        lines.push(format!("▌ {}", line));
    }
    lines.push(format!("▌ {}", status));
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
        let result = render_layout(Layout::Minimal, "slit", &lines, "status", 80);
        assert_eq!(result.len(), 3);
        assert_eq!(result[0], "line1");
        assert_eq!(result[1], "line2");
        assert_eq!(result[2], "status");
    }
}
