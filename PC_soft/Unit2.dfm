object Form2: TForm2
  Left = 496
  Top = 513
  Width = 798
  Height = 455
  Caption = 'Form2'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Memo1: TMemo
    Left = 89
    Top = 0
    Width = 693
    Height = 416
    Align = alClient
    ScrollBars = ssBoth
    TabOrder = 0
    WordWrap = False
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 89
    Height = 416
    Align = alLeft
    TabOrder = 1
    object Button1: TButton
      Left = 8
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Button1'
      TabOrder = 0
      OnClick = Button1Click
    end
  end
  object ClientSocket1: TClientSocket
    Active = False
    Address = '127.0.0.1'
    ClientType = ctBlocking
    Host = 'api.thingspeak.com'
    Port = 80
    OnLookup = ClientSocket1Lookup
    OnConnecting = ClientSocket1Connecting
    OnDisconnect = ClientSocket1Disconnect
    OnError = ClientSocket1Error
    Left = 16
    Top = 88
  end
end
