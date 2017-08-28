//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit2.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

int GetField(char *data, int len, const char *field, char *result);   // for init/reinit call "GetField(NULL,0,NULL, NULL);"
int ParseText(char *data, int len, const char *text, char *result, int resultMaxLen);

void ShiftArray(char array, int len, int count);

void ShiftArray(char newVal, char *array, int len)
{
      for (int i = 0; i < len; i++) {
          array[i] = array[i+1];
      }
      array[len] = newVal;
}


int ParseText(char *data, int len, const char *beginText, const char *endText, char *result, int maxResultLen)
{
    static int  resPtr = 0, found = 0;
    int beginTextLen = strlen(beginText);
    int endTextLen   = strlen(endText);

    for (int i = 0; i < len; i++) {
        if (!resPtr)  {
            ShiftArray(data[i], result, beginTextLen);
            if (!memcmp((void*)result, (void*)beginText, beginTextLen)) {
                   result[resPtr++] = data[i];
            }
        } else {
            result[resPtr++] = data[i];
            if (resPtr > endTextLen) {
                if (!memcmp((void*)&result[resPtr - endTextLen], (void*)endText, endTextLen)) {
                    found = resPtr - endTextLen;
                    resPtr = 0;
                    result[found] = '\0';
                    return found;
                }
            }
        }
    }
    return 0;
}
//------------------------------------------------------------------------------

TForm2 *Form2;
__fastcall TForm2::TForm2(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Button1Click(TObject *Sender)
{
     /*  GET /channels/321949/status/recent.json?key=9W4I06N6EXMLKC6E&status=true HTTP/1.1
         Host: api.thingspeak.com  */
    String resp, req = "GET /channels/321949/status/last.xml?key=9W4I06N6EXMLKC6E&status=true&results=1 HTTP/1.1\r\nHost: api.thingspeak.com\r\n\r\n";
    char str[1000], *answer, res[1000], host[100];
    int i = 0, m;

    ClientSocket1->Host = "thingspeak.com";
    ClientSocket1->Open();
    Memo1->Lines->Add("Connected.");
    Memo1->Lines->Add("S E N D:");
    ClientSocket1->Socket->SendText(req);
    Memo1->Lines->Add(req);
    Sleep(1000);
    resp = ClientSocket1->Socket->ReceiveText();
    Memo1->Lines->Add("R E C E I V E:  " + IntToStr(resp.Length()));
    Memo1->Lines->Add(resp);
    Memo1->Lines->Add("--------------------------------------");
    strcpy(str, resp.c_str());
    ClientSocket1->Close();
    memset (res, 0, sizeof(res));
    for (i = 0; i < resp.Length(); i += 10) {
       if (ParseText(&str[i], 10, "<status>", "</status>", res, sizeof(res))) break;
    }


    Memo1->Lines->Add(String(res));
    strcpy(host, strstr(res, "Host:")+6);
    *(strchr(host, '\r')) = '\0';

    Memo1->Lines->Add(String(host));


    ClientSocket1->Host = String(host);
    ClientSocket1->Open();
    Memo1->Lines->Add("Connected to " + String(host));
    Memo1->Lines->Add("S E N D:");
    ClientSocket1->Socket->SendText(res);
    Memo1->Lines->Add(res);
    Sleep(1000);

    int c = 4, n, f, s = 0, all;
    memset (res, 0, sizeof(res));
    do {
        n = ClientSocket1->Socket->ReceiveBuf((void*)str, sizeof(str));
        if (ParseText(str, n, "Content-Length: ", "\r\n", res, sizeof(res))) {
           all = atoi(res);
        }
        Memo1->Lines->Add(String(str));
        for (i = 0; i < n && c; i++) {
            if (str[i] == '\r' || str[i] == '\n') c--;
            else c = 4;
        }
        Sleep(1);
    } while(c);

    f = FileCreate("rx.txt");
    s += FileWrite(f, &str[i], n - i);
    Application->ProcessMessages();
    while(ClientSocket1->Socket->Connected && n && all - s) {
        n = ClientSocket1->Socket->ReceiveBuf((void*)str, sizeof(str));
        FileWrite(f, str, n);

        s += n;
        Form2->Caption = IntToStr(all - s);
        Application->ProcessMessages();
    }
    FileClose(f);

    Memo1->Lines->Add("R E C E I V E    F I L E:  " + IntToStr(resp.Length()));
    Memo1->Lines->Add(resp);
    Memo1->Lines->Add("*****************************************");
    ClientSocket1->Close();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ClientSocket1Disconnect(TObject *Sender,
      TCustomWinSocket *Socket)
{
     Memo1->Lines->Add("Disconnect");
}
//---------------------------------------------------------------------------
void __fastcall TForm2::ClientSocket1Error(TObject *Sender,
      TCustomWinSocket *Socket, TErrorEvent ErrorEvent, int &ErrorCode)
{
     Memo1->Lines->Add("Error code:" + IntToStr(ErrorCode));
}
//---------------------------------------------------------------------------
void __fastcall TForm2::ClientSocket1Connecting(TObject *Sender,
      TCustomWinSocket *Socket)
{
       Memo1->Lines->Add("Connecting...");
}
//---------------------------------------------------------------------------
void __fastcall TForm2::ClientSocket1Lookup(TObject *Sender,
      TCustomWinSocket *Socket)
{
      Memo1->Lines->Add("Lookup...");
}
//---------------------------------------------------------------------------



int GetField(char *data, int len, const char *field, char *result)
{
    static char currField[20];
    static int  resPtr = 0;
    int fieldLen = strlen(field); // +3 = '</>'
    if (data == NULL) {
        memset ((void*)currField, '\0', sizeof(currField));
        resPtr = 0;
        return 0;
    }
    for (int i = 0; i < len; i++) {
        for (int j = 0; j < fieldLen + 2; j++) { // +4 = data[i]  <  /  >
            currField[j] = currField[j+1];
        }
        currField[fieldLen + 2] = data[i];
        if (resPtr)  {
            result[resPtr++] = data[i];
        }
        if (!resPtr && currField[0] == '<' && currField[fieldLen + 1] == '>') {
            if (!memcmp((void*)&currField[1], (void*)field, fieldLen)) {
               result[resPtr++] = data[i];
            }
        }
        if (resPtr && currField[0] == '<' && currField[1] == '/' && currField[fieldLen + 2] == '>') {
            if (!memcmp((void*)&currField[2], (void*)field, fieldLen)) {
               result[resPtr - fieldLen - 3] = '\0';
               memset ((void*)currField, '\0', sizeof(currField));
               resPtr = 0;
               return 1;
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------



