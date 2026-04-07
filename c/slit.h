/***
 * slit.h - Main header file for slit
 * 
 * Shared types, constants, and enums for the slit streaming viewer.
 */

#ifndef SLIT_H
#define SLIT_H

#include <stddef.h>
#include <time.h>
#include <signal.h>

#define VERSION "0.2.4"
#define DEFAULT_MAX_LINES 50000
#define DEFAULT_LINES -1
#define DEFAULT_PASSTHROUGH_LINES 10
#define DEFAULT_LAYOUT LAYOUT_MINIMAL
#define RENDER_INTERVAL_MS 10
#define SPINNER_THROTTLE 10
#define DEFAULT_SPINNER SPINNER_BRAILLE

enum layout_type {
    LAYOUT_BOX,
    LAYOUT_ROUNDED,
    LAYOUT_COMPACT,
    LAYOUT_MINIMAL,
    LAYOUT_NONE,
    LAYOUT_QUOTE
};

enum spinner_type {
    SPINNER_BRAILLE,
    SPINNER_DOTS,
    SPINNER_ARROWS,
    SPINNER_OFF
};

enum color_mode {
    COLOR_AUTO,
    COLOR_ALWAYS,
    COLOR_NEVER
};

enum tee_format {
    TEE_RAW,
    TEE_DISPLAY
};

struct line_entry {
    char *text;
    time_t arrival;
    size_t line_num;
    size_t byte_len;
};

struct slit_config {
    int lines;
    int max_lines;
    char *output;
    int append;
    enum tee_format tee_format;
    int line_numbers;
    enum color_mode color;
    int wrap;
    int timestamp;
    char *truncation_char;
    enum layout_type layout;
    char *quote_bg;
    enum spinner_type spinner;
    int debug;
    char *log_file;
};

extern volatile sig_atomic_t sigint_flag;
extern volatile sig_atomic_t sigwinch_flag;
extern volatile sig_atomic_t sigtstp_flag;

#endif /* SLIT_H */
