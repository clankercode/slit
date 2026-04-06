/*** 
 * terminal.c - Terminal control implementation
 * 
 * Functions for managing terminal state, raw mode, and size queries.
 */

#include "terminal.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

static struct terminal_state term_state = {
    .tty_fd = -1,
    .orig_termios = {0},
    .is_raw = 0
};

int term_init(void) {
    if (term_state.tty_fd >= 0) {
        return term_state.tty_fd;
    }
    
    int fd = open("/dev/tty", O_RDWR);
    if (fd < 0) {
        return -1;
    }
    
    term_state.tty_fd = fd;
    term_state.is_raw = 0;
    return fd;
}

void term_raw_enter(int fd) {
    if (term_state.is_raw) {
        return;
    }
    
    if (tcgetattr(fd, &term_state.orig_termios) < 0) {
        return;
    }
    
    struct termios raw = term_state.orig_termios;
    
    raw.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) {
        return;
    }
    
    term_state.is_raw = 1;
}

void term_raw_restore(void) {
    if (!term_state.is_raw || term_state.tty_fd < 0) {
        return;
    }
    
    tcsetattr(term_state.tty_fd, TCSAFLUSH, &term_state.orig_termios);
    term_state.is_raw = 0;
}

void term_get_size(int fd, int *rows, int *cols) {
    struct winsize ws;
    
    if (ioctl(fd, TIOCGWINSZ, &ws) < 0) {
        *rows = 24;
        *cols = 80;
        return;
    }
    
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

void term_cleanup(void) {
    term_raw_restore();
    
    if (term_state.tty_fd >= 0) {
        close(term_state.tty_fd);
        term_state.tty_fd = -1;
    }
}

int is_stdin_tty(void) {
    return isatty(STDIN_FILENO);
}

int is_stderr_tty(void) {
    return isatty(STDERR_FILENO);
}
