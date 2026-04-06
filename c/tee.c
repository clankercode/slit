#include "tee.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tee_writer *tee_open(const char *path, int append, enum tee_format format) {
    FILE *fp = fopen(path, append ? "a" : "w");
    if (!fp) {
        fprintf(stderr, "warning: could not open tee file: %s\n", path);
        return NULL;
    }

    struct tee_writer *tw = malloc(sizeof(struct tee_writer));
    if (!tw) { fclose(fp); return NULL; }

    tw->fp = fp;
    tw->format = format;
    return tw;
}

void tee_write_line(struct tee_writer *tw, const char *line) {
    if (!tw || !tw->fp || !line) return;
    fputs(line, tw->fp);
    fputc('\n', tw->fp);
    fflush(tw->fp);
}

void tee_close(struct tee_writer *tw) {
    if (!tw) return;
    if (tw->fp) fclose(tw->fp);
    free(tw);
}
