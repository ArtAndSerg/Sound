
#include "Dallas.h"
#include "main.h"
#include "cmsis_os.h"

#define TICK            (DWT->CYCCNT)
#define TICK_us         (SystemCoreClock / 1000000)
#define holdLine(a)     (HAL_GPIO_WritePin(a->ioPort, a->ioPin, GPIO_PIN_RESET))
#define releaseLine(a)  (HAL_GPIO_WritePin(a->ioPort, a->ioPin, GPIO_PIN_SET))
#define getLine(a)      (a->ioPort->IDR & a->ioPin)

//(HAL_GPIO_ReadPin(a->ioPort, a->ioPin) == GPIO_PIN_SET)

static dsResult_t startLine (lineOptions_t *line);
static unsigned int whileLineStayUp   (lineOptions_t *line, unsigned int timeout);
static unsigned int whileLineStayHold (lineOptions_t *line, unsigned int timeout);

static void sendBit  (lineOptions_t *line, int Bit);
static int  getBit   (lineOptions_t *line);
static void sendByte    (lineOptions_t *line, int val);
static int  getByte     (lineOptions_t *line);
static void readBuf(lineOptions_t *line, unsigned char *buf, int size);
static void writeBuf(lineOptions_t *line, unsigned char *buf, int size);
static unsigned char dallasCRC8(unsigned char *buf, int n);
/*
static const unsigned char crcTable[] = {
0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147,205,
17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};
*/
static __INLINE void sDelay(unsigned int us) 
{
   DWT->CYCCNT = 0;
   volatile unsigned int timeout = us * TICK_us;  
   while(DWT->CYCCNT < timeout);
}
//-----------------------------------------------------------------------------

static unsigned int whileLineStayUp   (lineOptions_t *line, unsigned int timeout)
{   
    TICK = 0;
    timeout *= TICK_us;
    while (getLine(line) && TICK < timeout);
    return TICK / TICK_us;
}
//-----------------------------------------------------------------------------

static unsigned int whileLineStayHold (lineOptions_t *line, unsigned int timeout)
{
    TICK = 0;
    timeout *= TICK_us;
    while (!getLine(line) && TICK < timeout);
    return TICK / TICK_us;
}
//-----------------------------------------------------------------------------

static dsResult_t startLine (lineOptions_t *line)
{
    dsResult_t res = DS_ANS_OK; 
    
    DWT->CTRL |= (DWT_CTRL_EXCTRCENA_Msk | DWT_CTRL_CYCCNTENA_Msk);
       
    if (!getLine(line)) {
        releaseLine(line);
        if (whileLineStayHold(line, LINE_TIMEOUT) == LINE_TIMEOUT) {
            res = DS_ANS_SHORTCUT;
            goto error;
        }
        HAL_Delay(1);
    }
    holdLine(line);
    if (whileLineStayUp(line, LINE_TIMEOUT) == LINE_TIMEOUT) {
        res = DS_ANS_FAIL;
        goto error;
    }
    sDelay(500);
    releaseLine(line);
    line->setUpTime = whileLineStayHold(line, LINE_TIMEOUT);
    if (line->setUpTime == LINE_TIMEOUT){
        res = DS_ANS_SHORTCUT;
        goto error;
    }
    if (whileLineStayUp(line, 60) == 60) {
        res = DS_ANS_NOANS;
        goto error;
    }
    if (whileLineStayHold(line, 240 + line->setUpTime) == 240 + line->setUpTime) {
        res = DS_ANS_SHORTCUT;
        goto error;
    }
    
  error:
    releaseLine(line);
    sDelay(500);
    return res;
}
//-----------------------------------------------------------------------------

static void sendBit (lineOptions_t *line, int val)
{
    holdLine(line);
    whileLineStayUp(line, LINE_TIMEOUT);
    if (!val) {
        sDelay(60);
        releaseLine(line);
    } else {
        sDelay(1);
        releaseLine(line);
        sDelay(60);
    }
    whileLineStayHold(line, LINE_TIMEOUT);
    sDelay(1);
}
//-----------------------------------------------------------------------------

static int getBit (lineOptions_t *line)
{
    int t;
    
    holdLine(line);
    whileLineStayUp(line, LINE_TIMEOUT);
    sDelay(1);
    releaseLine(line);
    t = whileLineStayHold(line, 60 + line->setUpTime);
    if (t > 15 + line->setUpTime) {
        sDelay(61 + line->setUpTime - t);
        return 0;
    } else {
        sDelay(61 + line->setUpTime - t);
        return 1;
    }
}
//-----------------------------------------------------------------------------

static void sendByte (lineOptions_t *line, int val)
{
    sendBit(line, val & 0x01);
	sendBit(line, val & 0x02);
	sendBit(line, val & 0x04);
	sendBit(line, val & 0x08);
	sendBit(line, val & 0x10);
	sendBit(line, val & 0x20);
	sendBit(line, val & 0x40);
	sendBit(line, val & 0x80);
}
//-----------------------------------------------------------------------------

static int getByte(lineOptions_t *line)
{
	unsigned char res = 0;
	
	if(getBit(line)) {
		res |= 0x01;
    }
	if(getBit(line)) {
		res |= 0x02;
    }
	if(getBit(line)) {
		res |= 0x04;
    }
	if(getBit(line)) {
		res |= 0x08;
    }
	if(getBit(line)) {
		res |= 0x10;
    }
	if(getBit(line)) {
		res |= 0x20;
    }
	if(getBit(line)) {
		res |= 0x40;
    }
	if(getBit(line)) {
		res |= 0x80;
    }
	return res;
}
//-----------------------------------------------------------------------------

static unsigned char dallasCRC8(unsigned char *buf, int n)
{
    unsigned char  crc = 0x00, tmp;
    for (int i = 0; i < n; i++)
    {
        tmp = buf[i];
        for(int j = 0; j < 8; j++)
        {
            if ((tmp ^ crc) & 1) {
                crc = ((crc ^ 0x18) >> 1) | 0x80;
            } else {
                crc >>= 1;
            }
            tmp >>= 1;
        }
    }
/*    
    unsigned char crc = 0;
    for (int i = 0; i < n; i++) {
        crc = crcTable[crc ^ buf[i]];
    }
*/
    return crc;
}
//-----------------------------------------------------------------------------

static void readBuf(lineOptions_t *line, unsigned char *buf, int size)
{
    for(int i=0; i < size; i++) {
		buf[i] = getByte(line);
    }
}
//-----------------------------------------------------------------------------

static void writeBuf(lineOptions_t *line, unsigned char *buf, int size)
{	     
    for(int i=0; i < size; i++) {
		sendByte(line, buf[i]);
    }
}
//---------------------------------------------

dsResult_t dsGetID (lineOptions_t *line, unsigned char *val)
{
    dsResult_t res;
	
    taskENTER_CRITICAL();
    if ((res = startLine(line)) == DS_ANS_OK) {
        sendByte(line, DS_ROM_READ);    
    }
	readBuf(line, val, 8);
	if (dallasCRC8(val, 8) != 0) {
        res = DS_ANS_CRC;
    }
    taskEXIT_CRITICAL();     
	return res;
}
//-----------------------------------------------------------------------------

/*
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


dsResult_t DSGetID (lineOptions_t *line, unsigned char *val)
{
    dsResult_t res;
    
    res = startLine(line);
    return res;
}
//---------------------------------------------


/*

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

/*

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

