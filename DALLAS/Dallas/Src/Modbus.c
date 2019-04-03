// file: "Modbus.c"

#include <string.h>
#include "Modbus.h"

bool modbusInit(modbus_t *mb, uint8_t addr, uint32_t baudRate)
{
    mb->lastResult = MB_ERR_INIT;
    if (mb == NULL || addr == 0 || addr > 247 || !IS_UART_BAUDRATE(baudRate)) {   // Modbus address of the device it is intended for (1 to 247).
        return false;
    }
    osSemaphoreWait(mb->dataRxSemHandle, 0); 
    mb->huart->Init.BaudRate = baudRate;
    if (HAL_MultiProcessor_Init(mb->huart, 0, UART_WAKEUPMETHOD_IDLELINE) != HAL_OK)  {
        _Error_Handler(__FILE__, __LINE__);
    }
    mb->addr = addr;
    mb->rxLen = 0;
    memset((void*)mb->rxBuf, 0, MODBUF_RX_BUF_SIZE);
    mb->lastResult = MB_OK;
    return true;
}
//-----------------------------------------------------------------------------

void modbusBefore_HAL_UART_IRQHandler(modbus_t *mb)
{   
    if (__HAL_UART_GET_IT_SOURCE(mb->huart, UART_IT_TC)) {
       // __HAL_UART_CLEAR_FLAG(mb->huart, UART_FLAG_TC);
        mb->lastResult = MB_TX_FINISH;
    }
    if (__HAL_UART_GET_IT_SOURCE(mb->huart, UART_IT_IDLE)) {        
        __HAL_UART_CLEAR_IDLEFLAG(mb->huart);
        mb->lastResult = MB_RX_DONE;         
    }
}
//-----------------------------------------------------------------------------

void modbusAfter_HAL_UART_IRQHandler(modbus_t *mb)
{   
    switch (mb->lastResult) {  
      case MB_RX_DONE:
        if (mb->huart->ErrorCode != HAL_UART_ERROR_NONE) {
            mb->lastResult = MB_ERR_FRAME;
        } else if (mb->huart->RxState == HAL_UART_STATE_READY){
            mb->lastResult = MB_ERR_OVERFLOW;
        } 
        mb->rxLen = mb->huart->RxXferSize - mb->huart->hdmarx->Instance->CNDTR;
        HAL_UART_Abort(mb->huart);
        if (mb->rxBuf[0] == mb->addr) {
            if (modbusCrc16(mb->rxBuf, mb->rxLen) == 0x0000) {
                mb->lastResult = MB_OK;
            } else if (mb->lastResult == MB_RX_DONE) { 
                mb->lastResult = MB_ERR_CRC;
            }
            osSemaphoreRelease(mb->dataRxSemHandle);
        } else {
            HAL_UART_Receive_DMA(mb->huart, mb->rxBuf, MODBUF_RX_BUF_SIZE);
           __HAL_UART_ENABLE_IT(mb->huart, UART_IT_IDLE);
        }
        break;
        
        case MB_TX_FINISH:
          modbusSetTxMode(false);
          HAL_HalfDuplex_EnableReceiver(mb->huart);
          HAL_UART_Receive_DMA(mb->huart, mb->rxBuf, MODBUF_RX_BUF_SIZE);
          __HAL_UART_ENABLE_IT(mb->huart, UART_IT_IDLE);
          osSemaphoreRelease(mb->dataRxSemHandle);
          break;
     }
}
//-----------------------------------------------------------------------------

void modbusReceiveStart(modbus_t *mb)
{
    mb->lastResult = MB_OK;
    osSemaphoreWait(mb->dataRxSemHandle, 0);
    memset((void*)mb->rxBuf, 0, MODBUF_RX_BUF_SIZE);
    mb->rxLen = 0;
    if (HAL_UART_Receive_DMA(mb->huart, mb->rxBuf, MODBUF_RX_BUF_SIZE) != HAL_OK) {
        _Error_Handler(__FILE__, __LINE__);
    }
    __HAL_UART_ENABLE_IT(mb->huart, UART_IT_IDLE);
}
//-----------------------------------------------------------------------------

modbusResult_t modbusDataWait(modbus_t *mb, uint32_t timeout)
{
    if (osSemaphoreWait(mb->dataRxSemHandle, timeout) != osOK) {
        return MB_TIMEOUT;
    }
    return mb->lastResult;
}
//-----------------------------------------------------------------------------

void modbusSend(modbus_t *mb, uint8_t *buf, uint32_t len)
{
    osSemaphoreWait(mb->dataRxSemHandle, 0);
    HAL_HalfDuplex_EnableTransmitter(mb->huart);
    osDelay(1);
    modbusSetTxMode(true);
    mb->lastResult = MB_OK;
    if (HAL_UART_Transmit_DMA(mb->huart, buf, len) != HAL_OK) {
        _Error_Handler(__FILE__, __LINE__);
    }
    __HAL_UART_DISABLE_IT(mb->huart, UART_IT_IDLE);
  //  __HAL_UART_ENABLE_IT(mb->huart, UART_IT_TC);
}
//-----------------------------------------------------------------------------
      
