#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "logerror.h"

FILE *logfile = NULL;

char *logerr_header = ">> ERROR [%s:%d]: ", *logfatal_header = ">> FATAL [%s:%d]: ", *logdebug_header = ">> DEBUG [%s:%d]: ";

// somewhere from stack overflow :]
static void format_time(FILE *f) {
    if (!f) return;
    
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime );
    timeinfo = localtime(&rawtime);

    fprintf(f, "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void va_write(FILE *f, const char* format, ...) {
    if (!f) return;
    
    va_list args;
    
    // print current time
    format_time(f);
    
    // print log message itself
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
}

void loginit(char* fname)
{
    FILE* f = fopen(fname, "w");
    if (f != NULL) logfile = f; else printf("can't create logfile!");
}
