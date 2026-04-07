#include "configfile.h"
#include "slit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static int cf_parse_layout(const char *s) {
    if (strcmp(s, "box") == 0) return LAYOUT_BOX;
    if (strcmp(s, "rounded") == 0) return LAYOUT_ROUNDED;
    if (strcmp(s, "compact") == 0) return LAYOUT_COMPACT;
    if (strcmp(s, "minimal") == 0) return LAYOUT_MINIMAL;
    if (strcmp(s, "none") == 0) return LAYOUT_NONE;
    if (strcmp(s, "quote") == 0) return LAYOUT_QUOTE;
    return -1;
}

static int cf_parse_color(const char *s) {
    if (strcmp(s, "auto") == 0) return COLOR_AUTO;
    if (strcmp(s, "always") == 0) return COLOR_ALWAYS;
    if (strcmp(s, "never") == 0) return COLOR_NEVER;
    return -1;
}

static int cf_parse_spinner(const char *s) {
    if (strcmp(s, "braille") == 0) return SPINNER_BRAILLE;
    if (strcmp(s, "dots") == 0) return SPINNER_DOTS;
    if (strcmp(s, "arrows") == 0) return SPINNER_ARROWS;
    if (strcmp(s, "off") == 0) return SPINNER_OFF;
    return -1;
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

static char *find_config_path(void) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0]) {
        size_t len = strlen(xdg) + strlen("/slit/config.toml") + 1;
        char *path = malloc(len);
        if (!path) return NULL;
        snprintf(path, len, "%s/slit/config.toml", xdg);
        if (access(path, R_OK) == 0) return path;
        free(path);
    }
    const char *home = getenv("HOME");
    if (home && home[0]) {
        size_t len = strlen(home) + strlen("/.config/slit/config.toml") + 1;
        char *path = malloc(len);
        if (!path) return NULL;
        snprintf(path, len, "%s/.config/slit/config.toml", home);
        if (access(path, R_OK) == 0) return path;
        free(path);
    }
    return NULL;
}

static void unquote(char *s) {
    size_t len = strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len-1] == '"') || (s[0] == '\'' && s[len-1] == '\''))) {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

static void apply_kv(struct slit_config *cfg, const char *section, const char *key, const char *val) {
    if (strcmp(section, "display") == 0) {
        if (strcmp(key, "layout") == 0) {
            int v = cf_parse_layout(val);
            if (v >= 0) cfg->layout = v;
        } else if (strcmp(key, "lines") == 0) {
            int v = atoi(val);
            if (v >= 0) cfg->lines = v;
        } else if (strcmp(key, "line_numbers") == 0) {
            if (strcmp(val, "true") == 0) cfg->line_numbers = 1;
            else if (strcmp(val, "false") == 0) cfg->line_numbers = 0;
        } else if (strcmp(key, "color") == 0) {
            int v = cf_parse_color(val);
            if (v >= 0) cfg->color = v;
        } else if (strcmp(key, "wrap") == 0) {
            if (strcmp(val, "true") == 0) cfg->wrap = 1;
            else if (strcmp(val, "false") == 0) cfg->wrap = 0;
        } else if (strcmp(key, "timestamp") == 0) {
            if (strcmp(val, "true") == 0) cfg->timestamp = 1;
            else if (strcmp(val, "false") == 0) cfg->timestamp = 0;
        } else if (strcmp(key, "truncation_char") == 0) {
            char *dup = strdup(val);
            if (dup) { free(cfg->truncation_char); cfg->truncation_char = dup; }
        }
    } else if (strcmp(section, "buffer") == 0) {
        if (strcmp(key, "max_lines") == 0) {
            int v = atoi(val);
            if (v > 0) cfg->max_lines = v;
        }
    } else if (strcmp(section, "spinner") == 0) {
        if (strcmp(key, "style") == 0) {
            int v = cf_parse_spinner(val);
            if (v >= 0) cfg->spinner = v;
        }
    } else if (strcmp(section, "quote") == 0) {
        if (strcmp(key, "bg") == 0) {
            char *dup = strdup(val);
            if (dup) { free(cfg->quote_bg); cfg->quote_bg = dup; }
        }
    } else if (strcmp(section, "debug") == 0) {
        if (strcmp(key, "enabled") == 0) {
            if (strcmp(val, "true") == 0) cfg->debug = 1;
            else if (strcmp(val, "false") == 0) cfg->debug = 0;
        } else if (strcmp(key, "log_file") == 0) {
            char *dup = strdup(val);
            if (dup) { free(cfg->log_file); cfg->log_file = dup; }
        }
    }
}

void load_config_file(struct slit_config *cfg) {
    char *path = find_config_path();
    if (!path) return;

    FILE *fp = fopen(path, "r");
    if (!fp) { free(path); return; }

    char section[64] = "";
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *t = trim(line);
        if (*t == '\0' || *t == '#') continue;

        if (*t == '[') {
            char *end = strchr(t, ']');
            if (end) {
                *end = '\0';
                char *name = trim(t + 1);
                snprintf(section, sizeof(section), "%s", name);
            }
            continue;
        }

        char *eq = strchr(t, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim(t);
        char *val = trim(eq + 1);
        unquote(val);

        if (section[0] && key[0] && val[0]) {
            apply_kv(cfg, section, key, val);
        }
    }

    fclose(fp);
    free(path);
}
