
#include <stdarg.h>
#include <stdio.h>
#include "os_printf.h"

ssize_t os_printf(const char* pFormat, ...)
{
    ssize_t RetVal = 0;

    va_list args;
    va_start(args, pFormat);
    RetVal = vprintf(pFormat, args);
    va_end(args);

    return RetVal;
}
