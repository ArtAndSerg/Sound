object Form1: TForm1
  Left = 374
  Top = 195
  Width = 639
  Height = 515
  Caption = #1050#1086#1085#1092#1080#1075#1091#1088#1072#1090#1086#1088
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label2: TLabel
    Left = 16
    Top = 368
    Width = 43
    Height = 16
    Caption = #1053#1086#1084#1077#1088
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object Label1: TLabel
    Left = 16
    Top = 384
    Width = 50
    Height = 16
    Caption = #1082#1072#1085#1072#1083#1072':'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object Memo1: TMemo
    Left = 0
    Top = 0
    Width = 201
    Height = 329
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Courier New'
    Font.Style = []
    ParentFont = False
    ScrollBars = ssVertical
    TabOrder = 0
    WordWrap = False
  end
  object RadioGroup1: TRadioGroup
    Left = 96
    Top = 360
    Width = 105
    Height = 73
    Caption = #1056#1077#1078#1080#1084' '#1088#1072#1073#1086#1090#1099': '
    Items.Strings = (
      #1044#1086#1073#1072#1074#1083#1077#1085#1080#1077
      #1054#1087#1088#1086#1089'...'
      #1053#1072#1073#1083#1102#1076#1077#1085#1080#1077)
    TabOrder = 1
  end
  object Button1: TButton
    Left = 0
    Top = 328
    Width = 57
    Height = 25
    Cursor = crHandPoint
    Caption = #1047#1072#1087#1080#1089#1072#1090#1100
    TabOrder = 2
  end
  object CSpinEdit1: TCSpinEdit
    Left = 16
    Top = 408
    Width = 65
    Height = 26
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    MaxValue = 10
    MinValue = 1
    ParentFont = False
    TabOrder = 3
    Value = 1
  end
  object Button2: TButton
    Left = 64
    Top = 328
    Width = 57
    Height = 25
    Cursor = crHandPoint
    Caption = #1057#1095#1080#1090#1072#1090#1100
    TabOrder = 4
  end
  object Button3: TButton
    Left = 128
    Top = 328
    Width = 73
    Height = 25
    Cursor = crHandPoint
    Caption = #1057#1082#1072#1085#1080#1088#1086#1074#1072#1090#1100
    TabOrder = 5
  end
  object Memo2: TMemo
    Left = 208
    Top = 8
    Width = 201
    Height = 425
    BevelInner = bvNone
    BevelOuter = bvNone
    BorderStyle = bsNone
    Color = clBtnFace
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Courier New'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    TabOrder = 6
    WordWrap = False
  end
  object Memo3: TMemo
    Left = 416
    Top = 8
    Width = 201
    Height = 425
    BevelInner = bvNone
    BevelOuter = bvNone
    BorderStyle = bsNone
    Color = clBtnFace
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Courier New'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    TabOrder = 7
    WordWrap = False
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 437
    Width = 623
    Height = 19
    Panels = <>
    SimplePanel = False
  end
  object MainMenu1: TMainMenu
    Left = 400
    Top = 8
    object N1: TMenuItem
      Caption = #1060#1072#1081#1083
      object N2: TMenuItem
        Caption = #1047#1072#1075#1088#1091#1079#1080#1090#1100
        OnClick = N2Click
      end
      object N3: TMenuItem
        Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100
        OnClick = N3Click
      end
      object N4: TMenuItem
        Caption = #1042#1099#1093#1086#1076
        OnClick = N4Click
      end
    end
    object RS4851: TMenuItem
      Caption = 'RS-485'
      object N5: TMenuItem
        Caption = #1054#1090#1082#1088#1099#1090#1100' '#1087#1086#1088#1090
      end
      object N6: TMenuItem
        Caption = #1047#1072#1082#1088#1099#1090#1100' '#1087#1086#1088#1090
      end
      object N7: TMenuItem
        Caption = #1057#1082#1072#1085#1080#1088#1086#1074#1072#1090#1100' '#1087#1086#1088#1090#1099' ('#1085#1072#1081#1090#1080' '#1091#1089#1090#1088#1086#1081#1089#1090#1074#1086')'
      end
    end
    object N8: TMenuItem
      Caption = #1057#1087#1088#1072#1074#1082#1072
      object N9: TMenuItem
        Caption = #1054' '#1087#1088#1086#1075#1088#1072#1084#1084#1077'...'
      end
    end
  end
  object Timer1: TTimer
    Left = 440
    Top = 8
  end
  object SaveDialog1: TSaveDialog
    DefaultExt = 'txt'
    Filter = #1058#1077#1082#1089#1090#1086#1074#1099#1077' '#1092#1072#1081#1083#1099'|*.txt|'#1042#1089#1077' '#1092#1072#1081#1083#1099' *.*|*.*'
    InitialDir = '.'
    Options = [ofOverwritePrompt, ofNoNetworkButton, ofEnableSizing]
    Left = 336
    Top = 8
  end
  object OpenDialog1: TOpenDialog
    DefaultExt = 'txt'
    Filter = #1058#1077#1082#1089#1090#1086#1074#1099#1077' '#1092#1072#1081#1083#1099'|*.txt|'#1042#1089#1077' '#1092#1072#1081#1083#1099' *.*|*.*'
    InitialDir = '.'
    Options = [ofOverwritePrompt, ofNoNetworkButton, ofEnableSizing]
    Left = 368
    Top = 8
  end
end
