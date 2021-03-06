
#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "my/VS1053.h"
#include "cmsis_os.h"
#define VS1053_Delay    osDelay
#define debugLog        printf

// Hardware config
#define VS1053_SPI      hspi1
#define VS1053_xCS      VS_CS_GPIO_Port, VS_CS_Pin
#define VS1053_xDCS     VS_DCS_GPIO_Port, VS_DCS_Pin
#define VS1053_xRST     VS_RESET_GPIO_Port, VS_RESET_Pin 
#define VS1053_DREQ     VS_DREQ_GPIO_Port, VS_DREQ_Pin

#define slog            printf
#define SPI_LOW_SPEED   SPI_BAUDRATEPRESCALER_256
#define SPI_HI_SPEED    SPI_BAUDRATEPRESCALER_16
#define DEFAULT_VOLUME  60
#define VS1053_TIMEOUT  300

#ifdef  osCMSIS
  extern osMutexId soundMutexHandle;
  #define __LOCK_VS1053()      if (xSemaphoreTake(soundMutexHandle, VS1053_TIMEOUT) != pdPASS) {xSemaphoreGive(soundMutexHandle); return VS1053_BUSY; }
  #define __UNLOCK_VS1053()   xSemaphoreGive(soundMutexHandle)
#else  
  #define __LOCK_VS1053()
  #define __UNLOCK_VS1053()
#endif  



extern SPI_HandleTypeDef  VS1053_SPI;
PLAYER_State curState = PLAYER_ERROR;
uint16_t currVolume = DEFAULT_VOLUME;

static void spiWrite (uint8_t *buf, int len);
static void spiRead  (uint8_t *buf, int len);
static void spiReadWrite (uint8_t *bufTx, uint8_t *bufRx, int len);
static VS1053_result writeReg(uint8_t addressbyte, uint16_t value);

static void spiWrite (uint8_t *buf, int len)
{
    if (HAL_SPI_Transmit(&VS1053_SPI, buf, len, 100) != HAL_OK) {      //trasmit data
        debugLog("ERROR! VS1053 SPI WRITE ERROR!!!\n");
    }
}
//-----------------------------------------------------------------------------

static void spiRead (uint8_t *buf, int len)
{
    if (HAL_SPI_Receive(&VS1053_SPI, buf, len, 100) != HAL_OK) {      //trasmit data
        debugLog("ERROR! VS1053 SPI READ ERROR!!!\n");
    }
}
//-----------------------------------------------------------------------------

static void spiReadWrite (uint8_t *bufTx, uint8_t *bufRx, int len)
{
    if (HAL_SPI_TransmitReceive(&VS1053_SPI, bufTx, bufRx, len, 100) != HAL_OK) {      //trasmit data
        debugLog("ERROR! VS1053 SPI READ <=> WRITE ERROR!!!\n");
    }
}
//-----------------------------------------------------------------------------


static void vs1053_spiSpeed(uint8_t speed) 
{
	//CHECK YOUR APB1 FREQ!!!
	VS1053_SPI.Init.BaudRatePrescaler = speed;
    for (int i = 1; i < 100; i++) {
        if (HAL_SPI_Init(&VS1053_SPI) == HAL_OK) {
            return;
        }
    }
    debugLog("ERROR! Can't init SPI!");
}
//----------------------------------------------------------------------------


