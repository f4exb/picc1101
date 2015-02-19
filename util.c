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

// -------------------------------------------------------------------------------------------------
void _print_block(int verb_level, const uint8_t *pblock, size_t size)
// -------------------------------------------------------------------------------------------------
{
    size_t i;
    char   c;
    size_t lsize = 16;

    if (verb_level > verbose_level)
        return;

    fprintf(stderr, "     ");

    for (i=0; i<lsize; i++)
    {
        fprintf(stderr, "%X    ", i);
    }

    for (i=0; i<size; i++)
    {
        if (i % 16 == 0)
        {
            fprintf(stderr, "\n%03X: ", i);
        }

        if ((pblock[i] < 0x20) || (pblock[i] >= 0x7F))
        {
            c = '.';
        }
        else
        {
            c = (char) pblock[i];
        }

        fprintf(stderr, "%02X:%c ", pblock[i], c);
    }

    fprintf(stderr, "\n");
}