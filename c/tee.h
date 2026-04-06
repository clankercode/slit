#ifndef TEE_H
#define TEE_H

#include "slit.h"
#include <stdio.h>

struct tee_writer {
    FILE *fp;
    enum tee_format format;
};

struct tee_writer *tee_open(const char *path, int append, enum tee_format format);
void tee_write_line(struct tee_writer *tw, const char *line);
void tee_close(struct tee_writer *tw);

#endif
