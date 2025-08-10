#include "utils.h"

#include <stdio.h>
#include <stdarg.h>

void debug_print(char* s,...) {
    if (DEBUG) {
        va_list arg;
        va_start(arg, s);
        printf(s, arg);
    }
}