#ifndef _UTIL_H_
#define _UTIL_H_

#include <inttypes.h>
#include <sys/time.h>

extern int verbose_level;

void _verbprintf(int verb_level, const char *fmt, ...);
void _print_block(int verb_level, const uint8_t *pblock, size_t size);

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
uint32_t ts_us(struct timeval *x);

#if !defined(MAX_VERBOSE_LEVEL)
#   define MAX_VERBOSE_LEVEL 0
#endif
#define verbprintf(level, ...) \
    do { if (level <= MAX_VERBOSE_LEVEL) _verbprintf(level, __VA_ARGS__); } while (0)

#define print_block(level, pblock, size) \
    do { if (level <= MAX_VERBOSE_LEVEL) _print_block(level, pblock, size); } while(0)

#endif