//Write to VS10xx register
//SCI: Data transfers are always 16bit. When a new SCI operation comes in 
//DREQ goes low. We then have to wait for DREQ to go high again.
//XCS should be low for the full duration of operation.
static VS1053_result writeReg(uint8_t addressbyte, uint16_t value)
{
	//taskENTER_CRITICAL();
  uint32_t timer = HAL_GetTick();
  uint8_t  buf[4] = {0x02}; //Write instruction
  vs1053_spiSpeed(SPI_LOW_SPEED);    
  while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET) { //Wait for DREQ to go high indicating IC is available
      if (HAL_GetTick() - timer > VS1053_TIMEOUT) {
          debugLog("ERROR! VS1053 DREQ pin timeout at start of cmd(0x%02X, 0x%04X)!\n", addressbyte, value);
          curState = PLAYER_ERROR;
          return VS1053_ERROR;
      }
  }
  HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET); //Select control
  //SCI consists of instruction byte, address byte, and 16-bit data word.
  buf[1] = addressbyte;
  buf[2] = value >> 8;
  buf[3] = value & 0xFF;
  spiWrite(buf, 4);
  timer = HAL_GetTick();
  
  while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET) { //Wait for DREQ to go high indicating command is complete
	if (HAL_GetTick() - timer > VS1053_TIMEOUT) {
          debugLog("ERROR! VS1053 DREQ pin timeout at end of cmd(0x%02X, 0x%04X)!\n", addressbyte, value);
          HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_SET); //Deselect Control
          curState = PLAYER_ERROR;
          return VS1053_ERROR;
    }
  }
  HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_SET); //Deselect Control
  //taskEXIT_CRITICAL();
  vs1053_spiSpeed(SPI_HI_SPEED);
  return VS1053_OK;
}
//------------------------------------------------------------------------------

//Read the 16-bit value of a VS10xx register
static uint16_t readReg (uint8_t addressbyte) {
	
  //taskENTER_CRITICAL();
  uint32_t timer = HAL_GetTick();
  uint8_t  respH, respL, buf[4] = {0x03}; //Read instruction
  
  vs1053_spiSpeed(SPI_LOW_SPEED);    
  while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET) { //Wait for DREQ to go high indicating IC is available
      if (HAL_GetTick() - timer > VS1053_TIMEOUT) {
          debugLog("ERROR! VS1053 DREQ pin timeout at start of read(0x%02X)!\n", addressbyte);
          curState = PLAYER_ERROR;
          return 0xFFFF;
      }
  }
  HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET); //Select control
  //SCI consists of instruction byte, address byte, and 16-bit data word.
  buf[1] = addressbyte;
  spiWrite(buf, 2);
  
  spiRead (&respH, 1);
  timer = HAL_GetTick();
  while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET) { //Wait for DREQ to go high indicating command is complete
	if (HAL_GetTick() - timer > VS1053_TIMEOUT) {
          debugLog("ERROR! VS1053 DREQ pin timeout at H-byte read(0x%02X)!\n", addressbyte);
          HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_SET); //Deselect Control
          curState = PLAYER_ERROR;
          return 0xFFFF;
    }
  }

  spiRead (&respL, 1);
  timer = HAL_GetTick();
  while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET) { //Wait for DREQ to go high indicating command is complete
	if (HAL_GetTick() - timer > VS1053_TIMEOUT) {
          debugLog("ERROR! VS1053 DREQ pin timeout at L-byte read(0x%02X)!\n", addressbyte);
          HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_SET); //Deselect Control
          curState = PLAYER_ERROR;
          return 0xFFFF;
    }
  }
  HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_SET); //Deselect Control
  //taskEXIT_CRITICAL();
  vs1053_spiSpeed(SPI_HI_SPEED);
  
  return ((uint16_t)respH << 8) | respL;
}
//------------------------------------------------------------------------------
 
