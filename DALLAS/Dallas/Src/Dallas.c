
#include "Dallas.h"
#include "main.h"

#define holdLine(line)  HAL_GPIO_WritePin(line->ioPort, line->ioPin, GPIO_PIN_RESET)
#define releaseLine(line)  HAL_GPIO_WritePin(line->ioPort, line->ioPin, GPIO_PIN_SET)
#define getLine(line)  HAL_GPIO_ReadPin(line->ioPort, line->ioPin)

static dsResult_t startLine (lineOptions_t *line);
static void sendBit  (lineOptions_t *line, unsigned char Bit);
static int  getBit   (lineOptions_t *line, unsigned char *Bit);
static void sendByte    (lineOptions_t *line, unsigned char val);
static int  getByte     (lineOptions_t *line);
static unsigned char DSCRC8(unsigned char *buf, unsigned char n);

void sDelay(unsigned int parrots)
{
   for (volatile int i = 0; i < parrots; i++);
}
//-----------------------------------------------------------------------------

static dsResult_t startLine (lineOptions_t *line)
{
    
}
//-----------------------------------------------------------------------------


void DSGetBit(unsigned char *Bit)
{
	unsigned char AnswerTime, i;
		
	AnswerTime = 1;
	DSSetDown();
    for(i = 0; i < 25; i++) Nop();
	//Delay10us(1);
	TMOUT_IO = 0;
	while(!TMIN_IO && AnswerTime)
    {
    	AnswerTime++;
	}

	if(AnswerTime>DSUpTime + 10)
		*Bit=0;
	else
		*Bit=1;	
//	Delay10us(50-AnswerTime*3);
	Delay10us(5);
}
//---------------------------------------------

void DSGetByte(unsigned char *Byte){
	unsigned char Bit;
	*Byte=0;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x1;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x2;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x4;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x8;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x10;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x20;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x40;
	DSGetBit(&Bit);
	if(Bit) 
		*Byte+=0x80;
}
//---------------------------------------------

void DSSendBit(unsigned char Bit)
{
  	unsigned char i;
      	if((Bit)){//если 1 задержка в 0 менее 15 мкс
			DSSetDown();
			for(i = 0; i < 25; i++) Nop(); 
			DSSetUp();
			Delay10us(4);		
		}
		else{//если 0 задержка в 0 более 15 мкс (60мкс)
			DSSetDown();
			Delay10us(4);
			DSSetUp();
			Delay10us(1);		
		}
		Delay10us(1);
}
//---------------------------------------------

void DSSendByte(unsigned char Byte){
	DSSendBit(Byte&0x1);
	DSSendBit(Byte&0x2);
	DSSendBit(Byte&0x4);
	DSSendBit(Byte&0x8);
	DSSendBit(Byte&0x10);
	DSSendBit(Byte&0x20);
	DSSendBit(Byte&0x40);
	DSSendBit(Byte&0x80);
}
//---------------------------------------------
unsigned char DSInitLine()
{
  unsigned char w1_resp;
  unsigned char Answer;
 
  Delay10us(1);
  Answer = DSSetDown();
  if (Answer != DS_ANS_OK) return Answer;
  Delay10us(35);
	
  if(TMIN_IO) return DS_ANS_FAIL;
  TMOUT_IO = 0;
  DSUpTime = 1;	
  
  while(!TMIN_IO && DSUpTime) DSUpTime++; 
    
	w1_resp = 0;
    while(TMIN_IO){
		w1_resp++;
		Delay10us(1);//
		if(w1_resp > 100) 
		{
    	    Nop();
    	    Nop();
    	    Nop();
    	    Nop();
    		return DS_ANS_NOANS;	
        }  		
	}
	Nop();
	Nop();
	Nop();
    w1_resp = 1;
    while(!TMIN_IO && w1_resp)
    {
        Delay10us(1);//
        w1_resp++;
    }    
 	Delay10us(31);//задержка перед передачей
	return DS_ANS_OK;
}
//---------------------------------------------

void ReadBuf(unsigned char *Buf, unsigned char Size)
{
	unsigned char i;
	for(i=0; i<Size; i++)
		DSGetByte(&(Buf[i]));
}
//---------------------------------------------
void WriteBuf(unsigned char *Buf, unsigned char Size)
{
	unsigned char i;
	for(i=0; i<Size; i++)
		DSSendByte(Buf[i]);
}
//---------------------------------------------



BYTE DSGetID(BYTE *val)
{
	BYTE errCode;
	errCode = DSInitLine();
	if (errCode != DS_ANS_OK) return errCode;
	DSSendByte(DS_ROM_READ);
	ReadBuf(val, 8);
	if (DSCRC8(val, 8)) return DS_ANS_NOANS;
	return DS_ANS_OK;
}
//---------------------------------------------------------------------------------------- 	

