//����� ���������
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleModbusSlave.h>
#include <PortExpander_I2C.h>
//������������ ����������
unsigned short mode=0;
short p=-1,oldp=-1;
bool resetLoop=false;
const unsigned short count = 62;
//������������ ����������� �������� �� ��������
const unsigned short  countSensMAx = 60;
unsigned short numberOfDevices; 
//������ ��� ������� �� �������
unsigned int modMas[2];
//������� ��� �������� ���������� � ������ ������
unsigned int wire1[count];
unsigned int wire2[count];
unsigned int wire3[count];
unsigned int wire4[count];
unsigned int wire5[count];
unsigned int wire6[count];
unsigned int wire7[count];
unsigned int wire8[count];
unsigned int wire9[count];
unsigned int wire10[count];
bool checkWire[10];
bool check[600];
bool switchs[]={false,false,false,false,false,false,false,false};
uint8_t adrSlave=0;

OneWire oW[] = {PB9,PB8,PB12,PB13,PB5,PB4,PB3,PA15,PB14,PB15};
//OneWire oW[] = {PB15,PB14,PA15,PB3,PB4,PB5,PB13,PB12,PB8,PB9};
DallasTemperature sens[] = {&oW[0],&oW[1],&oW[2],&oW[3],&oW[4],&oW[5],&oW[6],&oW[7],&oW[8],&oW[9]};
PortExpander_I2C pe(0x38);
//�������, ������� ����������� 1 ��� � ������ ������ �����
void setup()
{
  //������������ �������������
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
  pe.init();
  for( int i = 0; i < 8; i++ ){
    pe.pinMode(i,INPUT);
  } 
  
  for(int i=0;i<600;i++)
  {
    check[i]=false;;
  }
  for(int i=0;i<10;i++)
  {
    checkWire[i] = false;
  }
  pinMode(PC13, OUTPUT);
  adrSlave = 255-pe.digitalReadAll(); 
  //������������� �������
  modbus_configure(&Serial1, 9600, SERIAL_8N2, adrSlave, PA8, 2, modMas);  
  modMas[0]=0; modMas[1]=0;
  //������������� �������� ��� ������� ���������� ���������� 32767
  for(int j=2;j<count;j++)
    {
      wire1[j]=32767;
      wire2[j]=32767;
      wire3[j]=32767;
      wire4[j]=32767;
      wire5[j]=32767;
      wire6[j]=32767;
      wire7[j]=32767;
      wire8[j]=32767;
      wire9[j]=32767;
      wire10[j]=32767;
    }
  //� ������ ��� ������ �������� ��������� ����� ����, �� ������� ��� �����
  wire1[1]=0;  
  wire1[0]=0;
  
  wire2[1]=0;
  wire2[0]=1;
  
  wire3[1]=0;
  wire3[0]=2;
  
  wire4[1]=0;
  wire4[0]=3;
  
  wire5[1]=0;
  wire5[0]=4;
  
  wire6[1]=0;
  wire6[0]=5;
  
  wire7[1]=0;
  wire7[0]=6;
  
  wire8[1]=0;
  wire8[0]=7;

  wire9[1]=0;
  wire9[0]=8;

  wire10[1]=0;
  wire10[0]=9;
}
//�������� ������� ��������� 
//����������� � ����������� �����
void loop(void)
{ 
  //���� ������ �� ����� �� ������ (mode = 0)
  if (mode==0)
  {
    //���������, ���� �� ������ �� ��������� ��
    modbus_update_mas(modMas,2);
    mode = modMas[1];
  }
  //���� ������ �� ����� �� ������ (mode = 0)
  if (mode==0)
  { 
    //���� �� ���� �����
    for(int p=0;p<10;p++)
    {      
      //���������, ���� �� ������ �� ��������� ��. ����� �� ������ �� ����� ������ ��������
      modbus_update_mas(modMas,2);
      modbus_update();
      delay(50);
      mode = modMas[1];
      //���� ������� ��� � �� ����, ���������� ������
      if (mode==0) {}
      else 
      {
	//���� ������ ������, ������������� ����� ��������
	//���������� ��� ����� ��� ���������� �������� 
        resetLoop=true;
        //�������� �� ����� �� ����
        break; 
      }
      //��� ����� ����� ���������� ����� � ���� �����, �� ������� �� �����������
      if (resetLoop==true) { resetLoop = false; p=oldp==9?0:oldp+1;}
      oldp=p;
      sens[p].begin();
      //���������� ��� ������� �� �������� �����
      sens[p].requestTemperatures();
      //������� ������� �������� ���� ��������
      numberOfDevices = sens[p].getDeviceCount();
      if (numberOfDevices>60) {numberOfDevices=60;}
      //���� ����� �������� ���, ����������� ��������������� 9999. ��� ������ ��� �������� ��� �� ��� ����������, ��� ���� ����
      if (numberOfDevices==0) 
      {
        if (checkWire[p]==true)
        {
            for(int i=countSensMAx*p;i<countSensMAx*p+countSensMAx;i++)
            {
              check[i]=false;
              switch(p)
              {
                case 0:
                wire1[i+2]=9999;
                break;
                case 1:
                wire2[i+2]=9999;
                break;
                case 2:
                wire3[i+2]=9999;
                break;
                case 3:
                wire4[i+2]=9999;
                break;
                case 4:
                wire5[i+2]=9999;
                break;
                case 5:
                wire6[i+2]=9999;
                break;
                case 6:
                wire7[i+2]=9999;
                break;
                case 7:
                wire8[i+2]=9999;
                break;
                case 8:
                wire9[i+2]=9999;
                break;
                case 9:
                wire10[i+2]=9999;
                break;
              } 
            }
            checkWire[p] = false;
        }
      }
      //���� ���� ���� ������ ��� �������, ������� ���������� � ���� � ������
      else 
      {
        checkWire[p] = true;
        for(int i=countSensMAx*p;i<countSensMAx*p+numberOfDevices;i++)
        {
          check[i]=false;
        }
	//������� ����������� � ������ ������������ ������ ����������� � �������
        for(int i=0;i<numberOfDevices; i++)
        {
            modbus_update();
            digitalWrite(PC13, LOW);
	    //�������� ������� �� �������
            byte lvl = sens[p].getUserDataByIndex(i);
	    //�������� ����������� ������� ���������
            int temp = sens[p].getTempCByIndex(i)*100;
	    //� ����������� �� ������ ����, ������� ����������� � ������
            switch(p)
            {
              case 0:
              wire1[lvl+1]=temp;
              check[lvl-1] = true;
              break;
              case 1:
              wire2[lvl+1]=temp;
              check[60+lvl-1] = true;
              break;
              case 2:
              wire3[lvl+1]=temp;
              check[120+lvl-1] = true;
              break;
              case 3:
              wire4[lvl+1]=temp;
              check[180+lvl-1] = true;
              break;
              case 4:
              wire5[lvl+1]=temp;
              check[240+lvl-1] = true;
              break;
              case 5:
              wire6[lvl+1]=temp;
              check[300+lvl-1] = true;
              break;
              case 6:
              wire7[lvl+1]=temp;
              check[360+lvl-1] = true;
              break;
              case 7:
              wire8[lvl+1]=temp;
              check[420+lvl-1] = true;
              break;
              case 8:
              wire9[lvl+1]=temp;
              check[480+lvl-1] = true;
              break;
              case 9:
              wire10[lvl+1]=temp;
              check[540+lvl-1] = true;
              break;
            }         
            digitalWrite(PC13, HIGH);
            delay(150);
        }
        for(int i=countSensMAx*p;i<countSensMAx*p+numberOfDevices;i++)
          if (check[i]==false)
            switch(p)
            {
              case 0:
              wire1[i+2]=9999;
              break;
              case 1:
              wire2[i+2]=9999;
              break;
              case 2:
              wire3[i+2]=9999;
              break;
              case 3:
              wire4[i+2]=9999;
              break;
              case 4:
              wire5[i+2]=9999;
              break;
              case 5:
              wire6[i+2]=9999;
              break;
              case 6:
              wire7[i+2]=9999;
              break;
              case 7:
              wire8[i+2]=9999;
              break;
              case 8:
              wire9[i+2]=9999;
              break;
              case 9:
              wire10[i+2]=9999;
              break;
            } 
      }    
    }
  }
  //���� ������ ������ �� �����
  else if (mode==1)
  {
    //�������, �� ����� ��� ������ ������
    p=modMas[0];
    //���������� �� ������� ������ �� ������� �������������� ����������� ����
    switch(p)
    {
            //���� ��� 0
            case 0:
            wire1[1]=2;
            modbus_update_mas(wire1,count);
            modbus_update();
            if (wire1[1]==0) {mode = 0; modMas[1]=0;}
            break;
            //���� ��� 1
	    case 1:
            wire2[1]=2;
            modbus_update_mas(wire2,count);
            modbus_update();
            if (wire2[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 2:
            wire3[1]=2;
            modbus_update_mas(wire3,count);
            modbus_update();
            if (wire3[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 3:
            wire4[1]=2;
            modbus_update_mas(wire4,count);
            modbus_update();
            if (wire4[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 4:
            wire5[1]=2;
            modbus_update_mas(wire5,count);
            modbus_update();
            if (wire5[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 5:
            wire6[1]=2;
            modbus_update_mas(wire6,count);
            modbus_update();
            if (wire6[1]==0){mode = 0; modMas[1]=0;}
            break;
            case 6:
            wire7[1]=2;
            modbus_update_mas(wire7,count);
            modbus_update();
            if (wire7[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 7:
            wire8[1]=2;
            modbus_update_mas(wire8,count);
            modbus_update();
            if (wire8[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 8:
            wire9[1]=2;
            modbus_update_mas(wire9,count);
            modbus_update();
            if (wire9[1]==0) {mode = 0; modMas[1]=0;}
            break;
            case 9:
            wire10[1]=2;
            modbus_update_mas(wire10,count);
            modbus_update();
            if (wire10[1]==0) {mode = 0; modMas[1]=0;}
            break;
    }           
  }
}

