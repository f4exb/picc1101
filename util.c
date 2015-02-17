#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include "util.h"

int verbose_level = 0;

// -------------------------------------------------------------------------------------------------
void _verbprintf(int verb_level, const char *fmt, ...)
// -------------------------------------------------------------------------------------------------
{
	int dont_flush = 1;

    if (verb_level > verbose_level)
        return;

    va_list args;
    va_start(args, fmt);

    {
        vfprintf(stderr, fmt, args);
        if(!dont_flush)
            fflush(stderr);
    }

    va_end(args);
}