VS1053_result VS1053_Init(void)
{
    __LOCK_VS1053();
    VS1053_result res = VS1053_OK;
    
    debugLog("\nVS1053 init... ");
	HAL_GPIO_WritePin(VS1053_xCS,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET); //deSelect data
	HAL_GPIO_WritePin(VS1053_xRST, GPIO_PIN_RESET);
	VS1053_Delay(1);
	HAL_GPIO_WritePin(VS1053_xRST, GPIO_PIN_SET);
	VS1053_Delay(3);
	//sine_test();
    res = writeReg(SCI_MODE, SM_SDINEW);
    res = writeReg(SCI_CLOCKF, 0x8800); 
    
    int MP3Mode   = readReg(SCI_MODE);
	int MP3Status = readReg(SCI_STATUS);
	int MP3Clock  = readReg(SCI_CLOCKF);
    int vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
    printf("SCI_Mode (0x4800) = 0x%x\n", MP3Mode);
    printf("SCI_Status (0x48) = 0x%x\n", MP3Status);
    printf("VS Version (VS1053 is 4) = 0x%x\n", vsVersion); //The 1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1003 = 3
    printf("SCI_ClockF = 0x%x\n", MP3Clock);
    res = writeReg(SCI_VOL, currVolume | (currVolume << 8));  
    if (res == VS1053_OK) {
        debugLog("ok.\n");
    } else {
        debugLog("error - %d !\n", (int)res);
    }   
 
    __UNLOCK_VS1053(); 
        
    return res;
}
//----------------------------------------------------------------------------

VS1053_result VS1053_setVolume(int vol)
{
    __LOCK_VS1053();
    if (vol > 100) {
        vol = 100;
    }
    if (vol < 0) {
        vol = 0;
    }
    
    currVolume = 100 - vol;
    VS1053_result res = writeReg(SCI_VOL, currVolume | (currVolume << 8));  
    
    __UNLOCK_VS1053();
        
    return res;
}
//----------------------------------------------------------------------------

PLAYER_State VS1053_getState(void) {
    return curState;
}
//----------------------------------------------------------------------------

VS1053_result VS1053_play(void) 
{
    VS1053_result res = VS1053_ERROR;
    curState = PLAYER_PLAY;  
    if (VS1053_Init() == VS1053_OK) {
        __LOCK_VS1053(); 
        res = writeReg(SCI_DECODE_TIME, 0);  
        __UNLOCK_VS1053();
    }
    return res;
}
//----------------------------------------------------------------------------

VS1053_result VS1053_addData(uint8_t *buf, int size)
{
    int writePtr = 0, timeout = 0;
    
    
    if (!size) {
        curState = PLAYER_STOP;
        return VS1053_OK;
    }
    __LOCK_VS1053();
    
    //HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_RESET); //Select data
	while(size > 0 && timeout < 200) {
        if (HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_SET) {
            timeout = 0;
            HAL_SPI_Transmit(&VS1053_SPI, buf, VS1053_MAX_TRANSFER_SIZE, 100);      //trasmit data
            buf += VS1053_MAX_TRANSFER_SIZE;
            size -= VS1053_MAX_TRANSFER_SIZE;
        } else {
            VS1053_Delay(1);
            timeout++;
        }
    }
	HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET);   //deSelect data
  
    __UNLOCK_VS1053();
    if (timeout < 200) {
        return VS1053_OK;
    }
    return VS1053_ERROR;
}
//----------------------------------------------------------------------------

/*
int VS1053_process(void) // return count of needed more bytes 
{
    static PLAYER_State pState = PLAYER_STOP;
    static uint32_t timer = 0;
    VS1053_result res = VS1053_OK;
    int neededDataLen = 0;    
    
    switch (curState) {
      case PLAYER_STOP: 
        break;
      
      case PLAYER_PLAY:
        if (pState != PLAYER_PLAY && pState != PLAYER_PAUSE) {
            
        } else {
            if (HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_SET) {
                neededDataLen = VS1053_MAX_TRANSFER_SIZE;
            } 
        }
        break;
           
      case PLAYER_PAUSE:
        break;
      
      case PLAYER_ERROR: 
        if (HAL_GetTick() - timer > 1000) {
            if (VS1053_Init() == VS1053_OK) {
                curState = PLAYER_STOP; 
            } else {
                timer = HAL_GetTick();              
            }
        }
        break;
    }
    if (curState != pState) {
        timer = HAL_GetTick();
    }
    pState = curState;
    return neededDataLen;
}
//----------------------------------------------------------------------------
*/

