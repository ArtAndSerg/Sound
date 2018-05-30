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
unsigned short  *wavData2 = NULL;
unsigned char *adpcmData = NULL;
int sizeWavInBytes = 0;

int showingWindow = 10000;

unsigned short ADPCMDecoder(unsigned char code)
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

   if(predsample > 32766) {
      predsample = 32766;
   } else if (predsample < -32766) {
      predsample = -32766;
   }

   index += IndexTable[code];

   if( index < 0 ) {
      index = 0;
   }
   if( index > 88 ) {
      index = 88;
   }

   return( (unsigned short)(predsample+32766) );
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

void DecodeFrom_ADPCM_to_WAV(unsigned short *wav, unsigned char *adpcm, int adpcmLen)
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

void  __fastcall TForm1::OpenWAV(String filename)
{
       int f, size;
       sizeWavInBytes = 0;
       f = FileOpen(filename, fmOpenRead);
       size = FileSeek(f, 0, 2);
       if (size > 100000000 || size < 200) {
           Application->MessageBoxA("Ошибка открытия файла!\nИли недопустимый размер (от 200б до 100Мб)!", "В Н И М А Н И Е !", MB_OK);
           FileClose(f);
           return;
       }
       FileSeek(f, 44, 0);
       size = size - 44;
       if (wavData != NULL) {
           delete wavData;
           wavData = NULL;
       }
       wavData = new signed short [size / sizeof(signed short)];
       if (wavData == NULL) {
            Application->MessageBoxA("Ошибка выделения памяти для WAV!", "В Н И М А Н И Е !", MB_OK);
            FileClose(f);
            return;
       }
       if (FileRead(f, (void*)wavData, size) != size) {
            Application->MessageBoxA("Ошибка чтения файла!", "В Н И М А Н И Е !", MB_OK);
            FileClose(f);
            return;
       }
       FileClose(f);

       if (adpcmData != NULL) {
           delete adpcmData;
           adpcmData = NULL;
       }
       adpcmData = new unsigned char [(size / sizeof(signed short)) / 2];
       if (adpcmData == NULL) {
            Application->MessageBoxA("Ошибка выделения памяти для ADPCM!", "В Н И М А Н И Е !", MB_OK);
            FileClose(f);
            return;
       }
       predsample = 0;
       index = 0;
       EncodeFrom_WAV_to_ADPCM(adpcmData, wavData, size / sizeof(signed short));
       sizeWavInBytes = size;


       if (wavData2 != NULL) {
           delete wavData2;
           wavData2 = NULL;
       }
       wavData2 = new unsigned short [size / sizeof(unsigned short)];
       if (wavData2 == NULL) {
            Application->MessageBoxA("Ошибка выделения памяти для WAV2!", "В Н И М А Н И Е !", MB_OK);
            return;
       }
       predsample = 0;
       index = 0;
       DecodeFrom_ADPCM_to_WAV(wavData2, adpcmData, (size / sizeof(signed short)) / 2);

       Form1->Caption = "Файл \"" + ExtractFileName(OpenDialog1->FileName) + "\", размер " +
                        IntToStr(sizeWavInBytes / 1024) + "кБ. (" + IntToStr((sizeWavInBytes / 4) / 1024) + "кБ.)";
       showingWindow = 10000;
       TrackBar1->Max = sizeWavInBytes/2;
       TrackBar1->Position = 0;
}
//---------------------------------------------------------------------------


void __fastcall TForm1::WAV1Click(TObject *Sender)
{

    if (OpenDialog1->Execute()) {
       OpenWAV(OpenDialog1->FileName);
       refreshGUI();
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SaveADPCM(String filename)
{
      int f;
      if (!sizeWavInBytes) {
            Application->MessageBoxA("Ошибка! Сперва нужно открыть звук в \"WAV\"", "В Н И М А Н И Е !", MB_OK);
            return;
       }
       if (FileExists(filename)) {
          if (!DeleteFile(filename)) {
              Application->MessageBoxA("Ошибка доступа к файлу (не могу удалить)!", "В Н И М А Н И Е !", MB_OK);
              return;
          }
       }
       f = FileCreate(filename);

       if (FileWrite(f, (void*)adpcmData, (sizeWavInBytes / sizeof(signed short)) / 2) != (sizeWavInBytes / sizeof(signed short)) / 2) {
            Application->MessageBoxA("Ошибка записи файла!", "В Н И М А Н И Е !", MB_OK);
       }
       FileClose(f);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ADPCM1Click(TObject *Sender)
{
    if (SaveDialog1->Execute()) {
        SaveADPCM(SaveDialog1->FileName);
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
   if (showingWindow < sizeWavInBytes) {
      showingWindow *= 2;
   }
   refreshGUI();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Go1Click(TObject *Sender)
{
    if (showingWindow > 10) {
      showingWindow /= 2;
   }
   refreshGUI();
}
//---------------------------------------------------------------------------


void __fastcall TForm1::N3Click(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------



void __fastcall TForm1::refreshGUI(void)
{
   if (!sizeWavInBytes) return;
   double *tmp1 = new double [showingWindow];
   double *tmp2 = new double [showingWindow];
   double *tmp3 = new double [showingWindow];
   Series1->Clear();
   Series2->Clear();
   Series3->Clear();
   for (int i = TrackBar1->Position; i < TrackBar1->Position + showingWindow && i < sizeWavInBytes/2; i++) {
        tmp1[i-TrackBar1->Position] = (double) (wavData[i]) / 327.67 + 100.;
        tmp2[i-TrackBar1->Position] = (double) wavData2[i] / 327.67;
        tmp3[i-TrackBar1->Position] = tmp2[i-TrackBar1->Position] - tmp1[i-TrackBar1->Position];
   }
   Series1->AddArray(tmp1, showingWindow);
   Series2->AddArray(tmp2, showingWindow);
   Series3->AddArray(tmp3, showingWindow);
   delete tmp1;
   delete tmp2;
   delete tmp3;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar1ContextPopup(TObject *Sender,
      TPoint &MousePos, bool &Handled)
{
   refreshGUI();
}
//---------------------------------------------------------------------------


void __fastcall TForm1::TrackBar1Change(TObject *Sender)
{
    refreshGUI();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::N5Click(TObject *Sender)
{
   TSearchRec tFileInfo;
   String     vasFilename, path = ExtractFilePath(Application->ExeName);
   vasFilename = path + "wav\\*.wav";
   if(FindFirst(vasFilename, faAnyFile, tFileInfo) == 0)
   {
     do
     {
       if((tFileInfo.Name != ".") && (tFileInfo.Name != "..") &&
           (tFileInfo.Attr & faDirectory) != faDirectory)
       {
           OpenWAV(path + "wav\\" + tFileInfo.Name);
           refreshGUI();
           Application->ProcessMessages();
           SaveADPCM(path + "raw\\" + ChangeFileExt(tFileInfo.Name, ".raw"));
       }
     } while (FindNext(tFileInfo) == 0);
   }
   FindClose(tFileInfo);
  
}
//---------------------------------------------------------------------------

