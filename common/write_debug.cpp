#include "write_debug.h"
#include <cstdarg>
#include <cstdio>

void write_debug(const char *fmt, ...)
{
#if 0
    va_list argp;
    va_start(argp, fmt);

    vprintf(fmt, argp);
    va_end(argp);
#endif
}

