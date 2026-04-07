use clap::{Parser, Subcommand, ValueEnum};
use clap_complete::{generate, shells::*};
use std::io;
use std::path::PathBuf;

pub mod app;
pub mod buffer;
pub mod config;
pub mod debug;
pub mod layout;
pub mod render;
pub mod spinner;
pub mod tee;

use config::Config;

#[derive(Parser)]
#[command(name = "slit")]
#[command(about = "A streaming terminal viewer")]
#[command(long_about = "slit reads stdin into a fixed-height pane, trims lines to terminal width, and re-renders on each new line.")]
#[command(version)]
pub struct Cli {
    #[arg(short = 'n', long, help = "Number of lines to display (interactive: 0 = auto/2/3 height; passthrough: 0 = pipe all, default N=10 shows first+last N lines)")]
    pub lines: Option<usize>,

    #[arg(long, help = "Maximum number of lines to buffer")]
    pub max_lines: Option<usize>,

    #[arg(short = 'o', long, help = "Write output to file", value_hint = clap::ValueHint::FilePath)]
    pub output: Option<PathBuf>,

    #[arg(short = 'a', long, help = "Append to output file instead of overwriting")]
    pub append: bool,

    #[arg(long, value_enum, help = "Tee output format (raw, display)")]
    pub tee_format: Option<TeeFormat>,

    #[arg(short = 'l', long, help = "Show line numbers")]
    pub line_numbers: bool,

    #[arg(long, value_enum, help = "Color output (auto, always, never)")]
    pub color: Option<ColorMode>,

    #[arg(short = 'w', long, help = "Wrap long lines")]
    pub wrap: bool,

    #[arg(short = 't', long, help = "Show timestamps")]
    pub timestamp: bool,

    #[arg(long, help = "Character used for truncation indicator")]
    pub truncation_char: Option<String>,

    #[arg(long, value_enum, help = "Layout style")]
    pub layout: Option<LayoutType>,

    #[arg(long = "box", help = "Use box layout (shortcut for --layout=box)")]
    pub box_layout: bool,

    #[arg(long, help = "Use rounded layout (shortcut for --layout=rounded)")]
    pub rounded: bool,

    #[arg(long, help = "Use compact layout (shortcut for --layout=compact)")]
    pub compact: bool,

    #[arg(long, help = "Use minimal layout (shortcut for --layout=minimal)")]
    pub minimal: bool,

    #[arg(long, help = "Use no layout (shortcut for --layout=none)")]
    pub none: bool,

    #[arg(long, help = "Use quote layout (shortcut for --layout=quote)")]
    pub quote: bool,

    #[arg(long, value_enum, help = "Quote background style")]
    pub quote_bg: Option<QuoteBg>,

    #[arg(long, value_enum, help = "Spinner style (braille, dots, arrows, off)")]
    pub spinner: Option<SpinnerStyle>,

    #[arg(short = 'd', long, help = "Enable debug logging")]
    pub debug: bool,

    #[arg(long, help = "Write debug logs to file", value_hint = clap::ValueHint::FilePath)]
    pub log_file: Option<PathBuf>,

    #[arg(long, help = "Generate man page to stdout", hide = true)]
    pub generate_man: bool,

    #[command(subcommand)]
    pub command: Option<Commands>,
}

#[derive(Subcommand)]
pub enum Commands {
    #[command(about = "Generate shell completion script")]
    Completion {
        #[arg(value_enum)]
        shell: Shell,
    },
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum Shell {
    Bash,
    Zsh,
    Fish,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum TeeFormat {
    Raw,
    Display,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum ColorMode {
    Auto,
    Always,
    Never,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum LayoutType {
    Box,
    Rounded,
    Compact,
    Minimal,
    None,
    Quote,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum QuoteBg {
    Off,
    On,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum SpinnerStyle {
    Braille,
    Dots,
    Arrows,
    Off,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    if cli.generate_man {
        generate_man_page()?;
        return Ok(());
    }

    match cli.command {
        Some(Commands::Completion { shell }) => {
            generate_completion(shell)?;
            return Ok(());
        }
        None => {}
    }

    let config = Config::resolve(&cli)?;

    if !app::is_stderr_tty() {
        app::passthrough(&config).await?;
        return Ok(());
    }

    let mut app = app::App::new(config);
    app.run().await?;

    Ok(())
}

fn generate_man_page() -> anyhow::Result<()> {
    let cmd = <Cli as clap::CommandFactory>::command();
    let man = clap_mangen::Man::new(cmd);
    let mut buffer: Vec<u8> = Default::default();
    man.render(&mut buffer)?;
    print!("{}", String::from_utf8(buffer)?);
    Ok(())
}

fn generate_completion(shell: Shell) -> anyhow::Result<()> {
    let mut cmd = <Cli as clap::CommandFactory>::command();
    match shell {
        Shell::Bash => generate(Bash, &mut cmd, "slit", &mut io::stdout()),
        Shell::Zsh => generate(Zsh, &mut cmd, "slit", &mut io::stdout()),
        Shell::Fish => generate(Fish, &mut cmd, "slit", &mut io::stdout()),
    }
    Ok(())
}
