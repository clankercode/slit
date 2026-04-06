package main

import (
	"bufio"
	"fmt"
	"io"
	"os"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/spf13/cobra"
	cobradoc "github.com/spf13/cobra/doc"
	"golang.org/x/term"
)

var version = "0.1.0"

var (
	flagLines          int
	flagMaxLines       int
	flagOutput         string
	flagAppend         bool
	flagTeeFormat      string
	flagLineNumbers    bool
	flagColor          string
	flagWrap           bool
	flagTimestamp      bool
	flagTruncationChar string
	flagLayout         string
	flagBox            bool
	flagRounded        bool
	flagCompact        bool
	flagMinimal        bool
	flagNone           bool
	flagQuote          bool
	flagQuoteBg        string
	flagSpinner        string
	flagDebug          bool
	flagLogFile        string
	flagGenerateMan    bool
)

var rootCmd = &cobra.Command{
	Use:     "slit",
	Short:   "A streaming terminal viewer",
	Long:    "slit reads stdin into a fixed-height pane, trims lines to terminal width, and re-renders on each new line.",
	Version: version,
	Example: "  tail -f /var/log/syslog | slit\n  dmesg | slit -n 20 --line-numbers\n  build-command 2>&1 | slit --compact -o build.log",
	Run: func(cmd *cobra.Command, args []string) {
		if flagGenerateMan {
			header := &cobradoc.GenManHeader{
				Title:   "SLIT",
				Section: "1",
			}
			if err := cobradoc.GenMan(cmd, header, os.Stdout); err != nil {
				fmt.Fprintf(os.Stderr, "error generating man page: %v\n", err)
				os.Exit(1)
			}
			return
		}

		cfg, err := resolveConfig(cmd)
		if err != nil {
			fmt.Fprintf(os.Stderr, "error: %v\n", err)
			os.Exit(1)
		}

		forceRender := os.Getenv("SLIT_FORCE_RENDER") == "1"
		if !forceRender && !term.IsTerminal(int(os.Stderr.Fd())) {
			io.Copy(os.Stdout, os.Stdin)
			return
		}

		m := newModel(cfg)

		p := tea.NewProgram(m, tea.WithOutput(os.Stderr))

		go func() {
			scanner := bufio.NewScanner(os.Stdin)
			for scanner.Scan() {
				line := scanner.Text()
				p.Send(lineMsg(line))
			}
			p.Send(eofMsg{})
		}()

		if _, err := p.Run(); err != nil {
			fmt.Fprintf(os.Stderr, "error: %v\n", err)
			os.Exit(1)
		}

		os.Stderr.WriteString("\x1b]0;\x07")
	},
}

func init() {
	rootCmd.Flags().IntVarP(&flagLines, "lines", "n", 0, "Number of lines to display (0 = auto/terminal height - 1)")
	rootCmd.Flags().IntVar(&flagMaxLines, "max-lines", 50000, "Maximum number of lines to buffer")
	rootCmd.Flags().StringVarP(&flagOutput, "output", "o", "", "Write output to file")
	rootCmd.Flags().BoolVarP(&flagAppend, "append", "a", false, "Append to output file instead of overwriting")
	rootCmd.Flags().StringVar(&flagTeeFormat, "tee-format", "raw", "Tee output format (raw, display)")

	rootCmd.Flags().BoolVarP(&flagLineNumbers, "line-numbers", "l", false, "Show line numbers")
	rootCmd.Flags().StringVar(&flagColor, "color", "auto", "Color output (auto, always, never)")
	rootCmd.Flags().BoolVarP(&flagWrap, "wrap", "w", false, "Wrap long lines")
	rootCmd.Flags().BoolVarP(&flagTimestamp, "timestamp", "t", false, "Show timestamps")
	rootCmd.Flags().StringVar(&flagTruncationChar, "truncation-char", "…", "Character used for truncation indicator")

	rootCmd.Flags().StringVar(&flagLayout, "layout", "minimal", "Layout style (box, rounded, compact, minimal, none, quote)")
	rootCmd.Flags().BoolVar(&flagBox, "box", false, "Use box layout (shortcut for --layout=box)")
	rootCmd.Flags().BoolVar(&flagRounded, "rounded", false, "Use rounded layout (shortcut for --layout=rounded)")
	rootCmd.Flags().BoolVar(&flagCompact, "compact", false, "Use compact layout (shortcut for --layout=compact)")
	rootCmd.Flags().BoolVar(&flagMinimal, "minimal", false, "Use minimal layout (shortcut for --layout=minimal)")
	rootCmd.Flags().BoolVar(&flagNone, "none", false, "Use no layout (shortcut for --layout=none)")
	rootCmd.Flags().BoolVar(&flagQuote, "quote", false, "Use quote layout (shortcut for --layout=quote)")
	rootCmd.Flags().StringVar(&flagQuoteBg, "quote-bg", "off", "Quote background style")
	rootCmd.Flags().StringVar(&flagSpinner, "spinner", "braille", "Spinner style (braille, dots, arrows, off)")

	rootCmd.Flags().BoolVarP(&flagDebug, "debug", "d", false, "Enable debug logging")
	rootCmd.Flags().StringVar(&flagLogFile, "log-file", "", "Write debug logs to file")

	rootCmd.Flags().BoolVar(&flagGenerateMan, "generate-man", false, "Generate man page to stdout")
	rootCmd.Flags().MarkHidden("generate-man")

	rootCmd.AddCommand(&cobra.Command{
		Use:   "completion [bash|zsh|fish]",
		Short: "Generate shell completion script",
		Args:  cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			switch args[0] {
			case "bash":
				rootCmd.GenBashCompletion(os.Stdout)
			case "zsh":
				rootCmd.GenZshCompletion(os.Stdout)
			case "fish":
				rootCmd.GenFishCompletion(os.Stdout, true)
			default:
				fmt.Fprintf(os.Stderr, "unsupported shell: %s (use bash, zsh, or fish)\n", args[0])
				os.Exit(1)
			}
		},
	})
}

