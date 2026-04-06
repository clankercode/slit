#include "spinner.h"

static const char *braille_frames[] = {
    "\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
    "\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
    "\xe2\xa0\x87", "\xe2\xa0\x8f"
};
static const int braille_count = 10;

static const char *dots_frames[] = {
    "\xe2\xa3\xbe", "\xe2\xa3\xbd", "\xe2\xa3\xbb", "\xe2\xa2\xbf",
    "\xe2\xa1\xbf", "\xe2\xa3\x9f", "\xe2\xa3\xaf", "\xe2\xa3\xb7"
};
static const int dots_count = 8;

static const char *arrows_frames[] = {
    "\xe2\x86\x90", "\xe2\x86\x96", "\xe2\x86\x91", "\xe2\x86\x97",
    "\xe2\x86\x92", "\xe2\x86\x98", "\xe2\x86\x93", "\xe2\x86\x99"
};
static const int arrows_count = 8;

const char *spinner_frame(enum spinner_type type, int frame_idx) {
    switch (type) {
        case SPINNER_BRAILLE: return braille_frames[frame_idx % braille_count];
        case SPINNER_DOTS:    return dots_frames[frame_idx % dots_count];
        case SPINNER_ARROWS:  return arrows_frames[frame_idx % arrows_count];
        case SPINNER_OFF:     return "";
    }
    return "";
}

int spinner_frame_count(enum spinner_type type) {
    switch (type) {
        case SPINNER_BRAILLE: return braille_count;
        case SPINNER_DOTS:    return dots_count;
        case SPINNER_ARROWS:  return arrows_count;
        case SPINNER_OFF:     return 1;
    }
    return 1;
}
