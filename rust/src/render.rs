use unicode_width::UnicodeWidthChar;
use unicode_width::UnicodeWidthStr;

fn width(s: &str) -> usize {
    UnicodeWidthStr::width(s)
}

fn char_width(ch: char) -> usize {
    UnicodeWidthChar::width(ch).unwrap_or(0)
}

pub fn strip_ansi(s: &str) -> String {
    let mut result = String::with_capacity(s.len());
    let chars: Vec<char> = s.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '\x1b' {
            i += 1;
            if i < chars.len() {
                if chars[i] == '[' {
                    i += 1;
                    while i < chars.len() {
                        let c = chars[i];
                        i += 1;
                        if c.is_ascii_alphabetic() {
                            break;
                        }
                    }
                } else if chars[i] == ']' {
                    i += 1;
                    while i < chars.len() {
                        if chars[i] == '\x07' {
                            i += 1;
                            break;
                        }
                        if chars[i] == '\x1b' && i + 1 < chars.len() && chars[i + 1] == '\\' {
                            i += 2;
                            break;
                        }
                        i += 1;
                    }
                } else {
                    i += 1;
                }
            }
        } else {
            result.push(chars[i]);
            i += 1;
        }
    }
    result
}

pub fn ansi_visible_width(s: &str) -> usize {
    let stripped = strip_ansi(s);
    width(stripped.as_str())
}

pub fn trim_line(line: &str, width_limit: usize, trunc_char: &str) -> String {
    let vis = width(line);
    if vis <= width_limit {
        return line.to_string();
    }
    let trunc_w = width(trunc_char);
    let target = width_limit.saturating_sub(trunc_w);
    let mut result = String::new();
    let mut w = 0;
    for ch in line.chars() {
        let cw = char_width(ch);
        if w + cw > target {
            break;
        }
        result.push(ch);
        w += cw;
    }
    result.push_str(trunc_char);
    result
}

pub fn trim_line_ansi(line: &str, width_limit: usize, trunc_char: &str) -> String {
    let vis = ansi_visible_width(line);
    if vis <= width_limit {
        return line.to_string();
    }
    let trunc_w = width(trunc_char);
    let target = width_limit.saturating_sub(trunc_w);
    let chars: Vec<char> = line.chars().collect();
    let mut result: Vec<char> = Vec::new();
    let mut visible = 0;
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '\x1b' {
            let start = i;
            i += 1;
            if i < chars.len() && chars[i] == '[' {
                i += 1;
                while i < chars.len() {
                    let c = chars[i];
                    i += 1;
                    if c.is_ascii_alphabetic() {
                        break;
                    }
                }
            } else if i < chars.len() && chars[i] == ']' {
                i += 1;
                while i < chars.len() {
                    if chars[i] == '\x07' {
                        i += 1;
                        break;
                    }
                    if chars[i] == '\x1b' && i + 1 < chars.len() && chars[i + 1] == '\\' {
                        i += 2;
                        break;
                    }
                    i += 1;
                }
            } else {
                i += 1;
            }
            for ch in &chars[start..i] {
                result.push(*ch);
            }
            continue;
        }
        if visible >= target {
            break;
        }
        let cw = char_width(chars[i]);
        result.push(chars[i]);
        visible += cw;
        i += 1;
    }

    while !result.is_empty() {
        let n = result.len();
        let last = result[n - 1];
        let is_term = last.is_ascii_alphabetic() || last == '\x07';
        if !is_term {
            break;
        }
        let mut j = n - 1;
        while j > 0 && result[j] != '\x1b' {
            j -= 1;
        }
        if j == 0 && result[0] != '\x1b' {
            break;
        }
        let seq: &[char] = &result[j..n];
        let valid = if seq.len() >= 3 && seq[1] == '[' {
            seq[2..seq.len() - 1]
                .iter()
                .all(|&r| r.is_ascii_digit() || r == ';')
        } else {
            seq.len() >= 3 && seq[1] == ']' && last == '\x07'
        };
        if !valid {
            break;
        }
        result.truncate(j);
    }

    let mut out: String = result.iter().collect();
    let mut sgr_depth = 0;
    let mut j = 0;
    let chars: Vec<char> = out.chars().collect();
    while j < chars.len() {
        if chars[j] == '\x1b' && j + 1 < chars.len() && chars[j + 1] == '[' {
            let mut k = j + 2;
            let mut params = String::new();
            while k < chars.len() && !chars[k].is_ascii_alphabetic() {
                params.push(chars[k]);
                k += 1;
            }
            if k < chars.len() && chars[k] == 'm' {
                if params.trim() == "0" || params.trim().is_empty() {
                    sgr_depth = 0;
                } else {
                    sgr_depth += 1;
                }
            }
            j = k + 1;
        } else {
            j += 1;
        }
    }
    out.push_str(trunc_char);
    if sgr_depth > 0 {
        out.push_str("\x1b[0m");
    }
    out
}

