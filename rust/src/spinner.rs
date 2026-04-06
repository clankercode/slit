use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};

pub static SPINNER_BRAILLE: &[&str] = &["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
pub static SPINNER_DOTS: &[&str] = &["⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"];
pub static SPINNER_ARROWS: &[&str] = &["←", "↖", "↑", "↗", "→", "↘", "↓", "↙"];

pub fn get_spinner_frame(style: &str, frame: usize) -> &'static str {
    let frames = match style {
        "braille" => SPINNER_BRAILLE,
        "dots" => SPINNER_DOTS,
        "arrows" => SPINNER_ARROWS,
        _ => return "",
    };
    if frames.is_empty() {
        return "";
    }
    frames[frame % frames.len()]
}

pub fn format_status_line(
    spinner_style: &str,
    frame: usize,
    eof: bool,
    line_count: u64,
    total_bytes: u64,
    file_size: u64,
    width: usize,
) -> String {
    let spinner = get_spinner_frame(spinner_style, frame);

    let progress_part = if file_size > 0 {
        let bar_width: usize = 10;
        let filled =
            ((bar_width as f64) * (total_bytes as f64) / (file_size as f64)).floor() as usize;
        let filled = filled.min(bar_width);
        let empty = bar_width - filled;
        format!(" [{}{}]", "=".repeat(filled), " ".repeat(empty))
    } else {
        String::new()
    };

    let keys_part = if spinner_style != "off" {
        "  q:quit"
    } else {
        ""
    };

    let line = if eof {
        format!("Done. ({} lines){}{}", line_count, progress_part, keys_part)
    } else {
        format!(
            "{} Streaming... ({} lines){}{}",
            spinner, line_count, progress_part, keys_part
        )
    };

    if width == 0 {
        return line;
    }

    let line_width = UnicodeWidthStr::width(line.as_str());
    if line_width > width {
        let mut result = String::new();
        let mut w = 0;
        for ch in line.chars() {
            let cw = UnicodeWidthChar::width(ch).unwrap_or(0);
            if w + cw > width {
                break;
            }
            result.push(ch);
            w += cw;
        }
        result
    } else if line_width < width {
        let mut result = line;
        for _ in 0..(width - line_width) {
            result.push(' ');
        }
        result
    } else {
        line
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_spinner_frame_braille() {
        assert_eq!(get_spinner_frame("braille", 0), "⠋");
        assert_eq!(get_spinner_frame("braille", 10), "⠋");
        assert_eq!(get_spinner_frame("braille", 1), "⠙");
    }

    #[test]
    fn test_spinner_frame_dots() {
        assert_eq!(get_spinner_frame("dots", 0), "⣾");
    }

    #[test]
    fn test_spinner_frame_off() {
        assert_eq!(get_spinner_frame("off", 0), "");
    }

    #[test]
    fn test_spinner_frame_unknown() {
        assert_eq!(get_spinner_frame("unknown", 0), "");
    }

    #[test]
    fn test_format_status_streaming() {
        let status = format_status_line("braille", 0, false, 42, 0, 0, 80);
        assert!(status.contains("Streaming..."));
        assert!(status.contains("42 lines"));
        assert!(status.contains("⠋"));
        assert!(status.contains("q:quit"));
    }

    #[test]
    fn test_format_status_done() {
        let status = format_status_line("braille", 0, true, 100, 0, 0, 80);
        assert!(status.contains("Done."));
        assert!(status.contains("100 lines"));
    }

    #[test]
    fn test_format_status_progress() {
        let status = format_status_line("braille", 0, false, 10, 50, 100, 80);
        assert!(status.contains("[=====     ]"));
    }
}
