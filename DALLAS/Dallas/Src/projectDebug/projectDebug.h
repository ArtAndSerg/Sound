// File: "projectDebug.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PROJECT_DEBUG_H
#define __PROJECT_DEBUG_H


#define DL_TRACE_DATA    0
#define DL_TRACE_INFO    1
#define DL_TRACE_WARNING 2
#define DL_TRACE_ERROR   3
#define DL_TRACE_ALARM   4
#define DL_LOG_INFO  20
#define DL_LOG_SEND  30
#define DL_LOG_ALARM 40

 #define TRACE_DEBUG_LEVEL    DL_TRACE_DATA

#ifdef __NO_FREE_RTOS__ 
    
    #define DEBUG_MSG(level, args)   

#else

    #define sizeoffield(s, f)   (sizeof(((s *)0)->f))
    #define DEBUG_MSG(level, args)   ;

    char *debug_fmt(const char *format, ... );
    void debug_print_msg(int level, const char *file, int line, const char *text ); 

    #endif

  


// debug messages types:
#define DT_TRACE   't'
#define DT_INFO    'i'
#define DT_ERROR   'e'
#define DT_LOG     'L'          
#define DT_ALARM   'A'

#define QUOTE_(LINE_NUMBER) #LINE_NUMBER
#define QUOTE(LINE_NUMBER) QUOTE_(LINE_NUMBER)
#define dbg(level, type, format, ...) do { if (level >= type) dbg_print_msg (QUOTE(type)"/"__FILE__"/"QUOTE(__LINE__)"\t "format"\n", ## __VA_ARGS__); } while(0)

void dbgInit(void);
void dbg_print_msg( const char *format, ...);   
void addToLog(char *text);

#endif
    
