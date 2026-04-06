#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "slit.h"

#define DEFAULT_TRUNCATION_CHAR "..."

static void print_usage(void) {
    printf("Usage: slit [OPTIONS]\n");
    printf("\n");
    printf("A streaming terminal viewer that displays the last N lines of input.\n");
    printf("\n");
    printf("Core Options:\n");
    printf("  -n, --lines=N            Number of lines to display (0 = auto)\n");
    printf("      --max-lines=N        Maximum lines to buffer (default: %d)\n", DEFAULT_MAX_LINES);
    printf("  -o, --output=FILE        Write output to file (tee mode)\n");
    printf("  -a, --append             Append to output file instead of overwrite\n");
    printf("      --tee-format=FORMAT  Tee format: raw or display (default: raw)\n");
    printf("\n");
    printf("Display Options:\n");
    printf("  -l, --line-numbers       Show line numbers\n");
    printf("      --color=MODE         Color mode: auto, always, never (default: auto)\n");
    printf("  -w, --wrap               Wrap long lines instead of truncating\n");
    printf("  -t, --timestamp          Show timestamps\n");
    printf("      --truncation-char=CHAR  Character for truncation (default: %s)\n", DEFAULT_TRUNCATION_CHAR);
    printf("\n");
    printf("Layout Options:\n");
    printf("      --layout=STYLE       Layout: box, rounded, compact, minimal, none, quote\n");
    printf("      --box                Use box layout\n");
    printf("      --rounded            Use rounded layout\n");
    printf("      --compact            Use compact layout\n");
    printf("      --minimal            Use minimal layout (default)\n");
    printf("      --none               Use no layout\n");
    printf("      --quote              Use quote layout\n");
    printf("      --quote-bg=COLOR     Quote background color (hex) or 'off'\n");
    printf("\n");
    printf("Progress Options:\n");
    printf("      --spinner=STYLE      Spinner: braille, dots, arrows, off (default: braille)\n");
    printf("\n");
    printf("Debug Options:\n");
    printf("  -d, --debug              Enable debug logging\n");
    printf("      --log-file=FILE      Debug log file path\n");
    printf("\n");
    printf("Other Options:\n");
    printf("      --help               Show this help message\n");
    printf("      --version            Show version information\n");
    printf("\n");
    printf("Commands:\n");
    printf("  completion SHELL         Output shell completion script (bash, zsh, fish)\n");
    printf("\n");
    printf("Environment:\n");
    printf("  SLIT_FORCE_RENDER=1      Force rendering even when stderr is not a TTY\n");
}

static void print_version(void) {
    printf("slit %s\n", VERSION);
}

static struct slit_config default_config(void) {
    struct slit_config cfg;
    cfg.lines = DEFAULT_LINES;
    cfg.max_lines = DEFAULT_MAX_LINES;
    cfg.output = NULL;
    cfg.append = 0;
    cfg.tee_format = TEE_RAW;
    cfg.line_numbers = 0;
    cfg.color = COLOR_AUTO;
    cfg.wrap = 0;
    cfg.timestamp = 0;
    cfg.truncation_char = strdup(DEFAULT_TRUNCATION_CHAR);
    cfg.layout = DEFAULT_LAYOUT;
    cfg.quote_bg = strdup("off");
    cfg.spinner = DEFAULT_SPINNER;
    cfg.debug = 0;
    cfg.log_file = NULL;
    return cfg;
}

static int parse_layout(const char *str) {
    if (strcmp(str, "box") == 0) return LAYOUT_BOX;
    if (strcmp(str, "rounded") == 0) return LAYOUT_ROUNDED;
    if (strcmp(str, "compact") == 0) return LAYOUT_COMPACT;
    if (strcmp(str, "minimal") == 0) return LAYOUT_MINIMAL;
    if (strcmp(str, "none") == 0) return LAYOUT_NONE;
    if (strcmp(str, "quote") == 0) return LAYOUT_QUOTE;
    return -1;
}

static int parse_spinner(const char *str) {
    if (strcmp(str, "braille") == 0) return SPINNER_BRAILLE;
    if (strcmp(str, "dots") == 0) return SPINNER_DOTS;
    if (strcmp(str, "arrows") == 0) return SPINNER_ARROWS;
    if (strcmp(str, "off") == 0) return SPINNER_OFF;
    return -1;
}

static int parse_color(const char *str) {
    if (strcmp(str, "auto") == 0) return COLOR_AUTO;
    if (strcmp(str, "always") == 0) return COLOR_ALWAYS;
    if (strcmp(str, "never") == 0) return COLOR_NEVER;
    return -1;
}

static int parse_tee_format(const char *str) {
    if (strcmp(str, "raw") == 0) return TEE_RAW;
    if (strcmp(str, "display") == 0) return TEE_DISPLAY;
    return -1;
}

