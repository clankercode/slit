use serde::Deserialize;
use std::fs;
use std::path::PathBuf;

/// Configuration from TOML file
#[derive(Debug, Default, Deserialize)]
pub struct FileConfig {
    #[serde(default)]
    pub display: DisplayConfig,
    #[serde(default)]
    pub buffer: BufferConfig,
    #[serde(default)]
    pub spinner: SpinnerConfig,
    #[serde(default)]
    pub quote: QuoteConfig,
    #[serde(default)]
    pub debug: DebugConfig,
}

#[derive(Debug, Default, Deserialize)]
pub struct DisplayConfig {
    pub layout: Option<String>,
    pub lines: Option<usize>,
    #[serde(rename = "line_numbers")]
    pub line_numbers: Option<bool>,
    pub color: Option<String>,
    pub wrap: Option<bool>,
    pub timestamp: Option<bool>,
    #[serde(rename = "truncation_char")]
    pub truncation_char: Option<String>,
}

#[derive(Debug, Default, Deserialize)]
pub struct BufferConfig {
    #[serde(rename = "max_lines")]
    pub max_lines: Option<usize>,
}

#[derive(Debug, Default, Deserialize)]
pub struct SpinnerConfig {
    pub style: Option<String>,
}

#[derive(Debug, Default, Deserialize)]
pub struct QuoteConfig {
    pub bg: Option<String>,
}

#[derive(Debug, Default, Deserialize)]
pub struct DebugConfig {
    pub enabled: Option<bool>,
    #[serde(rename = "log_file")]
    pub log_file: Option<String>,
}

/// Runtime configuration after merging defaults, file, and CLI
#[derive(Debug, Clone)]
pub struct Config {
    pub lines: usize,
    pub max_lines: usize,
    pub output: Option<PathBuf>,
    pub append: bool,
    pub tee_format: String,
    pub line_numbers: bool,
    pub color: String,
    pub wrap: bool,
    pub timestamp: bool,
    pub truncation_char: String,
    pub layout: String,
    pub quote_bg: String,
    pub spinner: String,
    pub debug: bool,
    pub log_file: Option<PathBuf>,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            lines: 0,
            max_lines: 50000,
            output: None,
            append: false,
            tee_format: "raw".to_string(),
            line_numbers: false,
            color: "auto".to_string(),
            wrap: false,
            timestamp: false,
            truncation_char: "…".to_string(),
            layout: "minimal".to_string(),
            quote_bg: "off".to_string(),
            spinner: "braille".to_string(),
            debug: false,
            log_file: None,
        }
    }
}

impl Config {
    /// Load configuration from file and merge with defaults and CLI options
    pub fn resolve(cli: &crate::Cli) -> anyhow::Result<Self> {
        let mut config = Self::default();

        // Apply file config if present
        if let Some(file_cfg) = load_config_file()? {
            config = merge_file_config(config, file_cfg);
        }

        // Apply CLI overrides (CLI always wins)
        config.apply_cli(cli);

        if config.max_lines == 0 {
            anyhow::bail!("max_lines must be greater than 0");
        }

        Ok(config)
    }

    fn apply_cli(&mut self, cli: &crate::Cli) {
        if let Some(lines) = cli.lines {
            self.lines = lines;
        }
        if let Some(max_lines) = cli.max_lines {
            self.max_lines = max_lines;
        }
        if cli.output.is_some() {
            self.output = cli.output.clone();
        }
        if cli.append {
            self.append = cli.append;
        }
        if let Some(tee_format) = cli.tee_format {
            self.tee_format = format!("{:?}", tee_format).to_lowercase();
        }
        if cli.line_numbers {
            self.line_numbers = cli.line_numbers;
        }
        if let Some(color) = cli.color {
            self.color = format!("{:?}", color).to_lowercase();
        }
        if cli.wrap {
            self.wrap = cli.wrap;
        }
        if cli.timestamp {
            self.timestamp = cli.timestamp;
        }
        if let Some(ref truncation_char) = cli.truncation_char {
            self.truncation_char = truncation_char.clone();
        }

        if let Some(layout) = resolve_layout(cli) {
            self.layout = layout;
        }

        if let Some(quote_bg) = cli.quote_bg {
            self.quote_bg = format!("{:?}", quote_bg).to_lowercase();
        }
        if let Some(spinner) = cli.spinner {
            self.spinner = format!("{:?}", spinner).to_lowercase();
        }
        if cli.debug {
            self.debug = cli.debug;
        }
        if cli.log_file.is_some() {
            self.log_file = cli.log_file.clone();
        }
    }
}