unsigned short modbusCrc16(unsigned char* data, unsigned char length)
{
    unsigned int crc=0xFFFF; 
    while (length--) { 
        crc ^= *data++; 
        for ( int i = 0; i < 8; i++) { 
            if (crc & 0x01 ){
                crc = (crc >> 1) ^ 0xA001; // LSB(b0)=1 
            } else {
                crc = crc >> 1; 
            } 
        }
    }
    return crc; 
}

/*

000001 11:29:59.576   ---------->     01 06 00 00 00 00 89 CA
000002 11:30:02.577   ---------->     01 06 00 00 00 00 89 CA
000003 11:30:07.174   ---------->     01 06 00 01 00 01 19 CA
000004 11:30:10.176   ---------->     01 06 00 01 00 01 19 CA
000005 11:30:13.176   ---------->     01 06 00 01 00 01 19 CA
000006 11:30:16.177   ---------->     01 06 00 01 00 01 19 CA
000007 11:30:20.890   ---------->     01 03 00 00 00 02 C4 0B
000008 11:30:23.892   ---------->     01 03 00 00 00 02 C4 0B
000009 11:30:26.892   ---------->     01 03 00 00 00 02 C4 0B
000010 11:30:29.893   ---------->     01 03 00 00 00 02 C4 0B
000011 11:30:32.979   ---------->     01 06 00 00 00 01 48 0A
000012 11:30:35.979   ---------->     01 06 00 00 00 01 48 0A
000013 11:30:38.981   ---------->     01 06 00 00 00 01 48 0A
000014 11:30:41.982   ---------->     01 06 00 00 00 01 48 0A
000015 11:30:47.040   ---------->     01 06 00 01 00 01 19 CA
000016 11:30:50.040   ---------->     01 06 00 01 00 01 19 CA
000017 11:30:53.041   ---------->     01 06 00 01 00 01 19 CA
000018 11:30:56.042   ---------->     01 06 00 01 00 01 19 CA
000019 11:31:00.723   ---------->     01 03 00 00 00 02 C4 0B
000020 11:31:03.723   ---------->     01 03 00 00 00 02 C4 0B
000021 11:31:06.724   ---------->     01 03 00 00 00 02 C4 0B
000022 11:31:09.725   ---------->     01 03 00 00 00 02 C4 0B
000023 11:31:12.835   ---------->     01 06 00 00 00 02 08 0B
000024 11:31:15.836   ---------->     01 06 00 00 00 02 08 0B
000025 11:31:18.837   ---------->     01 06 00 00 00 02 08 0B
000026 11:31:21.838   ---------->     01 06 00 00 00 02 08 0B
000027 11:31:26.254   ---------->     01 06 00 01 00 01 19 CA
000028 11:31:29.254   ---------->     01 06 00 01 00 01 19 CA
000029 11:31:32.255   ---------->     01 06 00 01 00 01 19 CA
000030 11:31:35.256   ---------->     01 06 00 01 00 01 19 CA
000031 11:31:41.459   ---------->     01 03 00 00 00 02 C4 0B
                                      0  1  2  3  4  5  6  7 
Добрый день

ПО работает циклично по всем подвескам
Вот алгоритм общения с платой для каждой подвески:
 
1 шаг.
Записываю в 0ой регистр номер пина, на котором весит подвеска, с которой необходимо получить температуру 
WriteModbusData(0, (ushort)leg);
Записываю в первый регистр "1". Тем самым перевожу плату из режима опроса датчиков, в режим ответа основной программе  
WriteModbusData(1, 1);
 
2 шаг.
Жду, когда плата изменит 1ый регистр на "2". Это будет означать что данные готовы для отправки в основную программу и их можно забирать
while (ResponseBufferHoldingRegs[1] != 2)
{
    ResponseBufferHoldingRegs = ModbusMasterRTU.ReadHoldingRegisters(Address, 0, 2);
    System.Threading.Thread.Sleep(200);
}
 
3 шаг.
Считываем регистры, в которых хранится температура 
Пример, для ситуации, когда меньше 25ти датчиков 
Считать по такому-то адресу, начиная со 2го регистра, столько то регистров 
ResponseBufferHoldingRegs = ModbusMasterRTU.ReadHoldingRegisters(Address, 2, Convert.ToUInt16(countSensThis));
 
4 шаг.
Записываю в 1ый регистр "0", тем самым возвращая плату в стандартный режим опроса 
WriteModbusData(1, 0);

В итоге для общения используются регистры 0 и 1
И остальные, начиная со 2го, чтобы забрать температуру с платы
 
Почему алгоритм работы именно такой, уже не помню, давно было)
Видимо тогда, моему мозгу студента с 4го курса, он казался самым оптимальным)

И странно что у первого файла, из тех что прикрепили, расширение cpp
Должно быть cs
Ну это уже так, если понадобится его компилить  


*/