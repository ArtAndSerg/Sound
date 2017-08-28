//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
                     
#include "Unit1.h"
#include "Unit2.h"
#include "math.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

/* Table of index changes */
const unsigned char IndexTable[16] = {
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8,
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8
};

/* Quantizer step size lookup table */
const int StepSizeTable[89] = {
 7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
int predsample = 0;	/* Output of ADPCM predictor */
char index = 0;		/* Index into step size table */

signed short  *wavData = NULL;
signed short  *wavData2 = NULL;
unsigned char *adpcmData = NULL;
int sizeWavInBytes = 0;

signed short ADPCMDecoder(unsigned char code)
{
   int step;
   int diffq;

   step = StepSizeTable[index];

   diffq = step >> 3;
   if( code & 4 ) {
      diffq += step;
   }
   if( code & 2 ) {
      diffq += (step >> 1);
   }
   if( code & 1 ) {
      diffq += (step >> 2);
   }

   if( code & 8 ) {
      predsample -= diffq;
   } else {
      predsample += diffq;
   }

   if(predsample > 32767) {
      predsample = 32767;
   } else if (predsample < -32768) {
      predsample = -32768;
   }

   index += IndexTable[code];

   if( index < 0 ) {
      index = 0;
   }
   if( index > 88 ) {
      index = 88;
   }

   return( (unsigned short)(predsample) );
}
//------------------------------------------------------------------------------

unsigned char ADPCMEncoder( signed short sample)
{
   int step;
   int diffq, diff;
   char code;

   step = StepSizeTable[index];

   // Compute the difference between the acutal sample (sample) and the
   // the predicted sample (predsample)
   diff = sample - predsample;
   if(diff >= 0) {
   	code = 0;
   } else {
   	code = 8;
   	diff = -diff;
   }

   // Quantize the difference into the 4-bit ADPCM code using the
   // the quantizer step size
   // Inverse quantize the ADPCM code into a predicted difference
   // using the quantizer step size
   diffq = step >> 3;
   if( diff >= step ) {
   	code |= 4;
   	diff -= step;
   	diffq += step;
   }
   step >>= 1;
   if( diff >= step ) {
   	code |= 2;
   	diff -= step;
   	diffq += step;
   }
   step >>= 1;
   if( diff >= step ) {
   	code |= 1;
   	diffq += step;
   }

   // Fixed predictor computes new predicted sample by adding the
   // old predicted sample to predicted difference
   if( code & 8 ) {
   	predsample -= diffq;
   } else {
   	predsample += diffq;
   }

   // Check for overflow of the new predicted sample
   if(predsample > 32767) {
        predsample = 32767;
   } else if(predsample < -32768) {
        predsample = -32768;
   }

  // Find new quantizer stepsize index by adding the old index
  // to a table lookup using the ADPCM code
  index += IndexTable[code];

  // Check for overflow of the new quantizer step size index
  if( index < 0 ) {
  	index = 0;
  }
  if( index > 88 ) {
  	index = 88;
  }

  // Return the new ADPCM code
  return ( code & 0x0f );
}
//-----------------------------------------------------------------------------


void EncodeFrom_WAV_to_ADPCM(unsigned char *adpcm, signed short *wav, int wavLen)
{
   for (int i = 0; i < wavLen; i++) {
     if (!(i & 0x01)) {
        adpcm[i/2] = ADPCMEncoder(wav[i]) << 4;
     }
     else {
        adpcm[i/2] |= (ADPCMEncoder(wav[i]));
     }
   }
}
//-----------------------------------------------------------------------------

void DecodeFrom_ADPCM_to_WAV(signed short *wav, unsigned char *adpcm, int adpcmLen)
{
    for (int i = 0; i < adpcmLen*2; i++) {
       if (!(i & 0x01)) {
          wav[i] = ADPCMDecoder((adpcm[i/2] >> 4) & 0x0F);
       }
       else {
          wav[i] = ADPCMDecoder((adpcm[i/2]) & 0x0F);
       }
    }
}
//-----------------------------------------------------------------------------








void __fastcall TForm1::N4Click(TObject *Sender)
{
  String str = "help.pdf";
  ShellExecute(Handle, NULL, str.c_str(), NULL, NULL, SW_RESTORE);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::WAV1Click(TObject *Sender)
{
    int f, size;
    sizeWavInBytes = 0;
    if (OpenDialog1->Execute()) {
       f = FileOpen(OpenDialog1->FileName, fmOpenRead);
       size = FileSeek(f, 0, 2);
       if (size > 100000000 || size < 200) {
           Application->MessageBoxA("������ �������� �����!\n��� ������������ ������ (�� 200� �� 100��)!", "� � � � � � � � !", MB_OK);
           FileClose(f);
           return;
       }
       FileSeek(f, 100, 0);
       size = size - 100;
       if (wavData != NULL) {
           delete wavData;
       }
       wavData = new signed short [6400 + size / sizeof(signed short)];
       memset(wavData, 0, 6400 + size / sizeof(signed short));
       if (wavData == NULL) {
            Application->MessageBoxA("������ ��������� ������ ��� WAV!", "� � � � � � � � !", MB_OK);
            FileClose(f);
            return;
       }
       if (FileRead(f, (void*)&wavData[3200], size) != size) {
            Application->MessageBoxA("������ ������ �����!", "� � � � � � � � !", MB_OK);
            FileClose(f);
            return;
       }
       FileClose(f);

       size += 6400;
       if (adpcmData != NULL) {
           delete adpcmData;
       }
       adpcmData = new unsigned char [(size / sizeof(signed short)) / 2];
       if (adpcmData == NULL) {
            Application->MessageBoxA("������ ��������� ������ ��� ADPCM!", "� � � � � � � � !", MB_OK);
            FileClose(f);
            return;
       }
       predsample = 0;
       index = 0;
       EncodeFrom_WAV_to_ADPCM(adpcmData, wavData, size / sizeof(signed short));
       sizeWavInBytes = size;


       if (wavData2 != NULL) {
           delete wavData2;
       }
       wavData2 = new signed short [size / sizeof(signed short)];
       if (wavData2 == NULL) {
            Application->MessageBoxA("������ ��������� ������ ��� WAV2!", "� � � � � � � � !", MB_OK);
            return;
       }
       predsample = 0;
       index = 0;
       DecodeFrom_ADPCM_to_WAV(wavData2, adpcmData, (size / sizeof(signed short)) / 2);

       Form1->Caption = "���� \"" + ExtractFileName(OpenDialog1->FileName) + "\", ������ " +
                        IntToStr(sizeWavInBytes / 1024) + "��. (" + IntToStr((sizeWavInBytes / 4) / 1024) + "��.)";
       N1->Click();
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ADPCM1Click(TObject *Sender)
{
    int f;
    if (SaveDialog1->Execute()) {
       if (!sizeWavInBytes) {
            Application->MessageBoxA("������! ������ ����� ������� ���� � \"WAV\"", "� � � � � � � � !", MB_OK);
            return;
       }
       if (FileExists(SaveDialog1->FileName)) {
          if (!DeleteFile(SaveDialog1->FileName)) {
              Application->MessageBoxA("������ ������� � ����� (�� ���� �������)!", "� � � � � � � � !", MB_OK);
              return;
          }
       }
       f = FileCreate(SaveDialog1->FileName);

       if (FileWrite(f, (void*)adpcmData, (sizeWavInBytes / sizeof(signed short)) / 2) != (sizeWavInBytes / sizeof(signed short)) / 2) {
            Application->MessageBoxA("������ ������ �����!", "� � � � � � � � !", MB_OK);
       }
       FileClose(f);
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
    if (wavData2 != NULL) {
           delete wavData2;
    }
    if (wavData != NULL) {
           delete wavData;
    }
    if (adpcmData != NULL) {
           delete adpcmData;
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::N1Click(TObject *Sender)
{
   static int start = 0;
   int step = 320000;
   if (!sizeWavInBytes) return;
   if (step > sizeWavInBytes/2) step = sizeWavInBytes/2;
   
   if (Sender == N1) {
      start += step;
      if (start >= sizeWavInBytes/2) start = 0;
   } else {
      start -= step;
      if (start < 0) start = sizeWavInBytes/2;
   }
   Series1->Clear();
   Series2->Clear();
   FastLineSeries1->Clear();
   for (int i = start; i < start + step; i++) {
       Series1->Add((double) wavData[i] / 327.67);
       Series2->Add((double) (wavData2[i]) / 327.67);
       FastLineSeries1->Add((double) wavData[i] / 327.67 - (double) wavData2[i] / 327.67);
   }
}
//---------------------------------------------------------------------------


void __fastcall TForm1::N3Click(TObject *Sender)
{
unsigned char *res = new unsigned char[1000000];
   int f = FileOpen("lenin.wav", fmOpenRead);
   signed short tmp;
   double d;
   int i = 0, n =0, j = 0;
   Series1->Clear();
   Series2->Clear();
   FastLineSeries1->Clear();
   FileSeek(f, 200, 0);
   for (i = 0; i < 10000; i++) {
     FileRead(f, (void*)&tmp, 2);
     //tmp = sin((double)i / 100.) * 32000;
     if (!(i & 0x01)) {
        res[n] = ADPCMEncoder(tmp) << 4;

     }
     else {
        res[n] |= (ADPCMEncoder(tmp) );
        n++;
     }
     tmp = ((signed short)tmp)/64 + 512;
     d =  tmp;
     Series1->Add(d);
   }
   FileClose(f);

   for (i = 0; i < n; i++) {
       tmp = ADPCMDecoder((res[i] >> 4) & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

       tmp = ADPCMDecoder(res[i] & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

   }
   delete res;
        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::test1Click(TObject *Sender)
{
   Form2->Show();
}
//---------------------------------------------------------------------------