static void free_config(struct slit_config *cfg) {
    if (cfg->truncation_char) free(cfg->truncation_char);
    if (cfg->quote_bg) free(cfg->quote_bg);
    if (cfg->output) free(cfg->output);
    if (cfg->log_file) free(cfg->log_file);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "completion") == 0) {
        printf("# Completion scripts not yet implemented\n");
        return 0;
    }

    struct slit_config cfg = default_config();
    int layout_set = 0;
    int layout_value = LAYOUT_MINIMAL;

    static struct option long_options[] = {
        {"lines", required_argument, 0, 'n'},
        {"max-lines", required_argument, 0, 0},
        {"output", required_argument, 0, 'o'},
        {"append", no_argument, 0, 'a'},
        {"tee-format", required_argument, 0, 0},
        {"line-numbers", no_argument, 0, 'l'},
        {"color", required_argument, 0, 0},
        {"wrap", no_argument, 0, 'w'},
        {"timestamp", no_argument, 0, 't'},
        {"truncation-char", required_argument, 0, 0},
        {"layout", required_argument, 0, 0},
        {"box", no_argument, 0, 0},
        {"rounded", no_argument, 0, 0},
        {"compact", no_argument, 0, 0},
        {"minimal", no_argument, 0, 0},
        {"none", no_argument, 0, 0},
        {"quote", no_argument, 0, 0},
        {"quote-bg", required_argument, 0, 0},
        {"spinner", required_argument, 0, 0},
        {"debug", no_argument, 0, 'd'},
        {"log-file", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "n:o:alwtvdh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'n':
                cfg.lines = atoi(optarg);
                break;
            case 'o':
                if (cfg.output) free(cfg.output);
                cfg.output = strdup(optarg);
                break;
            case 'a':
                cfg.append = 1;
                break;
            case 'l':
                cfg.line_numbers = 1;
                break;
            case 'w':
                cfg.wrap = 1;
                break;
            case 't':
                cfg.timestamp = 1;
                break;
            case 'd':
                cfg.debug = 1;
                break;
            case 'v':
                print_version();
                free_config(&cfg);
                return 0;
            case 'h':
                print_usage();
                free_config(&cfg);
                return 0;
            case 0:
                if (strcmp(long_options[option_index].name, "max-lines") == 0) {
                    cfg.max_lines = atoi(optarg);
                } else if (strcmp(long_options[option_index].name, "tee-format") == 0) {
                    int fmt = parse_tee_format(optarg);
                    if (fmt < 0) {
                        fprintf(stderr, "Error: invalid --tee-format: %s\n", optarg);
                        free_config(&cfg);
                        return 1;
                    }
                    cfg.tee_format = fmt;
                } else if (strcmp(long_options[option_index].name, "color") == 0) {
                    int col = parse_color(optarg);
                    if (col < 0) {
                        fprintf(stderr, "Error: invalid --color: %s\n", optarg);
                        free_config(&cfg);
                        return 1;
                    }
                    cfg.color = col;
                } else if (strcmp(long_options[option_index].name, "truncation-char") == 0) {
                    if (cfg.truncation_char) free(cfg.truncation_char);
                    cfg.truncation_char = strdup(optarg);
                } else if (strcmp(long_options[option_index].name, "layout") == 0) {
                    int lay = parse_layout(optarg);
                    if (lay < 0) {
                        fprintf(stderr, "Error: invalid --layout: %s\n", optarg);
                        free_config(&cfg);
                        return 1;
                    }
                    layout_set = 1;
                    layout_value = lay;
                } else if (strcmp(long_options[option_index].name, "box") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_BOX;
                } else if (strcmp(long_options[option_index].name, "rounded") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_ROUNDED;
                } else if (strcmp(long_options[option_index].name, "compact") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_COMPACT;
                } else if (strcmp(long_options[option_index].name, "minimal") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_MINIMAL;
                } else if (strcmp(long_options[option_index].name, "none") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_NONE;
                } else if (strcmp(long_options[option_index].name, "quote") == 0) {
                    layout_set = 1;
                    layout_value = LAYOUT_QUOTE;
                } else if (strcmp(long_options[option_index].name, "quote-bg") == 0) {
                    if (cfg.quote_bg) free(cfg.quote_bg);
                    cfg.quote_bg = strdup(optarg);
                } else if (strcmp(long_options[option_index].name, "spinner") == 0) {
                    int spin = parse_spinner(optarg);
                    if (spin < 0) {
                        fprintf(stderr, "Error: invalid --spinner: %s\n", optarg);
                        free_config(&cfg);
                        return 1;
                    }
                    cfg.spinner = spin;
                } else if (strcmp(long_options[option_index].name, "log-file") == 0) {
                    if (cfg.log_file) free(cfg.log_file);
                    cfg.log_file = strdup(optarg);
                }
                break;
            case '?':
                free_config(&cfg);
                return 1;
        }
    }

    if (layout_set) {
        cfg.layout = layout_value;
    }

    printf("Parsed config: lines=%d, max_lines=%d, layout=%d, spinner=%d\n",
           cfg.lines, cfg.max_lines, cfg.layout, cfg.spinner);

    free_config(&cfg);
    return 0;
}