unsigned char DSCRC8(unsigned char *buf,unsigned char n)
{
    unsigned char  crc = 0x00, c, i, j;
    for (i = 0; i < n; i++)
    {
        c = buf[i];
        for(j = 0; j < 8; j++)
        {
                if ((c ^ crc) & 1) crc = ((crc ^ 0x18) >> 1) | 0x80;
                else crc >>= 1;
                c >>= 1;
        }
    }
    return crc;
}
//-------------------------------------------------------------------------------------
unsigned char DSConvertTemp(void)
{
unsigned char Answer=0x00;
Answer=DSInitLine();
DSSendByte(DS_ROM_SKIP);
DSSendByte(DS_SRC_CONVERT);
return Answer;
}
//---------------------------------------------
void SendROM(unsigned char *val)
{
DSSendByte(val[0]);
DSSendByte(val[1]);
DSSendByte(val[2]);
DSSendByte(val[3]);
DSSendByte(val[4]);
DSSendByte(val[5]);
DSSendByte(val[6]);
DSSendByte(val[7]);
}
//---------------------------------------------
void ReadScratchpad(unsigned char *Scratchpad)
{
DSGetByte(&(Scratchpad[0]));
DSGetByte(&(Scratchpad[1]));
DSGetByte(&(Scratchpad[2]));
DSGetByte(&(Scratchpad[3]));
DSGetByte(&(Scratchpad[4]));
DSGetByte(&(Scratchpad[5]));
DSGetByte(&(Scratchpad[6]));
DSGetByte(&(Scratchpad[7]));
DSGetByte(&(Scratchpad[8]));
}
//---------------------------------------------



BYTE DSGetTemperature(short *val, BYTE n)
{
    BYTE ROMid[9], Scratchpad[9], Result;
    short tmp;
    static BYTE j;
    
 /*
  // !!!!!!! ОТЛАДКА УБРАТЬ ПОТОМ!!!!!!  
    if (n == 1 || n == 5 || n == 8)
    {
         *val = n*10 + j++;
         return DS_ANS_OK;
    } 
   */ 
    if (n) XEEReadArray(ADDR_DSROM + 8ul * (n-1), ROMid, 8);
  
    memset(Scratchpad, 0xFF, 8);
    if (!memcmp((void*)ROMid, (void*)Scratchpad, 8)) return DS_ANS_EMPTY;
    
    memset(Scratchpad, 0x00, 8);
    if (!memcmp((void*)ROMid, (void*)Scratchpad, 8)) return DS_ANS_EMPTY;
    
    
        
    
    Result = DSInitLine();
    if (Result != DS_ANS_OK)
    {
      *val = 0x0000;
      return Result;
    }
    if (n)
    {
      DSSendByte(DS_ROM_MATCH);
      WriteBuf(ROMid, 8);  
    }  
    else DSSendByte(DS_ROM_SKIP); 
    DSSendByte(DS_SRC_READ);//^&*
    ReadBuf(Scratchpad, 9);
    tmp = Scratchpad[1];
    tmp = tmp << 8;
    tmp += Scratchpad[0];
    *val = tmp;//*((WORD *)Scratchpad);//^&*
    // если по указзанному ROMid вернулись все 0хFF, то значит датчик не подключен
    memset (ROMid, 0xFF, 8);//^&*
    if (!memcmp((void*)Scratchpad,(void*)ROMid, 8)) return DS_ANS_NOANS;//^&*
    
    if (DSCRC8(Scratchpad, 9))
    {
      *val = 0xFFFF;
      return DS_ANS_FAIL;
    }
    return DS_ANS_OK;
}
//----------------------------------------------------------------------------------------


/*
void DSTemperatureDisplay(BYTE result, short val, BYTE n)
{
BYTE ShowVal, i;
LCDLIGHT_IO = 1;
if (n < 8)
{
LCDText[n*3+0] = ' ';
switch(result)
{
case DS_ANS_NOANS:
LCDText[n*3+1] = 'N';
LCDText[n*3+2] = 'C';
break;
case DS_ANS_FAIL:
LCDText[n*3+1] = 'E';
LCDText[n*3+2] = 'R';
break;
case DS_ANS_SHORTCUT:
LCDText[n*3+1] = 'K';
LCDText[n*3+2] = 'Z';
break;
case DS_ANS_OK:
if (val < 0)
{
val = -val;
LCDText[n*3+0] = '-';
}
ShowVal = ((val >> 3) & 0x00001) + (val >> 4); // математическое округление с учетом дробной части
if (ShowVal > 99u) ShowVal = 99u;
LCDText[n*3+1] = ShowVal/10 + '0';
LCDText[n*3+2] = ShowVal%10 + '0';
for (i = 3*(n+1); i < 24; i++) LCDText[i] = '\0';
break;
}
}
}*/
//----------------------------------------------------------------------------------------

