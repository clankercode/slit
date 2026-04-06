use clap::{Parser, Subcommand, ValueEnum};
use clap_complete::{generate, shells::*};
use std::io;
use std::path::PathBuf;

#[derive(Parser)]
#[command(name = "slit")]
#[command(about = "A streaming terminal viewer")]
#[command(long_about = "slit reads stdin into a fixed-height pane, trims lines to terminal width, and re-renders on each new line.")]
#[command(version)]
struct Cli {
    #[arg(short = 'n', long, help = "Number of lines to display (0 = auto/terminal height - 1)")]
    lines: Option<usize>,

    #[arg(long, help = "Maximum number of lines to buffer", default_value = "50000")]
    max_lines: usize,

    #[arg(short = 'o', long, help = "Write output to file")]
    output: Option<PathBuf>,

    #[arg(short = 'a', long, help = "Append to output file instead of overwriting")]
    append: bool,

    #[arg(long, value_enum, help = "Tee output format (raw, display)", default_value = "raw")]
    tee_format: TeeFormat,

    #[arg(short = 'l', long, help = "Show line numbers")]
    line_numbers: bool,

    #[arg(long, value_enum, help = "Color output (auto, always, never)", default_value = "auto")]
    color: ColorMode,

    #[arg(short = 'w', long, help = "Wrap long lines")]
    wrap: bool,

    #[arg(short = 't', long, help = "Show timestamps")]
    timestamp: bool,

    #[arg(long, help = "Character used for truncation indicator", default_value = "…")]
    truncation_char: String,

    #[arg(long, value_enum, help = "Layout style", default_value = "minimal")]
    layout: LayoutType,

    #[arg(long, help = "Use box layout (shortcut for --layout=box)")]
    box_layout: bool,

    #[arg(long, help = "Use rounded layout (shortcut for --layout=rounded)")]
    rounded: bool,

    #[arg(long, help = "Use compact layout (shortcut for --layout=compact)")]
    compact: bool,

    #[arg(long, help = "Use minimal layout (shortcut for --layout=minimal)")]
    minimal: bool,

    #[arg(long, help = "Use no layout (shortcut for --layout=none)")]
    none: bool,

    #[arg(long, help = "Use quote layout (shortcut for --layout=quote)")]
    quote: bool,

    #[arg(long, value_enum, help = "Quote background style", default_value = "off")]
    quote_bg: QuoteBg,

    #[arg(long, value_enum, help = "Spinner style (braille, dots, arrows, off)", default_value = "braille")]
    spinner: SpinnerStyle,

    #[arg(short = 'd', long, help = "Enable debug logging")]
    debug: bool,

    #[arg(long, help = "Write debug logs to file")]
    log_file: Option<PathBuf>,

    #[arg(long, help = "Generate man page to stdout", hide = true)]
    generate_man: bool,

    #[command(subcommand)]
    command: Option<Commands>,
}

#[derive(Subcommand)]
enum Commands {
    #[command(about = "Generate shell completion script")]
    Completion {
        #[arg(value_enum)]
        shell: Shell,
    },
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum Shell {
    Bash,
    Zsh,
    Fish,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum TeeFormat {
    Raw,
    Display,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum ColorMode {
    Auto,
    Always,
    Never,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum LayoutType {
    Box,
    Rounded,
    Compact,
    Minimal,
    None,
    Quote,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum QuoteBg {
    Off,
    On,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
enum SpinnerStyle {
    Braille,
    Dots,
    Arrows,
    Off,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    if cli.generate_man {
        generate_man_page().await?;
        return Ok(());
    }

    match cli.command {
        Some(Commands::Completion { shell }) => {
            generate_completion(shell)?;
            return Ok(());
        }
        None => {}
    }

    println!("slit initialized with layout: {:?}", cli.layout);
    println!("lines: {:?}, max_lines: {}", cli.lines, cli.max_lines);
    
    Ok(())
}

async fn generate_man_page() -> anyhow::Result<()> {
    let cmd = <Cli as clap::CommandFactory>::command();
    let man = clap_mangen::Man::new(cmd);
    let mut buffer: Vec<u8> = Default::default();
    man.render(&mut buffer)?;
    println!("{}", String::from_utf8(buffer)?);
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
