using System;
using System.IO.Ports;
using Modbus.Device;
using System.Data;

namespace VIKmonitoring
{
    public class Modbus_RTU
    {
        
        ThermometryDB thDB = new ThermometryDB();
        MonitorSilos msForm;
        AllSilos asForm;
        public SerialPort port;
        ModbusSerialMaster ModbusMasterRTU;
        Byte Address;   // Адрес удаленного устройства
        ushort startRegister;   // Первый считываемый регистр
        ushort numRegisters;    // Количество считываемых регистров
        int iDataType;          // Считываемая область памяти
        int requestInterval = 1000; //Интервал опроса 
        public Modbus_RTU(MonitorSilos msform, AllSilos asform)
        {
            msForm = msform;
            asForm = asform;
        }
        /// <summary>
        /// Соединяет с заданным Com портом
        /// </summary>
        /// <param name="r_ComPort">Порт</param>
        /// <param name="r_BaudRate">Скорость</param>
        /// <param name="r_parity">Четность</param>
        /// <param name="r_DataBits">Число бит (7,8)</param>
        /// <param name="r_StopBits">Число стоп битов</param>
        /// <param name="r_Timeout">Время до таймаута</param>
        public void ConnectWithPort_COM(string r_ComPort, int r_BaudRate, Parity r_parity, int r_DataBits,
                    StopBits r_StopBits, int r_Timeout)
        {
            port = new SerialPort();
            port.BaudRate = r_BaudRate;
            port.PortName = r_ComPort;
            port.Parity = r_parity;
            port.DataBits = r_DataBits;
            port.StopBits = r_StopBits;
            port.ReadTimeout = r_Timeout;
            port.WriteTimeout = r_Timeout;
            try
            {
                port.Open();
            }
            catch (System.IO.IOException)
            {
                asForm.Message("Порт не существует");
                port.Close();
            }
            catch (System.InvalidOperationException)
            {
                asForm.Message("Порт уже используется");
                port.Close();
            }
            catch (System.ArgumentException)
            {
                asForm.Message("Неверное имя порта");
                port.Close();
            }
            if (port.IsOpen)
                asForm.Message("Порт успешно открыт");

        }
        public void DisconnectFromPort()
        {
            port.Close();
                if (!port.IsOpen)
                    asForm.Message("Порт успешно закрыт");
                else
                    asForm.Message("Не удалось закрыть");

        }
        public void Request(int r_DataType, Byte r_Address, ushort r_StartRegister, ushort r_NumRegisters, int r_RequestInterval)
        {
            Address = r_Address;
            startRegister = r_StartRegister;
            numRegisters = r_NumRegisters;
            ModbusMasterRTU = ModbusSerialMaster.CreateRtu(port);
            iDataType = r_DataType;
            requestInterval = r_RequestInterval;
        }
        public void WriteModbusData(ushort r_registerAddress, ushort r_Value)
        {
            try
            {
                ModbusMasterRTU.WriteSingleRegister(Address, r_registerAddress, r_Value);
            }
            catch (Exception ex)
            {
                //System.Windows.Forms.MessageBox.Show("Error WriteModbusData:" + ex.Message);
            }
        }
        public void UpdateModbusData()
        {
            ProgramSettings ps = new ProgramSettings(1);
            int writeTime = thDB.GetTimeWriteSetting(1);
            bool neVTomSilose = false;
            int fire = 0;
            bool alarm = false;
            bool notFound = false;
            float sumTemp = 0, maxTemp = -100, minTemp = 1000;
            ulong countTemp = 0;
            float yellow = 0, red = 0, Temp=0;
            string numberSilos = "";
            try
            {
                try
                {
                    int maxfire = 0, minfire = 16;
                    while (!asForm.bThreadStop)
                    {

                        ps.autoBackUp();
                        DataTable dtSil = thDB.ShowSilos();
                        for (int s = 0; s < dtSil.Rows.Count; s++)
                        {
                            alarm = false;
                        linkAnotherSilos:
                            sumTemp = 0; maxTemp = -100; minTemp = 1000;
                            countTemp = 0;
                            bool inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                            if (inMonitor) numberSilos = asForm.numberMonitor().ToString();
                            else { numberSilos = (dtSil.Rows[s][1].ToString()).Trim(); }
                            maxfire = 0; fire = 0; minfire = 16;
                            if (inMonitor)
                            {
                                DataTable dtColor = thDB.GetColorInSilos(numberSilos);
                                red = Convert.ToSingle(dtColor.Rows[0][1]);
                                yellow = Convert.ToSingle(dtColor.Rows[0][0]);
                            }
                            else
                            {
                                red = Convert.ToSingle(dtSil.Rows[s][8]);
                                yellow = Convert.ToSingle(dtSil.Rows[s][7]);
                            }
                            DataTable dtDev = thDB.GetListDevInSilos(numberSilos);
                            for (int d = 0; d < dtDev.Rows.Count; d++)
                            {
                                if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                                byte numberDevice = Convert.ToByte(dtDev.Rows[d][0]);
                                byte vendDev = thDB.GetTypeCustomerDevice(numberDevice);
                                Address = numberDevice;
                                DataTable dtWire = thDB.GetListWireInDeviceInCurSilos(numberDevice, thDB.GetIdSil(numberSilos));
                                int maxcountsensinsilosNOW = 0;
                                for (int w = 0; w < dtWire.Rows.Count; w++)
                                {
                                    int numberWireLocal = Convert.ToInt32(dtWire.Rows[w][0]);
                                    int countSensThisLoc = thDB.GetCountSensInWire(numberWireLocal);
                                    if (countSensThisLoc > maxcountsensinsilosNOW) maxcountsensinsilosNOW = countSensThisLoc;
                                }
                                for (int w = 0; w < dtWire.Rows.Count; w++)
                                {
                                    int countAlarm = 0;
                                    int fireOnWire = 0;
                                    if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                    if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                    inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                                    int numberWire = Convert.ToInt32(dtWire.Rows[w][0]);
                                    byte vendWire = thDB.GetTypeCustomerWire(numberWire);
                                    if (thDB.GetWireVisible(numberWire))
                                    {
                                        #region vend0
                                        if (vendDev == 0)
                                        {
                                            if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                            if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                            inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                                            int leg = thDB.GetPinWire(numberWire);
                                            int countSensThis = thDB.GetCountSensInWire(numberWire);
                                            #region sensorRead
                                            try
                                            {
                                                string[] ResponseBuf = new string[countSensThis];
                                                ushort[] ResponseBufferHoldingRegs = new ushort[countSensThis];

                                                
                                                WriteModbusData(0, (ushort)leg);
                                                WriteModbusData(1, 1);

                                                System.Threading.Thread.Sleep(500);

                                                while (ResponseBufferHoldingRegs[1] != 2)
                                                {
                                                    ResponseBufferHoldingRegs = ModbusMasterRTU.ReadHoldingRegisters(Address, 0, 2);
                                                    System.Threading.Thread.Sleep(200);
                                                }

                                                if (countSensThis>50)
                                                {
                                                    ushort[] ResponseBufferHoldingRegs1 = new ushort[25];
                                                    ushort[] ResponseBufferHoldingRegs2 = new ushort[25];
                                                    ushort[] ResponseBufferHoldingRegs3 = new ushort[countSensThis - 50];
                                                    ResponseBufferHoldingRegs1 = ModbusMasterRTU.ReadHoldingRegisters(Address, 2, Convert.ToUInt16(25));
                                                    ResponseBufferHoldingRegs2 = ModbusMasterRTU.ReadHoldingRegisters(Address, 27, Convert.ToUInt16(25));
                                                    ResponseBufferHoldingRegs3 = ModbusMasterRTU.ReadHoldingRegisters(Address, 52, Convert.ToUInt16(countSensThis - 50));
                                                    ResponseBufferHoldingRegs = new ushort[countSensThis];
                                                    for (int h = 0; h < 25; h++)
                                                    {
                                                        ResponseBufferHoldingRegs[h] = ResponseBufferHoldingRegs1[h];
                                                    }
                                                    for (int h = 25; h < 50; h++)
                                                    {
                                                        ResponseBufferHoldingRegs[h] = ResponseBufferHoldingRegs2[h-25];
                                                    }
                                                    for (int h = 50; h < countSensThis; h++)
                                                    {
                                                        ResponseBufferHoldingRegs[h] = ResponseBufferHoldingRegs2[h - 50];
                                                    }
                                                }
                                                else if (countSensThis > 25 && countSensThis<51)
                                                {
                                                    ushort[] ResponseBufferHoldingRegs1 = new ushort[25];
                                                    ushort[] ResponseBufferHoldingRegs2 = new ushort[countSensThis-25];
                                                    ResponseBufferHoldingRegs1 = ModbusMasterRTU.ReadHoldingRegisters(Address, 2, Convert.ToUInt16(25));
                                                    ResponseBufferHoldingRegs2 = ModbusMasterRTU.ReadHoldingRegisters(Address, 27, Convert.ToUInt16(countSensThis-25));
                                                    ResponseBufferHoldingRegs = new ushort[countSensThis];
                                                    for (int h=0;h<25;h++)
                                                    {
                                                        ResponseBufferHoldingRegs[h] = ResponseBufferHoldingRegs1[h];
                                                    }
                                                    for (int h = 25; h < countSensThis; h++)
                                                    {
                                                        ResponseBufferHoldingRegs[h] = ResponseBufferHoldingRegs2[h-25];
                                                    }
                                                }
                                                else
                                                {
                                                    ResponseBufferHoldingRegs = ModbusMasterRTU.ReadHoldingRegisters(Address, 2, Convert.ToUInt16(countSensThis));

                                                }

                                                WriteModbusData(1, 0);

                                                if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                                if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                                inMonitor = (asForm.numberMonitor() != "-1") ? true : false;

                                                for (int i = 0; i < countSensThis; i++)
                                                {
                                                    ResponseBuf[i] = ResponseBufferHoldingRegs[i].ToString();
                                                }
                                                for (int i = 0; i < countSensThis; i++)
                                                {

                                                    if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                                    if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                                    inMonitor = (asForm.numberMonitor() != "-1") ? true : false;

                                                    int tempINTMult10 = int.Parse(ResponseBuf[i])>59000? int.Parse(ResponseBuf[i]) : int.Parse(ResponseBuf[i]) / 10;
                                                    string adrSensCompl = numberDevice.ToString() + ' ' + numberWire.ToString() + ' ' + (i + 1).ToString();

                                                    if (tempINTMult10 == 3276 || tempINTMult10 == 999 || tempINTMult10 == 85)
                                                    {
                                                        notFound = true;
                                                    }
                                                    else
                                                    {
                                                        if (tempINTMult10 > (red * 10)) fire = 4;
                                                        else if (tempINTMult10 > (yellow * 10)) fire = 3;
                                                        else if (tempINTMult10 > 0) fire = 2;
                                                    }
                                                    
                                                    if (tempINTMult10 > 59000)
                                                    {
                                                        tempINTMult10 = ((65535 - tempINTMult10) * (-1))/10;
                                                        if (tempINTMult10 < 0) fire = 1;
                                                    }
                                                    if (fire > fireOnWire) fireOnWire = fire;
                                                    if (fire > maxfire && fire > 2) maxfire = fire;
                                                    else if (fire < minfire) minfire = fire;
                                                    if (!notFound)
                                                    {
                                                        Temp = tempINTMult10 / (float)10;
                                                        if (Temp < minTemp) minTemp = Temp;
                                                        if (Temp > maxTemp) maxTemp = Temp;
                                                        sumTemp += Temp;
                                                        countTemp++;
                                                        if (inMonitor)
                                                        {
                                                            asForm.changeCell(numberWire, i, tempINTMult10, false, maxcountsensinsilosNOW - countSensThis, fire);    
                                                        }
                                                        if ((thDB.LastTime(thDB.GetIdSens(adrSensCompl)) < DateTime.Now.AddMinutes(-writeTime)))
                                                            asForm.UpdateTempCKomplect(tempINTMult10, i, adrSensCompl);
                                                        else if ((fire > 3) && (thDB.LastTime(thDB.GetIdSens(adrSensCompl)) < DateTime.Now.AddMinutes(-(writeTime/4))))
                                                        {
                                                            asForm.UpdateTempCKomplect(tempINTMult10, i, adrSensCompl);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        countAlarm++;
                                                        alarm = true;
                                                        asForm.changeAlarmLabel(numberSilos);
                                                        asForm.changeCell(numberWire, i, -1000, false, maxcountsensinsilosNOW - countSensThis,fire);
                                                    }
                                                    if (notFound)
                                                    {
                                                        notFound = false;
                                                       
                                                    }
                                                }
                                            }
                                            catch (TimeoutException)
                                            {
                                                asForm.Message("Timeout");
                                            }
                                            catch (Modbus.SlaveException e)
                                            {
                                                asForm.Message("Клиент не может прислать запрошенные данные");
                                            }
                                            catch (System.IO.IOException ex)
                                            {
                                                System.Windows.Forms.MessageBox.Show(ex.Message);
                                            }
                                            catch (System.InvalidOperationException ex)
                                            {
                                                asForm.Message(ex.Message);
                                            }
                                            System.Threading.Thread.Sleep(requestInterval);
                                            #endregion
                                            if (asForm.bThreadStop) break;
                                            if (countAlarm == countSensThis)
                                            {
                                                asForm.missedWire(numberWire);
                                            }
                                        }
                                        #endregion
                                        #region vend1
                                        else if (vendDev == 1)
                                        {
                                            if (neVTomSilose) { inMonitor = true; neVTomSilose = false; goto linkAnotherSilos; }
                                            if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                                            inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                                            int leg = thDB.GetPinWire(numberWire);
                                            int countSensThis = thDB.GetCountSensInWire(numberWire);
                                            #region sensorRead
                                            try
                                            {
                                                string[] ResponseBuf = new string[countSensThis];
                                                ushort[] ResponseBufferHoldingRegs = new ushort[countSensThis];
                                                ushort newStartReg = 0;
                                                switch (leg)
                                                {
                                                    case 1:
                                                        newStartReg = 101;
                                                        break;
                                                    case 2:
                                                        newStartReg = 152;
                                                        break;
                                                    case 3:
                                                        newStartReg = 203;
                                                        break;
                                                    case 4:
                                                        newStartReg = 254;
                                                        break;
                                                    case 5:
                                                        newStartReg = 305;
                                                        break;
                                                    case 6:
                                                        newStartReg = 356;
                                                        break;
                                                    case 7:
                                                        newStartReg = 407;
                                                        break;
                                                    case 8:
                                                        newStartReg = 458;
                                                        break;
                                                }
                                                ResponseBufferHoldingRegs = ModbusMasterRTU.ReadHoldingRegisters(Address, newStartReg, Convert.ToUInt16(countSensThis));
                                                for (int i = 0; i < countSensThis; i++)
                                                {
                                                    ResponseBuf[i] = ResponseBufferHoldingRegs[i].ToString();
                                                }
                                                for (int i = 0; i < countSensThis; i++)
                                                {
                                                    int tempINTMult10 = int.Parse(ResponseBuf[i]);
                                                    string adrSensCompl = numberDevice.ToString() + ' ' + numberWire.ToString() + ' ' + (i + 1).ToString();
                                                    if (tempINTMult10 == 32767 || tempINTMult10 == 85)
                                                    {
                                                        notFound = true;
                                                    }
                                                    else
                                                    {
                                                        if (tempINTMult10 > (red * 10)) fire = 4;
                                                        else if (tempINTMult10 > (yellow * 10)) fire = 3;
                                                        else if (tempINTMult10 > 0) fire = 2;
                                                    }

                                                    if (tempINTMult10 > 59000)
                                                    {
                                                        tempINTMult10 = (65535 - tempINTMult10) * (-1);
                                                        if (tempINTMult10 < 0) fire = 1;
                                                    }
                                                    if (fire > fireOnWire) fireOnWire = fire;
                                                    if (fire > maxfire && fire > 2) maxfire = fire;
                                                    else if (fire < minfire) minfire = fire;
                                                    
                                                    if (!notFound)
                                                    {
                                                        Temp = tempINTMult10 / (float)10;
                                                        if (Temp < minTemp) minTemp = Temp;
                                                        if (Temp > maxTemp) maxTemp = Temp;
                                                        sumTemp += Temp;
                                                        countTemp++;
                                                        if (inMonitor)
                                                        {
                                                            asForm.changeCell(numberWire, i, tempINTMult10, true, 0,fire);
                                                        }
                                                        if ((thDB.LastTime(thDB.GetIdSens(adrSensCompl)) < DateTime.Now.AddMinutes(-writeTime)))
                                                            asForm.UpdateTempCKomplect(tempINTMult10, i, adrSensCompl);
                                                        else if ((fire > 3) && (thDB.LastTime(thDB.GetIdSens(adrSensCompl)) < DateTime.Now.AddMinutes(-(writeTime / 4))))
                                                        {
                                                            asForm.UpdateTempCKomplect(tempINTMult10, i, adrSensCompl);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        alarm = true;
                                                        asForm.changeAlarmLabel(numberSilos);
                                                        asForm.changeCell(numberWire, i, -1000, true, 0,fire);
                                                    }
                                                    if (notFound)
                                                    {
                                                        notFound = false;
                                                    }
                                                }
                                            }
                                            catch (TimeoutException)
                                            {
                                                asForm.Message("Timeout");
                                            }
                                            catch (Modbus.SlaveException)
                                            {
                                                asForm.Message("Клиент не может прислать запрошенные данные");
                                            }
                                            catch (System.IO.IOException ex)
                                            {
                                                System.Windows.Forms.MessageBox.Show(ex.Message);
                                            }
                                            catch (System.InvalidOperationException ex)
                                            {
                                                asForm.Message(ex.Message);
                                            }
                                            System.Threading.Thread.Sleep(requestInterval);
                                            #endregion
                                            if (asForm.bThreadStop) break;
                                        }
                                        #endregion
                                    }
                                    asForm.changeLabelOnCircle(numberWire, fireOnWire);
                                    
                                }
                                if (asForm.bThreadStop) break;
                            }
                            asForm.changeImage(maxfire!=0?maxfire:minfire, numberSilos);
                            asForm.changeLabelOnImage(numberSilos, maxTemp, minTemp, minTemp == 1000 ? 0 : (float)sumTemp / (float)countTemp);
                            if (numberSilos != asForm.numberMonitor().ToString() && inMonitor) { inMonitor = false; neVTomSilose = true; continue; }
                            inMonitor = (asForm.numberMonitor() != "-1") ? true : false;
                            if (inMonitor)
                            {
                                 asForm.changeLabelTemp(maxTemp, minTemp, minTemp == 1000 ? 0 : (float)sumTemp / (float)countTemp);
                            }
                            if (asForm.bThreadStop) break;
                            if (!alarm) asForm.noVisibleAlarmLabel(numberSilos);
                        }
                        if (asForm.bThreadStop) break;
                    }

                }
                catch (System.Threading.ThreadAbortException abortException)
                {
                    System.Windows.Forms.MessageBox.Show((string)abortException.ExceptionState);
                }
            }
            catch(Exception e)
            {
                thDB.InsertLogErr(e.ToString(), DateTime.Now);
                System.Threading.Thread.Sleep(1000);
            }
        }  
    }
}
