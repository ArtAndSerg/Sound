// File: "projectDebug.c"

#include <stdio.h>
#include <stdarg.h>
//#include <string.h>
#include <time.h>
#ifndef __NO_FREE_RTOS__   // defined in project options!
    #include "cmsis_os.h"
    #include <stdbool.h>
#endif
#include "projectDebug.h"

osMutexId dbgMutexHandle = NULL;

__weak void addToLog(char *text)
{
   // Prevent unused argument(s) compilation warning 
   ((void)(text));
}
//------------------------------------------------------------------------------

void dbgInit(void)
{
    osMutexDef(dbgMutex);
    dbgMutexHandle = osMutexCreate(osMutex(dbgMutex));
}
//------------------------------------------------------------------------------

void dbg_print_msg( const char *format, ...)
{
    static uint32_t n = 0; 
    static char str[256-15];
    int maxlen = sizeof( str ) - 1;
    struct tm t;
    time_t tme = time(NULL);     
    va_list argp;
    
    osMutexWait(dbgMutexHandle, osWaitForever);
    t = *gmtime(&tme);
    va_start( argp, format);
    printf("%d) %02d:%02d:%02d ", n++, t.tm_hour, t.tm_min, t.tm_sec);
    vprintf(format, argp);
    if (format[1] == DT_LOG || format[1] == DT_ALARM) {
        vsnprintf( str, maxlen, format, argp );
        str[maxlen] = '\0';
        addToLog(str);
    }
    va_end(argp);
    osMutexRelease(dbgMutexHandle);
}
//------------------------------------------------------------------------------

char *debug_fmt( const char *format, ... )
{
    va_list argp;
    static char str[64];
    int maxlen = sizeof( str ) - 1;

    
    va_start( argp, format );
    vsnprintf( str, maxlen, format, argp );
    va_end( argp );
    str[maxlen] = '\0';
    return( str );
}
//------------------------------------------------------------------------------

void debug_print_msg(int level, const char *file, int line, const char *text )
{
    char tmp;
    static char str[128];
    static int n = 0;
    struct tm t;
    int maxlen = sizeof( str ) - 1, len;
    time_t tme = time(NULL);
    
#ifdef portENTER_CRITICAL
    portENTER_CRITICAL();
#endif
    len = snprintf(str, maxlen, "%s", file);
    snprintf(&str[len-2], maxlen, "/%04d %s", line, text);
    str[maxlen] = '\0';
    if (level >= TRACE_DEBUG_LEVEL) {
        t = *gmtime(&tme);
        for (int i = 0; i < maxlen; i++) {
            if (str[i] == '\r' || str[i] == '\n') {
                str[i] = ' ';
            }
        }
        switch(level) {
          case DL_TRACE_DATA   : tmp = 'd'; break;
          case DL_TRACE_INFO   : tmp = 'i'; break;
          case DL_TRACE_WARNING: tmp = 'w'; break;
          case DL_TRACE_ERROR  : tmp = 'e'; break;
          case DL_TRACE_ALARM  : tmp = 'a'; break;
          case DL_LOG_INFO     : tmp = 'I'; break;
          case DL_LOG_SEND     : tmp = 'S'; break;
          case DL_LOG_ALARM    : tmp = 'A'; break;
          default              : tmp = '?';  
        }
        printf("%d. %c %02d:%02d:%02d %s\n", n++, tmp, t.tm_hour, t.tm_min, t.tm_sec, str);
    } 
#ifdef portEXIT_CRITICAL
    portEXIT_CRITICAL();
#endif
    if (level > DL_TRACE_ALARM) {
        addToLog(str);
    }
}
//------------------------------------------------------------------------------