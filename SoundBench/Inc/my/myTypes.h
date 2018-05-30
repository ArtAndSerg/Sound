//file: muTypes.h
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYTYPES_H__
#define __MYTYPES_H__

#define MAX_FILENAME_LEN   16
#define MAX_LCDMESSAGE_LEN 23

typedef struct {
    char string[MAX_FILENAME_LEN];
} filename_t;

typedef struct {
    char msg[MAX_LCDMESSAGE_LEN];
    unsigned char duration; 
} LCDmsg_t;

#endif