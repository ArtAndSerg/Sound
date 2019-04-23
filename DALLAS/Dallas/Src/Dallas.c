
#include <string.h>
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
    
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= 1;
    DWT->CTRL |= (DWT_CTRL_EXCTRCENA_Msk | DWT_CTRL_CYCCNTENA_Msk);
    
    if (!getLine(line)) {
        releaseLine(line);
        if (whileLineStayHold(line, LINE_TIMEOUT) == LINE_TIMEOUT) {
            res = DS_ANS_SHORTCUT;
            goto error;
        }
        sDelay(1000);
    }
    __disable_interrupt();
    holdLine(line);
    if (whileLineStayUp(line, LINE_TIMEOUT) == LINE_TIMEOUT) {
        res = DS_ANS_FAIL;
        goto error;
    }
    __enable_interrupt();
    sDelay(500);
    __disable_interrupt();
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
    __enable_interrupt();
    releaseLine(line);
    sDelay(500);
    line->lastResult = res;
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
    __disable_interrupt();
    sendBit(line, val & 0x01);
	sendBit(line, val & 0x02);
	sendBit(line, val & 0x04);
	sendBit(line, val & 0x08);
	sendBit(line, val & 0x10);
	sendBit(line, val & 0x20);
	sendBit(line, val & 0x40);
	sendBit(line, val & 0x80);
    __enable_interrupt();
}
//-----------------------------------------------------------------------------