VS1053_result VS1053_sineTest(void)
{	
    uint8_t data[] = {0x53, 0xef, 0x6e, 0x44, 0x00, 0x00, 0x00, 0x00};
    VS1053_result res;    
	curState = PLAYER_PLAY;
    VS1053_process();

    debufLog("Sine test...\n");
	res = writeReg(SCI_VOL, DEFAULT_VOLUME | (DEFAULT_VOLUME << 8));  
	res = writeReg(SCI_MODE, SM_SDINEW | SM_TESTS);
	debufLog("SCI_Mode = 0x%x\n", readReg(SCI_MODE));
    debufLog("Result = %d\n", (int)res);
    
    return res;
}
//----------------------------------------------------------------------------






/*











void set_vol(uint8_t vol)
{
	VSvolume = 10 + (100 - vol)*130/100;
}
void vs1053_read_parametric(void)
{
	parametric.version = vs1053_read_wram_16(PAR_VERSION);
	parametric.config1 = vs1053_read_wram_16(PAR_CONFIG1);
	parametric.playSpeed = vs1053_read_wram_16(PAR_PLAY_SPEED);
	
	slog("par %d %d %d", parametric.version, parametric.config1, parametric.playSpeed);
}


uint8_t _vs1053_par_start = 0;
void vs1053_update_parametric(void)
{
	if(_vs1053_par_start)
		return;
	vs1053_spiSpeed(0);
	_vs1053_par_start = 1;
	//parametric.byteRate = vs1053_read_wram_16(PAR_VERSION);
	//parametric.endFillByte = vs1053_read_wram_16(PAR_CONFIG1);
	//parametric.playSpeed = vs1053_read_wram_16(PAR_PLAY_SPEED);
	parametric.positionMsec = vs1053_read_reg(SCI_DECODE_TIME)*1000;//vs1053_read_wram_32(PAR_POSITION_MSEC);
	_vs1053_par_start = 0;
	vs1053_spiSpeed(1);
}

void VS1053_play_file(FIL* file)
{
	VS1053_curFile = file;
	VS1053_curState = PLAYER_PLAY;
	VS1053_filechanged = 1;
}

uint8_t VS1053_getState(void)
{
	return VS1053_curState;
}
void VS1053_pause(void)
{
	if(VS1053_curState == PLAYER_PLAY)
		VS1053_curState = PLAYER_PAUSE;
}
void VS1053_unpause(void)
{
	if(VS1053_curState == PLAYER_PAUSE)
		VS1053_curState = PLAYER_PLAY;
}

uint8_t _VS1053_curState = PLAYER_STOP;
FIL* _VS1053_curFile = 0;

void VS1053_thread(void)
{
	slog("VS1053_thread");
	uint8_t need = 1, last = PLAYER_STOP;
	static uint8_t buf[32] = {0};
	UINT br = 0;
	static uint8_t res = 0, stat = 0;

	slog("\n\nVS1053_thread started\n");
	vs1053_write_reg_16(SCI_DECODE_TIME, 0); //reset time
    HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET); //Select control
   // vs1053_spiSpeed(0);
	while(1)
	{		
        while(_VS1053_curState != PLAYER_STOP && VS1053_filechanged != 1 && (VS1053_curFile == _VS1053_curFile || VS1053_curFile == 0) && _VS1053_curFile != 0)
		{
			if (stat) {
                printf("SCI_Status (0x48) = 0x%x\n", vs1053_read_reg(SCI_STATUS));
                stat = 0;
            }
			while(VS1053_curState == PLAYER_PAUSE) //PAUSE
			{
				vs1053_update_parametric(); 
				osDelay(50);
			}
			last = _VS1053_curState;
			while(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET)
			{
				if(need)
				{
					res = f_read(_VS1053_curFile, &buf, 32, &br);
					if(res != FR_OK)
					{
						slog("end 1");
						need = 0;
						_VS1053_curState = PLAYER_STOP;
					}
					//slog("br %d, r %d", br, res);
					need = 0;
					//vs1053_update_parametric();
				}
				if(VSvolume != VScurvolume)
				{
					vs1053_spiSpeed(0);
					vs1053_write_reg(SCI_VOL, VSvolume, VSvolume);
					VScurvolume = VSvolume;
					vs1053_spiSpeed(1);
				}
			}			//Wait for DREQ to go high indicating IC is available
			
			if(need) //if there weren't freetime
			{
				res = f_read(_VS1053_curFile, &buf, 32, &br);
				if(res != FR_OK)
				{
					slog("end 2");
					_VS1053_curState = PLAYER_STOP;
				}
				//slog("br %d, r %d", br, res);
				need = 0;
			}
			if(_VS1053_curState != PLAYER_STOP)
			{
				taskENTER_CRITICAL();
				//HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_RESET); //Select data
				HAL_SPI_Transmit(&VS1053_SPI, buf, br, 1);      //trasmit data
				HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET);   //deSelect data
				taskEXIT_CRITICAL();
			
				need = 1;
			}
		}
		
		if(//0 && //test!!!!
			(last != PLAYER_STOP || (VS1053_curFile != _VS1053_curFile && VS1053_curFile != 0) || VS1053_filechanged == 1))
		{
			slog("STOP playing");
			last = PLAYER_STOP;
			_VS1053_curState = PLAYER_STOP;
			
			memset(&buf, parametric.endFillByte, sizeof(buf));
			br = VS1053_END_FILL_BYTES;
			uint32_t i = 0;
			HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_RESET); //Select control
			for(i = 0; i <= VS1053_END_FILL_BYTES; i += 32)
			{
				taskENTER_CRITICAL();
				HAL_SPI_Transmit(&VS1053_SPI, buf, br, 1);      //trasmit data
				taskEXIT_CRITICAL();
			}
			HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET);   //DESelect control
			vs1053_write_reg_16(SCI_MODE, vs1053_read_reg(SCI_MODE) | SM_CANCEL);
			//while(vs1053_read_reg(SCI_MODE) & SM_CANCEL)
			//{
			//	taskENTER_CRITICAL();
			//	HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_RESET); //Select control
			//	HAL_SPI_Transmit(&VS1053_SPI, buf, 2, 1);      //trasmit data
			//	HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET);   //DESelect control
			//	taskEXIT_CRITICAL();
			//	osDelay(1);
			//}
			slog("Stopped");
		}
		osDelay(50);
		
		if(VS1053_curState == PLAYER_PLAY && VS1053_curFile != 0)
		{
			slog("Start play, %d", vs1053_read_reg(SCI_CLOCKF));
			_VS1053_curState = PLAYER_PLAY;
			vs1053_write_reg_16(SCI_DECODE_TIME, 0); //reset time
            vs1053_write_reg(SCI_VOL, 60, 60);
            _VS1053_curFile = VS1053_curFile;
            vs1053_read_parametric();
			VS1053_filechanged  = 0;
            vs1053_spiSpeed(1);
            
       while(1) {
            if (HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_SET)
			{
					res = f_read(_VS1053_curFile, &buf, 32, &br);
					taskENTER_CRITICAL();
				    HAL_GPIO_WritePin(VS1053_xCS, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_RESET); //Select data
				    HAL_SPI_Transmit(&VS1053_SPI, buf, 32, 100);      //trasmit data
				    HAL_GPIO_WritePin(VS1053_xDCS, GPIO_PIN_SET);   //deSelect data
				    taskEXIT_CRITICAL();
            } else {
                buf[0] = 0;
            }
       }     
            
		}
		else
		{
			VS1053_curState = PLAYER_STOP;
		}
	}
}


//uint32_t lst = 0;
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//	if( lst == 0)
//		lst = HAL_GetTick();
//	if(HAL_GPIO_ReadPin(VS1053_DREQ) == GPIO_PIN_RESET)
//	{
//		slog("tm:%d",  HAL_GetTick() - lst);
//		lst = HAL_GetTick();
//	}
//}
*/
