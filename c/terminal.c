#include "terminal.h"
#include "slit.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t sigint_flag = 0;
volatile sig_atomic_t sigwinch_flag = 0;
volatile sig_atomic_t sigtstp_flag = 0;
volatile sig_atomic_t sigcont_flag = 0;

static struct terminal_state term_state = {
    .tty_fd = -1,
    .orig_termios = {0},
    .is_raw = 0
};

static void sigint_handler(int sig) {
    (void)sig;
    sigint_flag = 1;
}

static void sigwinch_handler(int sig) {
    (void)sig;
    sigwinch_flag = 1;
}

static void sigtstp_handler(int sig) {
    (void)sig;
    sigtstp_flag = 1;
}

static void sigcont_handler(int sig) {
    (void)sig;
    sigcont_flag = 1;
}

void signals_install(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigint_handler;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sigint_handler;
    sigaction(SIGHUP, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);

    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);

    sa.sa_handler = sigtstp_handler;
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = sigcont_handler;
    sa.sa_flags = 0;
    sigaction(SIGCONT, &sa, NULL);
}

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
    raw.c_oflag |= OPOST | ONLCR;
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

void term_set_title(const char *title) {
    if (term_state.tty_fd >= 0) {
        dprintf(term_state.tty_fd, "\x1b]0;%s\x07", title);
    }
}

void term_restore_title(void) {
    if (term_state.tty_fd >= 0) {
        dprintf(term_state.tty_fd, "\x1b]0;\x07");
    }
}
