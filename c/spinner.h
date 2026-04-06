#ifndef SPINNER_H
#define SPINNER_H

#include "slit.h"

const char *spinner_frame(enum spinner_type type, int frame_idx);
int spinner_frame_count(enum spinner_type type);

#endif
