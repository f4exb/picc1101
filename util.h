#ifndef _UTIL_H_
#define _UTIL_H_

#include <inttypes.h>

extern int verbose_level;

void _verbprintf(int verb_level, const char *fmt, ...);

#if !defined(MAX_VERBOSE_LEVEL)
#   define MAX_VERBOSE_LEVEL 0
#endif
#define verbprintf(level, ...) \
    do { if (level <= MAX_VERBOSE_LEVEL) _verbprintf(level, __VA_ARGS__); } while (0)

#endif