#pragma once

/** simple error handling stuff 
    --wbcbz7 l8.ob.zol8
**/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __WINDOWS_386__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

extern FILE *logfile;

void va_write(FILE *f, const char* format, ...);

void loginit(char* fname);
extern char *logerr_header, *logfatal_header, *logdebug_header;

#ifdef DEBUG_LOG

#ifdef __WINDOWS_386__

#define logerr(a, ...)    { const char *text = a, *fname = __FILE__; \
                            va_write(stderr, logerr_header,  fname, __LINE__);  \
                            va_write(logfile, logerr_header, fname, __LINE__); \
                            fprintf(stderr,  text,  ##__VA_ARGS__); \
                            fprintf(logfile, text,  ##__VA_ARGS__); }

#define logfatal(a, ...)  { const char *text = a, *fname = __FILE__; \
                            va_write(stderr, logfatal_header,  fname, __LINE__);  \
                            va_write(logfile, logfatal_header, fname, __LINE__); \
                            fprintf(stderr,  text,  ##__VA_ARGS__); \
                            fprintf(logfile, text,  ##__VA_ARGS__); \
                            exit(1);}

#else
    
#define logerr(a, ...)    { const char *text = a, *fname = __FILE__; \
                            va_write(stderr, logerr_header,  fname, __LINE__);  \
                            va_write(logfile, logerr_header, fname, __LINE__); \
                            fprintf(stderr,  text,  ##__VA_ARGS__); \
                            fprintf(logfile, text,  ##__VA_ARGS__); }

#define logfatal(a, ...)  { const char *text = a, *fname = __FILE__; \
                            va_write(stderr, logfatal_header,  fname, __LINE__);  \
                            va_write(logfile, logfatal_header, fname, __LINE__); \
                            fprintf(stderr,  text,  ##__VA_ARGS__); \
                            fprintf(logfile, text,  ##__VA_ARGS__); \
                            exit(1);}
#endif
                        

#define logwrite(...)    va_write(logfile, __VA_ARGS__);
#define logprint(...)    {  va_write(stderr,  __VA_ARGS__); \
                            va_write(logfile, __VA_ARGS__); \
                         }

#define logdebug(a, ...)    { const char *text = a, *fname = __FILE__; \
                            va_write(stderr, logdebug_header,  fname, __LINE__);  \
                            va_write(logfile, logdebug_header, fname, __LINE__); \
                            fprintf(stderr,  text,  ##__VA_ARGS__); \
                            fprintf(logfile, text,  ##__VA_ARGS__); }

#else 
#define logerr(a, ...)
#define logfatal(a, ...)
#define logwrite(...) 
#define logprint(...)
#define logdebug(a, ...)
#endif