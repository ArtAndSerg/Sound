//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Chart.hpp>
#include <ExtCtrls.hpp>
#include <Series.hpp>
#include <TeEngine.hpp>
#include <TeeProcs.hpp>
#include <Menus.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TChart *Chart1;
        TFastLineSeries *Series1;
        TChart *Chart2;
        TMainMenu *MainMenu1;
        TMenuItem *Go1;
        TMenuItem *N1;
        TMenuItem *N2;
        TMenuItem *WAV1;
        TMenuItem *ADPCM1;
        TMenuItem *N3;
        TMenuItem *N4;
        TOpenDialog *OpenDialog1;
        TSaveDialog *SaveDialog1;
        TTrackBar *TrackBar1;
        TFastLineSeries *Series2;
        TFastLineSeries *Series3;
        void __fastcall N4Click(TObject *Sender);
        void __fastcall WAV1Click(TObject *Sender);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall ADPCM1Click(TObject *Sender);
        void __fastcall N1Click(TObject *Sender);
        void __fastcall N3Click(TObject *Sender);
        void __fastcall TrackBar1ContextPopup(TObject *Sender,
          TPoint &MousePos, bool &Handled);
        void __fastcall Go1Click(TObject *Sender);
        void __fastcall TrackBar1Change(TObject *Sender);
       
private:	// User declarations
public:		// User declarations
        __fastcall TForm1(TComponent* Owner);
        void __fastcall refreshGUI(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
