/*** 
 * terminal.h - Terminal control for slit
 * 
 * Functions for managing terminal state, raw mode, and size queries.
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>

struct terminal_state {
    int tty_fd;
    struct termios orig_termios;
    int is_raw;
};

int term_init(void);
void term_raw_enter(int fd);
void term_raw_restore(void);
void term_get_size(int fd, int *rows, int *cols);
void term_cleanup(void);
int is_stdin_tty(void);
int is_stderr_tty(void);

#endif /* TERMINAL_H */
