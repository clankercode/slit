#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "slit.h"
#include "terminal.h"
#include "buffer.h"
#include "render.h"
#include "layout.h"
#include "spinner.h"
#include "status.h"
#include "tee.h"
#include "debug.h"
#include "completion.h"

#define DEFAULT_TRUNCATION_CHAR "\xe2\x80\xa6"

static int g_tty_fd = -1;

static void atexit_restore(void) {
    if (g_tty_fd >= 0) {
        term_raw_restore();
        term_restore_title();
    }
}

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

static void print_manpage(void) {
    printf(".TH SLIT 1 \"slit %s\" \"slit\" \"User Commands\"\n", VERSION);
    puts(".SH NAME");
    puts("slit \\\\- streaming terminal viewer with a fixed-height live pane");
    puts(".SH SYNOPSIS");
    puts(".B slit\n[OPTIONS]\n.br\n.B slit\ncompletion\nI SHELL");
    puts(".SH DESCRIPTION");
    puts("slit reads stdin into a fixed-height pane, trims lines to terminal width, and re-renders on each new line.");
    puts("It renders to stderr so stdout can still be piped onward when rendering is disabled.");
    puts(".SH OPTIONS");
    puts(".TP\n.BR \\-n ,\\~\\-\\-lines = N\nNumber of lines to display (0 = auto).");
    puts(".TP\n.BR \\-\\-max\\-lines = N\nMaximum number of lines to buffer.");
    puts(".TP\n.BR \\-o ,\\~\\-\\-output = FILE\nWrite output to file (tee mode).");
    puts(".TP\n.BR \\-a ,\\~\\-\\-append\nAppend to output file instead of overwriting.");
    puts(".TP\n.BR \\-\\-tee\\-format = raw|display\nTee output format.");
    puts(".TP\n.BR \\-l ,\\~\\-\\-line\\-numbers\nShow line numbers.");
    puts(".TP\n.BR \\-\\-color = auto|always|never\nColor mode.");
    puts(".TP\n.BR \\-w ,\\~\\-\\-wrap\nWrap long lines instead of truncating.");
    puts(".TP\n.BR \\-t ,\\~\\-\\-timestamp\nShow timestamps.");
    puts(".TP\n.BR \\-\\-truncation\\-char = CHAR\nCharacter used for truncation indicator.");
    puts(".TP\n.BR \\-\\-layout = box|rounded|compact|minimal|none|quote\nLayout style.");
    puts(".TP\n.BR \\-\\-box \\fR,\\fP \\-\\-rounded \\fR,\\fP \\-\\-compact \\fR,\\fP \\-\\-minimal \\fR,\\fP \\-\\-none \\fR,\\fP \\-\\-quote\nLayout shortcuts.");
    puts(".TP\n.BR \\-\\-quote\\-bg = off|on\nQuote background style.");
    puts(".TP\n.BR \\-\\-spinner = braille|dots|arrows|off\nSpinner style.");
    puts(".TP\n.BR \\-d ,\\~\\-\\-debug\nEnable debug logging.");
    puts(".TP\n.BR \\-\\-log\\-file = FILE\nWrite debug logs to file.");
    puts(".TP\n.BR \\-\\-help\nShow help text.");
    puts(".TP\n.BR \\-\\-version\nShow version information.");
    puts(".SH COMMANDS");
    puts(".TP\n.BI \"completion \" SHELL\nOutput shell completion script for bash, zsh, or fish.");
    puts(".SH ENVIRONMENT");
    puts(".TP\n.B SLIT_FORCE_RENDER=1\nForce rendering even when stderr is not a TTY.");
    puts(".SH EXAMPLES");
    puts(".EX\nmake build 2>&1 | slit\ntail -f /var/log/syslog | slit -n 10 -l -t\npytest tests/ | slit --box -o results.log\n.EE");
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
    if (!cfg.truncation_char) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    cfg.layout = DEFAULT_LAYOUT;
    cfg.quote_bg = strdup("off");
    if (!cfg.quote_bg) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
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

static int digits_in(size_t n) {
    if (n < 10) return 1;
    int d = 0;
    while (n > 0) { n /= 10; d++; }
    return d;
}

static int gutter_width(struct slit_config *cfg, size_t total_lines) {
    int w = 0;
    if (cfg->line_numbers) {
        w += digits_in(total_lines > 0 ? total_lines : 1) + 1;
    }
    if (cfg->timestamp) {
        w += 9;
    }
    return w;
}

static int line_num_pad_width(struct slit_config *cfg, size_t total_lines) {
    if (!cfg->line_numbers) return 0;
    return digits_in(total_lines > 0 ? total_lines : 1);
}

static void format_line_prefix(struct slit_config *cfg, struct line_entry *entry,
                               char *buf, size_t buf_size, size_t total_lines,
                               int is_wrap_continuation) {
    buf[0] = '\0';
    size_t pos = 0;

    if (cfg->timestamp) {
        struct tm tm_buf;
        localtime_r(&entry->arrival, &tm_buf);
        pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[2m");
        pos += strftime(buf + pos, buf_size - pos, "%H:%M:%S", &tm_buf);
        pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[0m ");
    }

    if (cfg->line_numbers) {
        int pad_w = line_num_pad_width(cfg, total_lines);
        if (is_wrap_continuation) {
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[2m");
            for (int i = 0; i < pad_w && pos < buf_size - 8; i++) buf[pos++] = ' ';
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[0m ");
        } else {
            pos += (size_t)snprintf(buf + pos, buf_size - pos, "\x1b[2m%*zu\x1b[0m ", pad_w, entry->line_num);
        }
    }

    buf[pos] = '\0';
}

static void passthrough_mode(struct slit_config *cfg) {
    struct tee_writer *tw = NULL;
    if (cfg->output) {
        tw = tee_open(cfg->output, cfg->append, cfg->tee_format);
    }
    debug_init(cfg->log_file, cfg->debug);
    debug_log("passthrough mode starting");
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
        fputs(line, stdout);
        if (tw) {
            size_t slen = strlen(line);
            while (slen > 0 && (line[slen-1] == '\n' || line[slen-1] == '\r'))
                line[--slen] = '\0';
            tee_write_line(tw, line);
        }
    }
    free(line);
    if (tw) tee_close(tw);
    debug_log("passthrough mode done");
    debug_close();
}

static void render_frame(struct slit_config *cfg, struct ring_buffer *buf,
                         int term_width, int term_rows, int content_lines,
                         int spinner_frame, int eof, long file_size,
                         int prev_height, int *out_rendered_height) {
    int data_lines = layout_data_lines(cfg->layout, content_lines);
    int content_width = layout_content_width(cfg->layout, term_width);
    int gw = gutter_width(cfg, buffer_total_lines(buf));
    int data_width = content_width - gw;
    if (data_width < 1) data_width = 1;

    int should_strip = (cfg->color == COLOR_NEVER) ||
                       (cfg->color == COLOR_AUTO && !is_stderr_tty());

    size_t count = buffer_count(buf);
    size_t start = 0;
    char stripped_buf[4096];

    if (cfg->wrap && data_lines > 0) {
        int slots_remaining = data_lines;
        size_t i = count;
        while (i > 0 && slots_remaining > 0) {
            i--;
            struct line_entry *e = buffer_get(buf, i);
            if (!e || !e->text) continue;
            const char *line_text = e->text;
            if (should_strip) {
                strip_ansi(line_text, stripped_buf, sizeof(stripped_buf));
                line_text = stripped_buf;
            }
            int wrap_count = (int)((visible_strlen(line_text) + data_width - 1) / data_width);
            if (wrap_count < 1) wrap_count = 1;
            slots_remaining -= wrap_count;
        }
        start = i;
        if (slots_remaining < 0) start++;
    } else if (data_lines > 0 && (int)count > data_lines) {
        start = count - data_lines;
    }

    int max_fmt = data_lines;
    if (max_fmt < 1) max_fmt = 1;

    char **formatted = malloc(max_fmt * sizeof(char *));
    if (!formatted) return;

    int fmt_count = 0;
    char trim_buf[4096];
    char prefix_buf[256];

    for (size_t i = start; i < count && fmt_count < max_fmt; ) {
        struct line_entry *entry = buffer_get(buf, i);
        if (!entry || !entry->text) { i++; continue; }

        const char *line = entry->text;

        if (should_strip) {
            strip_ansi(line, stripped_buf, sizeof(stripped_buf));
            line = stripped_buf;
        }

        if (cfg->wrap) {
            char **wrapped = NULL;
            int wrap_count = 0;
            wrap_line(line, data_width, &wrapped, &wrap_count);
            for (int w = 0; w < wrap_count && fmt_count < max_fmt; w++) {
                format_line_prefix(cfg, entry, prefix_buf, sizeof(prefix_buf),
                                   buffer_total_lines(buf), w > 0);
                size_t prefix_len = strlen(prefix_buf);
                size_t wrap_len = strlen(wrapped[w]);
                char *full = malloc(prefix_len + wrap_len + 1);
                if (full) {
                    memcpy(full, prefix_buf, prefix_len);
                    memcpy(full + prefix_len, wrapped[w], wrap_len + 1);
                    formatted[fmt_count++] = full;
                }
            }
            wrap_lines_free(wrapped, wrap_count);
        } else {
            trim_line(line, trim_buf, sizeof(trim_buf), data_width, cfg->truncation_char);
            format_line_prefix(cfg, entry, prefix_buf, sizeof(prefix_buf),
                               buffer_total_lines(buf), 0);
            size_t prefix_len = strlen(prefix_buf);
            size_t trim_len = strlen(trim_buf);
            char *full = malloc(prefix_len + trim_len + 1);
            if (full) {
                memcpy(full, prefix_buf, prefix_len);
                memcpy(full + prefix_len, trim_buf, trim_len + 1);
                formatted[fmt_count++] = full;
            }
        }
        i++;
    }

    int status_width = content_width;
    if (cfg->layout == LAYOUT_BOX || cfg->layout == LAYOUT_ROUNDED) {
        status_width = term_width - 4;
    }

    char *status = NULL;
    if (cfg->layout != LAYOUT_NONE) {
        status = format_status_line(cfg->spinner, spinner_frame, eof,
                                    buffer_total_lines(buf), buffer_total_bytes(buf),
                                    file_size, status_width);
    }

    if (prev_height > 0) {
        fprintf(stderr, "\x1b[%dA", prev_height);
    }
    fprintf(stderr, "\x1b[0J");

    layout_render(cfg->layout, term_width, "slit",
                  (const char **)formatted, fmt_count,
                  status, cfg->quote_bg);

    for (int i = 0; i < fmt_count; i++) free(formatted[i]);
    free(formatted);
    free(status);

    struct layout_chrome ch = layout_get_chrome(cfg->layout);
    if (cfg->layout == LAYOUT_QUOTE) {
        *out_rendered_height = fmt_count + 2;
    } else {
        *out_rendered_height = fmt_count + ch.top_lines + ch.bottom_lines;
    }

    (void)term_rows;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "completion") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: slit completion <shell>\nSupported: bash, zsh, fish\n");
            return 1;
        }
        return completion_print(argv[2]);
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
        {"generate-man", no_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "n:o:alwtvdh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'n':
                {
                    char *endp;
                    long val = strtol(optarg, &endp, 10);
                    if (*endp != '\0' || val <= 0) {
                        fprintf(stderr, "Error: --lines must be a positive integer\n");
                        free_config(&cfg);
                        return 1;
                    }
                    cfg.lines = (int)val;
                }
                if (cfg.lines < 0) {
                    fprintf(stderr, "Error: --lines must be >= 0\n");
                    free_config(&cfg);
                    return 1;
                }
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
                    char *endp;
                    long val = strtol(optarg, &endp, 10);
                    if (*endp != '\0' || val <= 0) {
                        fprintf(stderr, "Error: --max-lines must be a positive integer\n");
                        free_config(&cfg);
                        return 1;
                    }
                    cfg.max_lines = (int)val;
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
                } else if (strcmp(long_options[option_index].name, "generate-man") == 0) {
                    print_manpage();
                    free_config(&cfg);
                    return 0;
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

    const char *force_env = getenv("SLIT_FORCE_RENDER");
    int force_render = force_env != NULL && strcmp(force_env, "1") == 0;
    int can_render = is_stderr_tty() || force_render;

    if (!can_render) {
        passthrough_mode(&cfg);
        free_config(&cfg);
        return 0;
    }

    int tty_fd = term_init();
    if (tty_fd < 0) {
        passthrough_mode(&cfg);
        free_config(&cfg);
        return 0;
    }

    g_tty_fd = tty_fd;
    atexit(atexit_restore);

    signals_install();
    term_raw_enter(tty_fd);

    if (cfg.layout != LAYOUT_NONE) {
        term_set_title("slit");
    }

    struct ring_buffer *buf = buffer_create(cfg.max_lines);
    if (!buf) {
        term_restore_title();
        term_cleanup();
        free_config(&cfg);
        return 1;
    }

    struct tee_writer *tw = NULL;
    if (cfg.output) {
        tw = tee_open(cfg.output, cfg.append, cfg.tee_format);
    }

    debug_init(cfg.log_file, cfg.debug);
    debug_log("slit starting");

    long file_size = 0;
    struct stat st;
    if (fstat(STDIN_FILENO, &st) == 0 && S_ISREG(st.st_mode)) {
        file_size = st.st_size;
    }

    int term_rows = 24, term_cols = 80;
    term_get_size(tty_fd, &term_rows, &term_cols);

    struct layout_chrome chrome = layout_get_chrome(cfg.layout);
    int content_lines;
    if (cfg.lines <= 0) {
        int available = term_rows - chrome.top_lines - chrome.bottom_lines;
        if (available < 1) available = 1;
        content_lines = available * 2 / 3;
        if (content_lines < 10) content_lines = 10;
    } else {
        content_lines = cfg.lines;
    }
    if (content_lines < 1) content_lines = 1;

    int eof_reached = 0;
    int spinner_frame = 0;
    int render_count = 0;
    int prev_height = 0;
    int dirty = 0;
    struct timespec last_render_time = {0, 0};

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = tty_fd;
    fds[1].events = POLLIN;

    char *line = NULL;
    size_t line_len = 0;

    while (!sigint_flag && !eof_reached) {
        int ret = poll(fds, 2, RENDER_INTERVAL_MS);

        if (sigwinch_flag) {
            sigwinch_flag = 0;
            debug_log("SIGWINCH received");
            term_get_size(tty_fd, &term_rows, &term_cols);
            if (cfg.lines <= 0) {
                int available = term_rows - chrome.top_lines - chrome.bottom_lines;
                if (available < 1) available = 1;
                content_lines = available * 2 / 3;
                if (content_lines < 10) content_lines = 10;
            }
            dirty = 1;
        }

        if (sigtstp_flag) {
            sigtstp_flag = 0;
            term_raw_restore();
            term_restore_title();
            raise(SIGSTOP);
            term_raw_enter(tty_fd);
            if (cfg.layout != LAYOUT_NONE) term_set_title("slit");
            term_get_size(tty_fd, &term_rows, &term_cols);
            if (cfg.lines <= 0) {
                int available = term_rows - chrome.top_lines - chrome.bottom_lines;
                if (available < 1) available = 1;
                content_lines = available * 2 / 3;
                if (content_lines < 10) content_lines = 10;
            }
            dirty = 1;
        }

        if (ret < 0) continue;

        if (fds[0].revents & POLLIN) {
            int can_drain = (fds[0].revents & POLLHUP) != 0;
            do {
                ssize_t nread = getline(&line, &line_len, stdin);
                if (nread == -1) {
                    if (errno == EINTR) continue;
                    eof_reached = 1;
                    debug_log("EOF reached, total_lines=%zu", buffer_total_lines(buf));
                    break;
                }
                while (nread > 0 && (line[nread-1] == '\n' || line[nread-1] == '\r')) {
                    line[--nread] = '\0';
                }

                struct line_entry *entry = malloc(sizeof(struct line_entry));
                if (entry) {
                    entry->text = strdup(line);
                    entry->arrival = time(NULL);
                    entry->line_num = buffer_total_lines(buf) + 1;
                    entry->byte_len = nread;
                    buffer_push(buf, entry);
                }

                if (tw) {
                    if (cfg.tee_format == TEE_DISPLAY) {
                        char dtmp[4096], stripped[4096];
                        dtmp[0] = '\0';
                        int pos = 0;
                        if (cfg.timestamp) {
                            struct tm *tm_info = localtime(&(entry->arrival));
                            pos += snprintf(dtmp + pos, sizeof(dtmp) - pos,
                                            "%02d:%02d:%02d ",
                                            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
                        }
                        if (cfg.line_numbers) {
                            size_t tl = buffer_total_lines(buf);
                            int gw = 1;
                            while (tl >= 10) { tl /= 10; gw++; }
                            pos += snprintf(dtmp + pos, sizeof(dtmp) - pos,
                                            "%*zu ", gw, entry->line_num);
                        }
                        if (cfg.color == COLOR_NEVER || (cfg.color == COLOR_AUTO && !is_stderr_tty())) {
                            snprintf(dtmp + pos, sizeof(dtmp) - pos, "%s", line);
                        } else {
                            strip_ansi(line, stripped, sizeof(stripped));
                            snprintf(dtmp + pos, sizeof(dtmp) - pos, "%s", stripped);
                        }
                        tee_write_line(tw, dtmp);
                    } else {
                        tee_write_line(tw, line);
                    }
                }

                dirty = 1;
            } while (can_drain);
        }

        if ((fds[0].revents & POLLHUP) && !(fds[0].revents & POLLIN)) {
            eof_reached = 1;
        }

        if (fds[1].revents & POLLIN) {
            char c;
            ssize_t n = read(tty_fd, &c, 1);
            if (n > 0 && (c == 'q' || c == 'Q')) {
                sigint_flag = 1;
            }
        }

        int force_render_now = eof_reached || sigwinch_flag || sigtstp_flag;
        int timer_expired = (ret == 0);
        if (dirty || eof_reached || timer_expired) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - last_render_time.tv_sec) * 1000L +
                              (now.tv_nsec - last_render_time.tv_nsec) / 1000000L;
            if (!last_render_time.tv_sec || force_render_now || elapsed_ms >= RENDER_INTERVAL_MS) {
                last_render_time = now;
                render_count++;
                if (render_count % SPINNER_THROTTLE == 0)
                    spinner_frame++;
                render_frame(&cfg, buf, term_cols, term_rows, content_lines,
                             spinner_frame, eof_reached, file_size, prev_height, &prev_height);

                dirty = 0;
            }
        }
    }

    if (eof_reached && !sigint_flag) {
        spinner_frame++;
        render_frame(&cfg, buf, term_cols, term_rows, content_lines,
                     spinner_frame, 1, file_size, prev_height, &prev_height);

        struct pollfd wait_fd;
        wait_fd.fd = tty_fd;
        wait_fd.events = POLLIN;
        poll(&wait_fd, 1, 200);
    }

    term_restore_title();
    term_raw_restore();
    term_cleanup();
    if (tw) tee_close(tw);
    debug_log("slit exiting");
    debug_close();
    buffer_free(buf);
    free(line);
    free_config(&cfg);
    return 0;
}