fn resolve_layout(cli: &crate::Cli) -> Option<String> {
    if cli.box_layout {
        return Some("box".to_string());
    }
    if cli.rounded {
        return Some("rounded".to_string());
    }
    if cli.compact {
        return Some("compact".to_string());
    }
    if cli.minimal {
        return Some("minimal".to_string());
    }
    if cli.none {
        return Some("none".to_string());
    }
    if cli.quote {
        return Some("quote".to_string());
    }
    cli.layout.map(|l| format!("{:?}", l).to_lowercase())
}

/// Load config from file if it exists
pub fn load_config_file() -> anyhow::Result<Option<FileConfig>> {
    let config_path = find_config_path();

    if let Some(path) = config_path {
        if path.exists() {
            let contents = fs::read_to_string(&path)?;
            let config: FileConfig = toml::from_str(&contents)?;
            return Ok(Some(config));
        }
    }

    Ok(None)
}

/// Find config file path: $XDG_CONFIG_HOME/slit/config.toml or ~/.config/slit/config.toml
fn find_config_path() -> Option<PathBuf> {
    // Try $XDG_CONFIG_HOME first
    if let Ok(xdg_config) = std::env::var("XDG_CONFIG_HOME") {
        let path = PathBuf::from(xdg_config).join("slit").join("config.toml");
        return Some(path);
    }

    // Fall back to ~/.config
    if let Ok(home) = std::env::var("HOME") {
        let path = PathBuf::from(home)
            .join(".config")
            .join("slit")
            .join("config.toml");
        return Some(path);
    }

    None
}

/// Merge file config into base config
fn merge_file_config(base: Config, file: FileConfig) -> Config {
    let mut result = base;

    if let Some(lines) = file.display.lines {
        result.lines = lines;
    }
    if let Some(max_lines) = file.buffer.max_lines {
        result.max_lines = max_lines;
    }
    if let Some(line_numbers) = file.display.line_numbers {
        result.line_numbers = line_numbers;
    }
    if let Some(color) = file.display.color {
        result.color = color;
    }
    if let Some(wrap) = file.display.wrap {
        result.wrap = wrap;
    }
    if let Some(timestamp) = file.display.timestamp {
        result.timestamp = timestamp;
    }
    if let Some(truncation_char) = file.display.truncation_char {
        result.truncation_char = truncation_char;
    }
    if let Some(layout) = file.display.layout {
        result.layout = layout;
    }
    if let Some(style) = file.spinner.style {
        result.spinner = style;
    }
    if let Some(bg) = file.quote.bg {
        result.quote_bg = bg;
    }
    if let Some(enabled) = file.debug.enabled {
        result.debug = enabled;
    }
    if let Some(log_file) = file.debug.log_file {
        result.log_file = Some(PathBuf::from(log_file));
    }

    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_default_config() {
        let cfg = Config::default();
        assert_eq!(cfg.lines, 0);
        assert_eq!(cfg.max_lines, 50000);
        assert_eq!(cfg.layout, "minimal");
        assert_eq!(cfg.truncation_char, "…");
        assert!(!cfg.debug);
    }

    #[test]
    fn test_merge_file_config() {
        let base = Config::default();
        let file = FileConfig {
            display: DisplayConfig {
                layout: Some("box".to_string()),
                lines: Some(10),
                line_numbers: Some(true),
                color: None,
                wrap: None,
                timestamp: None,
                truncation_char: None,
            },
            buffer: BufferConfig {
                max_lines: Some(1000),
            },
            spinner: SpinnerConfig::default(),
            quote: QuoteConfig::default(),
            debug: DebugConfig::default(),
        };

        let merged = merge_file_config(base, file);
        assert_eq!(merged.lines, 10);
        assert_eq!(merged.max_lines, 1000);
        assert_eq!(merged.layout, "box");
        assert!(merged.line_numbers);
    }

    #[test]
    fn test_box_flag_name() {
        let cmd = <crate::Cli as clap::CommandFactory>::command();
        let box_arg = cmd
            .get_arguments()
            .find(|a| a.get_id() == "box_layout")
            .unwrap();
        let longs: Vec<&str> = box_arg.get_long_and_visible_aliases().unwrap();
        assert!(
            longs.contains(&"box"),
            "--box flag missing, got: {:?}",
            longs
        );
    }
}
