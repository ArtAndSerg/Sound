
//file: "VS1053.h"

#ifndef VS1053_H
#define VS1053_H

#include <stdint.h>
#include "my/vs1053_defs.h"

#define VS1053_END_FILL_BYTES    2050
#define VS1053_MAX_TRANSFER_SIZE 32

typedef enum {
    VS1053_OK = 0,
    VS1053_INITING,
    VS1053_ERROR,
    VS1053_BUSY
} VS1053_result;

typedef enum {
    PLAYER_STOP = 0,
    PLAYER_PLAY,
    PLAYER_PAUSE,
    PLAYER_ERROR
} PLAYER_State;

PLAYER_State  VS1053_getState(void);
VS1053_result VS1053_play(void);
VS1053_result VS1053_stop(void);
VS1053_result VS1053_pause(void);
VS1053_result VS1053_unpause(void);
VS1053_result VS1053_Init(void);
int VS1053_process(void); // return count of needed more bytes 
VS1053_result VS1053_addData(uint8_t *buf, int size);
VS1053_result VS1053_setVolume(uint8_t vol);

#endif