func resolveConfig(cmd *cobra.Command) (*Config, error) {
	layout := ""
	if cmd.Flags().Changed("box") && flagBox {
		layout = "box"
	}
	if cmd.Flags().Changed("rounded") && flagRounded {
		layout = "rounded"
	}
	if cmd.Flags().Changed("compact") && flagCompact {
		layout = "compact"
	}
	if cmd.Flags().Changed("minimal") && flagMinimal {
		layout = "minimal"
	}
	if cmd.Flags().Changed("none") && flagNone {
		layout = "none"
	}
	if cmd.Flags().Changed("quote") && flagQuote {
		layout = "quote"
	}
	if cmd.Flags().Changed("layout") {
		layout = flagLayout
	}

	validTeeFormat := map[string]bool{"raw": true, "display": true}
	if !validTeeFormat[flagTeeFormat] {
		return nil, fmt.Errorf("invalid --tee-format %q: must be raw or display", flagTeeFormat)
	}

	validColor := map[string]bool{"auto": true, "always": true, "never": true}
	if !validColor[flagColor] {
		return nil, fmt.Errorf("invalid --color %q: must be auto, always, or never", flagColor)
	}

	validSpinner := map[string]bool{"braille": true, "dots": true, "arrows": true, "off": true}
	if !validSpinner[flagSpinner] {
		return nil, fmt.Errorf("invalid --spinner %q: must be braille, dots, arrows, or off", flagSpinner)
	}

	validLayout := map[string]bool{"box": true, "rounded": true, "compact": true, "minimal": true, "none": true, "quote": true}
	if layout != "" && !validLayout[layout] {
		return nil, fmt.Errorf("invalid --layout %q: must be box, rounded, compact, minimal, none, or quote", layout)
	}

	cfg := DefaultConfig()
	cfg = ApplyFileConfig(cfg)

	if cmd.Flags().Changed("lines") {
		cfg.Lines = flagLines
	}
	if cmd.Flags().Changed("max-lines") {
		cfg.MaxLines = flagMaxLines
	}
	if cmd.Flags().Changed("output") {
		cfg.Output = flagOutput
	}
	if cmd.Flags().Changed("append") {
		cfg.Append = flagAppend
	}
	if cmd.Flags().Changed("tee-format") {
		cfg.TeeFormat = flagTeeFormat
	}
	if cmd.Flags().Changed("line-numbers") {
		cfg.LineNumbers = flagLineNumbers
	}
	if cmd.Flags().Changed("color") {
		cfg.Color = flagColor
	}
	if cmd.Flags().Changed("wrap") {
		cfg.Wrap = flagWrap
	}
	if cmd.Flags().Changed("timestamp") {
		cfg.Timestamp = flagTimestamp
	}
	if cmd.Flags().Changed("truncation-char") {
		cfg.TruncationChar = flagTruncationChar
	}
	if layout != "" {
		cfg.Layout = layout
	}
	if cmd.Flags().Changed("quote-bg") {
		cfg.QuoteBg = flagQuoteBg
	}
	if cmd.Flags().Changed("spinner") {
		cfg.Spinner = flagSpinner
	}
	if cmd.Flags().Changed("debug") {
		cfg.Debug = flagDebug
	}
	if cmd.Flags().Changed("log-file") {
		cfg.LogFile = flagLogFile
	}

	return &cfg, nil
}

func main() {
	rootCmd.Execute()
}