static int getByte(lineOptions_t *line)
{
	unsigned char res = 0;
    __disable_interrupt();
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
    __enable_interrupt();
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
	
    if ((res = startLine(line)) == DS_ANS_OK) {
        sendByte(line, DS_ROM_READ);    
    	readBuf(line, val, ROM_ID_SIZE);
	    if (dallasCRC8(val, ROM_ID_SIZE) != 0) {
            res = DS_ANS_CRC;
        }
    }
    line->lastResult = res;
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsConvertStart   (lineOptions_t *line)
{
    dsResult_t res;
    
    if ((res = startLine(line)) == DS_ANS_OK) {
        sendByte(line, DS_ROM_SKIP);
        sendByte(line, DS_SRC_CONVERT);
    }
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsReadScratchpad(lineOptions_t *line, uint8_t *id, uint8_t *scratchpad)
{
    dsResult_t res = DS_ANS_OK;
    
    memset(scratchpad, 0, SCRATCHPAD_SIZE);
    if ((res = startLine(line)) != DS_ANS_OK) {
        return res;
    }
    sendByte(line, DS_ROM_MATCH);
    writeBuf(line, id, ROM_ID_SIZE);
    sendByte(line, DS_SRC_READ);
    readBuf(line, scratchpad, SCRATCHPAD_SIZE);
    if (scratchpad[6] == 0xFF && scratchpad[7] == 0xFF) { // mast be 0x0C and 0x10 
        res = DS_ANS_NOANS;
    } else {
        if (dallasCRC8(scratchpad, SCRATCHPAD_SIZE) != 0) {
            res = DS_ANS_CRC;
        }
    }
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsWriteData (lineOptions_t *line, uint8_t *id, uint8_t *data)
{
    dsResult_t res;
    
    if ((res = startLine(line)) != DS_ANS_OK) {
        return res;
    }
    sendByte(line, DS_ROM_MATCH);
    writeBuf(line, id, ROM_ID_SIZE);
    sendByte(line, DS_SRC_WRITE);
    writeBuf(line, data, 3);
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsSaveToEEPROM (lineOptions_t *line, uint8_t *id)
{
    dsResult_t res;
    
    if ((res = startLine(line)) != DS_ANS_OK) {
        return res;
    }
    sendByte(line, DS_ROM_MATCH);
    writeBuf(line, id, ROM_ID_SIZE);
    sendByte(line, DS_SRC_COPY);
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsReadTemperature (lineOptions_t *line, sensorOptions_t *sensor)
{
    dsResult_t res;
    unsigned char scratchpad[SCRATCHPAD_SIZE];
    int tmp;
    
    memset(scratchpad, 0, SCRATCHPAD_SIZE);
    if ((res = dsReadScratchpad(line, sensor->id, scratchpad)) == DS_ANS_OK) {
        tmp = scratchpad[1];
        tmp = (tmp << 8) | scratchpad[0];
        tmp *= 100;
        sensor->currentTemperature = tmp / 16;
        sensor->num = scratchpad[2];
        sensor->num = (sensor->num << 8) | scratchpad[3];
    }
    line->lastResult = res;
	return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsSearch(lineOptions_t *line, unsigned char rom[ROM_ID_SIZE])
{
    dsResult_t res = DS_ANS_NOANS;
    int id_bit, cmp_id_bit, search_direction;   
    int id_bit_number = 1;
    int last_zero = 0;
    
    //memset((void*)rom, 0, ROM_ID_SIZE);
    // if the last call was not the last one
    // 1-Wire reset
    if ((res = startLine(line)) != DS_ANS_OK) {
        return res;
    }
    // issue the search command 
    sendByte(line, DS_ROM_SEARCH);
    // loop to do the search
    do {
        // read a bit and its complement
        __disable_interrupt();
        id_bit = getBit(line);
        cmp_id_bit = getBit(line);
        __enable_interrupt();
        // check for no devices on 1-wire
        if ((id_bit == 1) && (cmp_id_bit == 1)) {
            break;
        } 
        if ((id_bit == 0) && (cmp_id_bit == 0)) {
            if (id_bit_number == line->lastDiscrepancy) {
                search_direction = 1;
            } else if (id_bit_number > line->lastDiscrepancy) {
                search_direction = 0;
            } else {
                search_direction = (rom[(id_bit_number-1) / 8] & (0x01 << (id_bit_number-1) % 8)) != 0;
            }
            if (search_direction == 0) {
                last_zero = id_bit_number;
            }
        } else {
            search_direction = id_bit;
        }
        if (search_direction) {
            rom[(id_bit_number-1) / 8] |= (1 << (id_bit_number-1) % 8);
        } else {
            rom[(id_bit_number-1) / 8] &= (~(1 << (id_bit_number-1) % 8));
        }
        __disable_interrupt();
        sendBit(line, search_direction);
        __enable_interrupt();
        id_bit_number++;       
    } while(id_bit_number <= 64);  // loop until through all ROM bytes 0-7

    // if the search was successful then
    if (!((id_bit_number <= 64) || dallasCRC8(rom, ROM_ID_SIZE))) {
        // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
        line->lastDiscrepancy = last_zero;
        // check for last device
        if (line->lastDiscrepancy == 0) {
            res = DS_ANS_DISABLED;
        } else {
            res = DS_ANS_OK;
        }
    } else {
        res = DS_ANS_CRC;
    }
    line->lastResult = res;
    return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsFindAllId (lineOptions_t *line)
{
    unsigned char rom[ROM_ID_SIZE];
    dsResult_t res = DS_ANS_OK;
    int count = 0;
    
    line->lastDiscrepancy = 0;
    
    memset(rom, 0, ROM_ID_SIZE);
    for (int i = 0; i < SENSORS_PER_LINE_MAXCOUNT && res == DS_ANS_OK; i++) {
        res = dsSearch(line, rom);
        if (res == DS_ANS_OK || res == DS_ANS_DISABLED) {
            memcpy((void*)line->sensor[i].id, (void*)rom, ROM_ID_SIZE);
            count++;
        } 
        if (res == DS_ANS_DISABLED) {
            res = DS_ANS_OK;
            break;
        }
    }
    line->sensorsCount = count;
    line->lastResult = res;
    return res;
}
//-----------------------------------------------------------------------------

dsResult_t dsWriteNum (lineOptions_t *line, uint8_t *id, uint16_t num)
{
    uint8_t data[3] = {num >> 8, num & 0xFF, DS_CONF_12_BITS};
    dsResult_t res = DS_ANS_OK;    
    unsigned char scratchpad[SCRATCHPAD_SIZE];
        
    if ((res = dsWriteData(line, id, data)) == DS_ANS_OK) {
        if ((res = dsReadScratchpad (line, id, scratchpad)) == DS_ANS_OK) {
            if (scratchpad[2] != (num >> 8) || scratchpad[3] != (num &0xFF) || scratchpad[4] != DS_CONF_12_BITS) {
                res = DS_ANS_BAD_SENSOR; 
            } else {
                res = dsSaveToEEPROM(line, id);
                sDelay(1000);
                res = dsSaveToEEPROM(line, id);
                sDelay(1000);
                res = dsSaveToEEPROM(line, id);
            }
        }
    }
    line->lastResult = res;
	return res;
}
//-----------------------------------------------------------------------------

/*
//-----------------------------------------------------------------------------
// Данная функция осуществляет сканирование сети 1-wire и записывает найденные
//   ID устройств в массив buf, по 8 байт на каждое устройство.
// переменная num ограничивает количество находимых устройств, чтобы не переполнить
// буфер.
//-----------------------------------------------------------------------------
uint8_t OW_Scan(uint8_t *buf, uint8_t num) {

        uint8_t found = 0;
        uint8_t *lastDevice;
        uint8_t *curDevice = buf;
        uint8_t numBit, lastCollision, currentCollision, currentSelection;

        lastCollision = 0;
        while (found < num) {
                numBit = 1;
                currentCollision = 0;

                // посылаем команду на поиск устройств
                OW_Send(OW_SEND_RESET, (uint8_t*)"\xf0", 1, 0, 0, OW_NO_READ);

                for (numBit = 1; numBit <= 64; numBit++) {
                        // читаем два бита. Основной и комплементарный
                        OW_toBits(OW_READ_SLOT, ow_buf);
                        OW_SendBits(2);

                        if (ow_buf[0] == OW_R_1) {
                                if (ow_buf[1] == OW_R_1) {
                                        // две единицы, где-то провтыкали и заканчиваем поиск
                                        return found;
                                } else {
                                        // 10 - на данном этапе только 1
                                        currentSelection = 1;
                                }
                        } else {
                                if (ow_buf[1] == OW_R_1) {
                                        // 01 - на данном этапе только 0
                                        currentSelection = 0;
                                } else {
                                        // 00 - коллизия
                                        if (numBit < lastCollision) {
                                                // идем по дереву, не дошли до развилки
                                                if (lastDevice[(numBit - 1) >> 3]
                                                                & 1 << ((numBit - 1) & 0x07)) {
                                                        // (numBit-1)>>3 - номер байта
                                                        // (numBit-1)&0x07 - номер бита в байте
                                                        currentSelection = 1;

                                                        // если пошли по правой ветке, запоминаем номер бита
                                                        if (currentCollision < numBit) {
                                                                currentCollision = numBit;
                                                        }
                                                } else {
                                                        currentSelection = 0;
                                                }
                                        } else {
                                                if (numBit == lastCollision) {
                                                        currentSelection = 0;
                                                } else {
                                                        // идем по правой ветке
                                                        currentSelection = 1;

                                                        // если пошли по правой ветке, запоминаем номер бита
                                                        if (currentCollision < numBit) {
                                                                currentCollision = numBit;
                                                        }
                                                }
                                        }
                                }
                        }

                        if (currentSelection == 1) {
                                curDevice[(numBit - 1) >> 3] |= 1 << ((numBit - 1) & 0x07);
                                OW_toBits(0x01, ow_buf);
                        } else {
                                curDevice[(numBit - 1) >> 3] &= ~(1 << ((numBit - 1) & 0x07));
                                OW_toBits(0x00, ow_buf);
                        }
                        OW_SendBits(1);
                }
                found++;
                lastDevice = curDevice;
                curDevice += 8;
                if (currentCollision == 0)
                        return found;

                lastCollision = currentCollision;
        }

        return found;
}

*/


/*
// Переменные для хранения промежуточного результата поиска
uint8_t onewire_enum[8]; // найденный восьмибайтовый адрес 
uint8_t onewire_enum_fork_bit; // последний нулевой бит, где была неоднозначность (нумеруя с единицы)

// Инициализирует процедуру поиска адресов устройств
void onewire_enum_init() {
  for (uint8_t p = 0; p < 8; p++) {
    onewire_enum[p] = 0;
  }      
  onewire_enum_fork_bit = 65; // правее правого
}

// Перечисляет устройства на шине 1-wire и получает очередной адрес.
// Возвращает указатель на буфер, содержащий восьмибайтовое значение адреса, либо NULL, если поиск завешён
uint8_t * onewire_enum_next() {
  if (!onewire_enum_fork_bit) { // Если на предыдущем шаге уже не было разногласий
    return 0; // то просто выходим ничего не возвращая
  }
  if (!onewire_reset()) {
    return 0;
  }  
  uint8_t bp = 8;
  uint8_t * pprev = &onewire_enum[0];
  uint8_t prev = *pprev;
  uint8_t next = 0;
  
  uint8_t p = 1;
  onewire_send(0xF0);
  uint8_t newfork = 0;
  for(;;) {
    uint8_t not0 = onewire_read_bit();
    uint8_t not1 = onewire_read_bit();
    if (!not0) { // Если присутствует в адресах бит ноль
      if (!not1) { // Но также присустствует бит 1 (вилка)
        if (p < onewire_enum_fork_bit) { // Если мы левее прошлого правого конфликтного бита, 
          if (prev & 1) {
            next |= 0x80; // то копируем значение бита из прошлого прохода
          } else {
            newfork = p; // если ноль, то запомним конфликтное место
          }          
        } else if (p == onewire_enum_fork_bit) {
          next |= 0x80; // если на этом месте в прошлый раз был правый конфликт с нулём, выведем 1
        } else {
          newfork = p; // правее - передаём ноль и запоминаем конфликтное место
        }        
      } // в противном случае идём, выбирая ноль в адресе
    } else {
      if (!not1) { // Присутствует единица
        next |= 0x80;
      } else { // Нет ни нулей ни единиц - ошибочная ситуация
        return 0;
      }
    }
    onewire_send_bit(next & 0x80);
    bp--;
    if (!bp) {
      *pprev = next;
      if (p >= 64)
        break;
      next = 0;
      pprev++;
      prev = *pprev;
      bp = 8;
    } else {
      if (p >= 64)
        break;
      prev >>= 1;
      next >>= 1;
    }
    p++;
  }
  onewire_enum_fork_bit = newfork;
  return &onewire_enum[0];
}

// Выполняет инициализацию (сброс) и, если импульс присутствия получен,
// выполняет MATCH_ROM для последнего найденного методом onewire_enum_next адреса
// Возвращает 0, если импульс присутствия не был получен, 1 - в ином случае
uint8_t onewire_match_last() {
  return onewire_match(&onewire_enum[0]);
}
*/

/*
// считаем количество устройств от 0 до 7 (максимум 8) 
for( r=0;r<8;r++) 
         { 
    reset_(); 
   wr_b(0xf0); 
   find_tst(r,sn); 
///////////////////bla-bla-bla-------------------------------------

//------------------------------- 
void find_tst(unsigned char t,unsigned char *sn) 
// каждый вызов этой функции записывает серийный номер в массив sn 
{  // t -это маска, которая будет использоваться при совпадениях 
unsigned char nbt; // 8 байт серийного номера 
unsigned char nbit;// 8 бит в байте 
static bit inf0,inf1,inf;  
//----- 
for (nbt=0;nbt<8;nbt++) 
   { 
    for (nbit=0;nbit<8;nbit++) 
            { 
    sn[nbt]=sn[nbt]>>1;// сдвигаем серийный байт на 1 бит вправо,освобождаем место 
   inf0=read_bit();   // читаем значение - если нет совпадений,   
   inf1=read_bit();   //  то прочитается 10 или 01 
         if (inf0 != inf1) 
               { 
               if (inf0 == 1) 
                  { 
                    sn[nbt]=sn[nbt]+0x80;// ставим 1 в старшем бите, 0 -получился  
                    }                    // при сдвиге 
              write_bit(inf0); // выдаем то значение, которое 
                  }            // пришло первым 
            else //------------------- есть совпадение в серийниках 
                   { 
                    //--------------- 
 
                inf=t&0x01;     // читаем то значение бита байта, который мы задавали при вызове 
               t= t  >> 1;     // готовим следующий бит 
                    //---------------  
 
                  write_bit(inf); // выдаем его 
                  if(inf == 1) 
                  { 
                  sn[nbt]=sn[nbt]+0x80;// если он 1, аналогично пишем в серийник 
                       }    
                   
               }//-----------------------        
              } 
   } 
} 
//-------------------------------------------------------------------------------------------
*/




/*
/ TMEX API TEST BUILD DECLARATIONS
#define TMEXUTIL
#include "ibtmexcw.h"
long session_handle;
// END TMEX API TEST BUILD DECLARATIONS

// definitions
#define FALSE 0
#define TRUE  1

// method declarations
int  OWFirst();
int  OWNext();
int  OWVerify();
void OWTargetSetup(unsigned char family_code);
void OWFamilySkipSetup();
int  OWReset();
void OWWriteByte(unsigned char byte_value);
void OWWriteBit(unsigned char bit_value);
unsigned char OWReadBit();
int  OWSearch();
unsigned char docrc8(unsigned char value);

// global search state
unsigned char ROM_NO[8];
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
unsigned char crc8;

//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
int OWFirst()
{
   // reset the search state
   LastDiscrepancy = 0;
   LastDeviceFlag = FALSE;
   LastFamilyDiscrepancy = 0;

   return OWSearch();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWNext()
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWSearch()
{
   int id_bit_number;
   int last_zero, rom_byte_number, search_result;
   int id_bit, cmp_id_bit;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (!OWReset())
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command 
      OWWriteByte(0xF0);  

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = OWReadBit();
         cmp_id_bit = OWReadBit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            OWWriteBit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = TRUE;
         
         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }

   return search_result;
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
//
int OWVerify()
{
   unsigned char rom_backup[8];
   int i,rslt,ld_backup,ldf_backup,lfd_backup;

   // keep a backup copy of the current state
   for (i = 0; i < 8; i++)
      rom_backup[i] = ROM_NO[i];
   ld_backup = LastDiscrepancy;
   ldf_backup = LastDeviceFlag;
   lfd_backup = LastFamilyDiscrepancy;

   // set search to find the same device
   LastDiscrepancy = 64;
   LastDeviceFlag = FALSE;

   if (OWSearch())
   {
      // check if same device found
      rslt = TRUE;
      for (i = 0; i < 8; i++)
      {
         if (rom_backup[i] != ROM_NO[i])
         {
            rslt = FALSE;
            break;
         }
      }
   }
   else
     rslt = FALSE;

   // restore the search state 
   for (i = 0; i < 8; i++)
      ROM_NO[i] = rom_backup[i];
   LastDiscrepancy = ld_backup;
   LastDeviceFlag = ldf_backup;
   LastFamilyDiscrepancy = lfd_backup;

   // return the result of the verify
   return rslt;
}

//--------------------------------------------------------------------------
// Setup the search to find the device type 'family_code' on the next call
// to OWNext() if it is present.
//
void OWTargetSetup(unsigned char family_code)
{
   int i;

   // set the search state to find SearchFamily type devices
   ROM_NO[0] = family_code;
   for (i = 1; i < 8; i++)
      ROM_NO[i] = 0;
   LastDiscrepancy = 64;
   LastFamilyDiscrepancy = 0;
   LastDeviceFlag = FALSE;
}

//--------------------------------------------------------------------------
// Setup the search to skip the current device type on the next call
// to OWNext().
//
void OWFamilySkipSetup()
{
   // set the Last discrepancy to last family discrepancy
   LastDiscrepancy = LastFamilyDiscrepancy;
   LastFamilyDiscrepancy = 0;

   // check for end of list
   if (LastDiscrepancy == 0)
      LastDeviceFlag = TRUE;
}

//--------------------------------------------------------------------------
// 1-Wire Functions to be implemented for a particular platform
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Reset the 1-Wire bus and return the presence of any device
// Return TRUE  : device present
//        FALSE : no device present
//
int OWReset()
{
   // platform specific
   // TMEX API TEST BUILD
   return (TMTouchReset(session_handle) == 1);
}

//--------------------------------------------------------------------------
// Send 8 bits of data to the 1-Wire bus
//
void OWWriteByte(unsigned char byte_value)
{
   // platform specific
   
   // TMEX API TEST BUILD
   TMTouchByte(session_handle,byte_value);
}

//--------------------------------------------------------------------------
// Send 1 bit of data to teh 1-Wire bus
//
void OWWriteBit(unsigned char bit_value)
{
   // platform specific

   // TMEX API TEST BUILD
   TMTouchBit(session_handle,(short)bit_value);
}

//--------------------------------------------------------------------------
// Read 1 bit of data from the 1-Wire bus 
// Return 1 : bit read is 1
//        0 : bit read is 0
//
unsigned char OWReadBit()
{
   // platform specific

   // TMEX API TEST BUILD
   return (unsigned char)TMTouchBit(session_handle,0x01);

}

// TEST BUILD
static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   
   // TEST BUILD
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}

//--------------------------------------------------------------------------
// TEST BUILD MAIN
//
int main(short argc, char **argv)
{
   short PortType=5,PortNum=1;
   int rslt,i,cnt;

   // TMEX API SETUP
   // get a session
   session_handle = TMExtendedStartSession(PortNum,PortType,NULL);
   if (session_handle <= 0)
   {
      printf("No session, %d\n",session_handle);
      exit(0);
   }

   // setup the port
   rslt = TMSetup(session_handle);
   if (rslt != 1)
   {
      printf("Fail setup, %d\n",rslt);
      exit(0);
   }
   // END TMEX API SETUP

   // find ALL devices
   printf("\nFIND ALL\n");
   cnt = 0;
   rslt = OWFirst();
   while (rslt)
   {
      // print device found
      for (i = 7; i >= 0; i--)
         printf("%02X", ROM_NO[i]);
      printf("  %d\n",++cnt);

      rslt = OWNext();
   }

   // find only 0x1A
   printf("\nFIND ONLY 0x1A\n");
   cnt = 0;
   OWTargetSetup(0x1A);
   while (OWNext())
   {
      // check for incorrect type
      if (ROM_NO[0] != 0x1A)
         break;
      
      // print device found
      for (i = 7; i >= 0; i--)
         printf("%02X", ROM_NO[i]);
      printf("  %d\n",++cnt);
   }

   // find all but 0x04, 0x1A, 0x23, and 0x01
   printf("\nFIND ALL EXCEPT 0x10, 0x04, 0x0A, 0x1A, 0x23, 0x01\n");
   cnt = 0;
   rslt = OWFirst();
   while (rslt)
   {
      // check for incorrect type
      if ((ROM_NO[0] == 0x04) || (ROM_NO[0] == 0x1A) || 
          (ROM_NO[0] == 0x01) || (ROM_NO[0] == 0x23) ||
          (ROM_NO[0] == 0x0A) || (ROM_NO[0] == 0x10))
          OWFamilySkipSetup();
      else
      {
         // print device found
         for (i = 7; i >= 0; i--)
            printf("%02X", ROM_NO[i]);
         printf("  %d\n",++cnt);
      }

      rslt = OWNext();
   }

   // TMEX API CLEANUP
   // release the session
   TMEndSession(session_handle);
   // END TMEX API CLEANUP
}

*/
