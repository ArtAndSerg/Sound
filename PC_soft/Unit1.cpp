//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
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

void __fastcall TForm1::Button1Click(TObject *Sender)
{
   int f = FileOpen("lenin.wav", fmOpenRead);
   unsigned short tmp;
   double d;
   int i = 0;
   Series1->Clear();

   while (i++ < 3000 && FileRead(f, (void*)&tmp, 2))
   {
     tmp = ((signed short)tmp)/64 + 512;
     d =  tmp;
     Series1->Add(d);
   }

   FileClose(f);
}
//---------------------------------------------------------------------------
 