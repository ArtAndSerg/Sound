//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CSPIN"
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TForm1::N4Click(TObject *Sender)
{
  Close();        
}
//---------------------------------------------------------------------------
void __fastcall TForm1::N2Click(TObject *Sender)
{
     if (OpenDialog1->Execute()) {
         Memo1->Lines->LoadFromFile(OpenDialog1->FileName);    
     }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::N3Click(TObject *Sender)
{
    SaveDialog1->FileName.sprintf("%02d_Sensors.txt", CSpinEdit1->Value);
    if (SaveDialog1->Execute()) {
       Memo1->Lines->SaveToFile(SaveDialog1->FileName);
    }
}
//---------------------------------------------------------------------------