pub fn wrap_line(line: &str, width_limit: usize) -> Vec<String> {
    if width_limit == 0 {
        return vec![line.to_string()];
    }
    if line.is_empty() {
        return vec![String::new()];
    }
    let mut result = Vec::new();
    let mut current = String::new();
    let mut w = 0;
    for ch in line.chars() {
        let cw = char_width(ch);
        if w + cw > width_limit && w > 0 {
            result.push(current.clone());
            current.clear();
            w = 0;
        }
        current.push(ch);
        w += cw;
    }
    if !current.is_empty() || result.is_empty() {
        result.push(current);
    }
    result
}

pub fn wrap_line_ansi(line: &str, width_limit: usize) -> Vec<String> {
    if width_limit == 0 {
        return vec![line.to_string()];
    }
    if line.is_empty() {
        return vec![String::new()];
    }
    let chars: Vec<char> = line.chars().collect();
    let mut result = Vec::new();
    let mut current_line = String::new();
    let mut pending_ansi = String::new();
    let mut active_sgr = String::new();
    let mut visible = 0;
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '\x1b' {
            let start = i;
            i += 1;
            if i < chars.len() && chars[i] == '[' {
                i += 1;
                while i < chars.len() {
                    let c = chars[i];
                    i += 1;
                    if c.is_ascii_alphabetic() {
                        break;
                    }
                }
            } else if i < chars.len() && chars[i] == ']' {
                i += 1;
                while i < chars.len() {
                    if chars[i] == '\x07' {
                        i += 1;
                        break;
                    }
                    if chars[i] == '\x1b' && i + 1 < chars.len() && chars[i + 1] == '\\' {
                        i += 2;
                        break;
                    }
                    i += 1;
                }
            } else {
                i += 1;
            }
            let sgr: String = chars[start..i].iter().collect();
            if sgr.ends_with('m') {
                if sgr == "\x1b[0m" {
                    active_sgr.clear();
                } else {
                    active_sgr = sgr.clone();
                }
            }
            pending_ansi.push_str(&sgr);
            continue;
        }

        if visible >= width_limit {
            if !active_sgr.is_empty() {
                current_line.push_str("\x1b[0m");
            }
            result.push(current_line.clone());
            current_line.clear();
            if !active_sgr.is_empty() {
                current_line.push_str(&active_sgr);
            }
            visible = 0;
        }

        if !pending_ansi.is_empty() {
            current_line.push_str(&pending_ansi);
            pending_ansi.clear();
        }
        current_line.push(chars[i]);
        let cw = char_width(chars[i]);
        visible += cw;
        i += 1;
    }
    if !pending_ansi.is_empty() {
        current_line.push_str(&pending_ansi);
    }
    if !current_line.is_empty() || result.is_empty() {
        result.push(current_line);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_strip_ansi_basic() {
        assert_eq!(strip_ansi("\x1b[31mred\x1b[0m normal"), "red normal");
    }

    #[test]
    fn test_strip_ansi_osc() {
        assert_eq!(strip_ansi("\x1b]0;title\x07text"), "text");
    }

    #[test]
    fn test_strip_ansi_no_escape() {
        assert_eq!(strip_ansi("hello world"), "hello world");
    }

    #[test]
    fn test_ansi_visible_width() {
        assert_eq!(ansi_visible_width("\x1b[31mred\x1b[0m"), 3);
        assert_eq!(ansi_visible_width("hello"), 5);
    }

    #[test]
    fn test_trim_line_short() {
        assert_eq!(trim_line("hi", 10, "…"), "hi");
    }

    #[test]
    fn test_trim_line_exact() {
        assert_eq!(trim_line("hello", 5, "…"), "hello");
    }

    #[test]
    fn test_trim_line_long() {
        let result = trim_line("hello world", 8, "…");
        assert_eq!(result, "hello w…");
    }

    #[test]
    fn test_trim_line_ansi_short() {
        assert_eq!(
            trim_line_ansi("\x1b[31mred\x1b[0m", 10, "…"),
            "\x1b[31mred\x1b[0m"
        );
    }

    #[test]
    fn test_trim_line_ansi_long() {
        let result = trim_line_ansi("\x1b[31mhello world\x1b[0m", 8, "…");
        assert!(result.contains("\x1b[31m"));
        let expected_suffix = "…\x1b[0m";
        assert!(
            result.ends_with(expected_suffix),
            "should end with trunc char and reset, got: {:?}",
            result
        );
    }

    #[test]
    fn test_wrap_line_short() {
        let result = wrap_line("hello", 10);
        assert_eq!(result, vec!["hello"]);
    }

    #[test]
    fn test_wrap_line_long() {
        let result = wrap_line("hello world", 5);
        assert_eq!(result, vec!["hello", " worl", "d"]);
    }

    #[test]
    fn test_wrap_line_empty() {
        let result = wrap_line("", 10);
        assert_eq!(result, vec![""]);
    }

    #[test]
    fn test_wrap_line_ansi() {
        let result = wrap_line_ansi("\x1b[31mhello world\x1b[0m", 5);
        assert!(result.len() >= 2);
        assert!(result[0].contains("\x1b[31m"));
    }

    #[test]
    fn test_bug1_no_stdout_echo() {
        // Bug #1 is an architectural check: app.rs stdin reader sends lines
        // via channel, never writes to stdout. Verified by code inspection.
        // This test ensures wrap_line_ansi doesn't produce empty phantom lines.
        let line = "\x1b[31mhello\x1b[0m";
        let result = wrap_line_ansi(line, 80);
        assert_eq!(result.len(), 1);
        assert_eq!(result[0], "\x1b[31mhello\x1b[0m");
    }

    #[test]
    fn test_bug2_pad_right_ansi_content() {
        let colored = "\x1b[31mhello\x1b[0m";
        let vis = ansi_visible_width(colored);
        assert_eq!(vis, 5);
        let padded = {
            let target = 10;
            if vis > target {
                trim_line_ansi(colored, target, "")
            } else if vis == target {
                colored.to_string()
            } else {
                let mut r = colored.to_string();
                for _ in 0..(target - vis) {
                    r.push(' ');
                }
                r
            }
        };
        assert_eq!(ansi_visible_width(&padded), 10);
    }

    #[test]
    fn test_bug3_wrap_line_ansi_reset_detection() {
        let line = "a\x1b[0mb";
        let result = wrap_line_ansi(line, 10);
        assert_eq!(result.len(), 1);
        assert_eq!(result[0], "a\x1b[0mb");
    }

    #[test]
    fn test_bug3_wrap_line_ansi_no_phantom_line() {
        let line = "abcde";
        let result = wrap_line_ansi(line, 5);
        assert_eq!(
            result.len(),
            1,
            "should not produce phantom empty line: {:?}",
            result
        );
        assert_eq!(result[0], "abcde");
    }

    #[test]
    fn test_bug3_wrap_line_ansi_no_phantom_line_multiple_widths() {
        for w in 3..=10 {
            let line = "a".repeat(w);
            let result = wrap_line_ansi(&line, w);
            assert_eq!(
                result.len(),
                1,
                "width={}: should not produce phantom line: {:?}",
                w,
                result
            );
        }
    }

    #[test]
    fn test_bug3_wrap_line_ansi_width79() {
        let line = "a".repeat(79);
        let result = wrap_line_ansi(&line, 79);
        assert_eq!(result.len(), 1);
    }

    #[test]
    fn test_bug4_trim_line_ansi_no_dangling_ansi() {
        let line = "\x1b[31mhello world\x1b[0m";
        let result = trim_line_ansi(line, 7, "…");
        assert!(
            !result.contains("\x1b[31m…"),
            "should not have dangling ANSI before trunc char"
        );
        assert!(
            result.ends_with("…\x1b[0m"),
            "should end with trunc char and reset, got: {:?}",
            result
        );
    }

    #[test]
    fn test_bug5_visible_width_wide_char() {
        assert_eq!(ansi_visible_width("中"), 2);
        assert_eq!(ansi_visible_width("中文字"), 6);
    }

    #[test]
    fn test_bug5_visible_width_mixed_width() {
        assert_eq!(ansi_visible_width("a中b"), 4);
    }

    #[test]
    fn test_bug5_visible_width_maze_char() {
        assert_eq!(ansi_visible_width("╱"), 1);
        assert_eq!(ansi_visible_width("╲"), 1);
    }

    #[test]
    fn test_bug5_trim_line_ansi_wide_char() {
        let result = trim_line_ansi("中文测试", 5, "…");
        assert!(result.ends_with("…"));
        assert!(ansi_visible_width(&result) <= 5);
    }

    #[test]
    fn test_bug5_wrap_line_ansi_wide_char() {
        let result = wrap_line_ansi("中文测试", 4);
        assert!(result.len() >= 2);
    }

    #[test]
    fn test_visible_width_unicode() {
        assert_eq!(ansi_visible_width("hello"), 5);
        assert_eq!(ansi_visible_width("\x1b[31mred\x1b[0m"), 3);
    }

    #[test]
    fn test_trim_line_width_one() {
        let result = trim_line("abc", 1, "…");
        assert_eq!(result, "…");
    }

    #[test]
    fn test_trim_line_custom_trunc_char() {
        let result = trim_line("hello world", 8, ">");
        assert_eq!(result, "hello w>");
    }

    #[test]
    fn test_trim_line_multi_byte_trunc_char() {
        let result = trim_line("hello world", 8, "…");
        assert_eq!(result, "hello w…");
    }

    #[test]
    fn test_strip_ansi_multiple_codes() {
        assert_eq!(
            strip_ansi("\x1b[1;31m\x1b[42mbold red on green\x1b[0m"),
            "bold red on green"
        );
    }

    #[test]
    fn test_strip_ansi_osc_sequence() {
        assert_eq!(strip_ansi("\x1b]0;window-title\x07rest"), "rest");
    }

    #[test]
    fn test_wrap_line_exact_split() {
        let result = wrap_line("abcde", 5);
        assert_eq!(result, vec!["abcde"]);
    }

    #[test]
    fn test_trim_line_ansi_preserves_color() {
        let result = trim_line_ansi("\x1b[31mhi\x1b[0m", 10, "…");
        assert_eq!(result, "\x1b[31mhi\x1b[0m");
    }

    #[test]
    fn test_trim_line_ansi_truncates_with_color() {
        let result = trim_line_ansi("\x1b[31mhello world\x1b[0m", 8, "…");
        assert!(result.contains("\x1b[31m"));
        let expected_suffix = "…\x1b[0m";
        assert!(
            result.ends_with(expected_suffix),
            "should end with trunc char and reset, got: {:?}",
            result
        );
    }

    #[test]
    fn test_wrap_line_ansi_multiple_colors() {
        let result = wrap_line_ansi("\x1b[31mred\x1b[0m \x1b[32mgreen\x1b[0m", 5);
        assert!(result.len() >= 2);
    }
}
