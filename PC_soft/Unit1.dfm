object Form1: TForm1
  Left = 698
  Top = 223
  Width = 548
  Height = 517
  Caption = #1050#1086#1085#1074#1077#1088#1090#1077#1088' '#1079#1074#1091#1082#1086#1074#1099#1093' '#1092#1072#1081#1083#1086#1074
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object Chart1: TChart
    Left = 0
    Top = 201
    Width = 532
    Height = 223
    BackWall.Brush.Color = clWhite
    BackWall.Brush.Style = bsClear
    Title.Text.Strings = (
      'TChart')
    Title.Visible = False
    LeftAxis.Automatic = False
    LeftAxis.AutomaticMaximum = False
    LeftAxis.AutomaticMinimum = False
    LeftAxis.Maximum = 100
    LeftAxis.Minimum = -100
    Legend.Visible = False
    View3D = False
    Align = alClient
    TabOrder = 0
    object Series1: TFastLineSeries
      Marks.ArrowLength = 8
      Marks.Visible = False
      SeriesColor = clRed
      LinePen.Color = clRed
      XValues.DateTime = False
      XValues.Name = 'X'
      XValues.Multiplier = 1
      XValues.Order = loAscending
      YValues.DateTime = False
      YValues.Name = 'Y'
      YValues.Multiplier = 1
      YValues.Order = loNone
    end
    object Series2: TFastLineSeries
      Marks.ArrowLength = 8
      Marks.Visible = False
      SeriesColor = clGreen
      LinePen.Color = clGreen
      XValues.DateTime = False
      XValues.Name = 'X'
      XValues.Multiplier = 1
      XValues.Order = loAscending
      YValues.DateTime = False
      YValues.Name = 'Y'
      YValues.Multiplier = 1
      YValues.Order = loNone
    end
  end
  object Chart2: TChart
    Left = 0
    Top = 0
    Width = 532
    Height = 201
    BackWall.Brush.Color = clWhite
    BackWall.Brush.Style = bsClear
    Title.Font.Charset = DEFAULT_CHARSET
    Title.Font.Color = clBlack
    Title.Font.Height = -11
    Title.Font.Name = 'Arial'
    Title.Font.Style = []
    Title.Text.Strings = (
      #1048#1089#1082#1072#1078#1077#1085#1080#1103' ('#1088#1072#1079#1085#1086#1089#1090#1100' '#1084#1077#1078#1076#1091' '#1086#1088#1080#1075#1080#1085#1072#1083#1086#1084' '#1080' '#1090#1088#1072#1085#1089#1083#1103#1094#1080#1077#1081')')
    LeftAxis.Automatic = False
    LeftAxis.AutomaticMaximum = False
    LeftAxis.AutomaticMinimum = False
    LeftAxis.Maximum = 10
    LeftAxis.Minimum = -10
    Legend.Visible = False
    View3D = False
    Align = alTop
    TabOrder = 1
    object Series3: TFastLineSeries
      Marks.ArrowLength = 8
      Marks.Visible = False
      SeriesColor = clBlue
      LinePen.Color = clBlue
      XValues.DateTime = False
      XValues.Name = 'X'
      XValues.Multiplier = 1
      XValues.Order = loAscending
      YValues.DateTime = False
      YValues.Name = 'Y'
      YValues.Multiplier = 1
      YValues.Order = loNone
    end
  end
  object TrackBar1: TTrackBar
    Left = 0
    Top = 424
    Width = 532
    Height = 34
    Align = alBottom
    Max = 200000
    Orientation = trHorizontal
    Frequency = 10000
    Position = 0
    SelEnd = 0
    SelStart = 0
    TabOrder = 2
    TickMarks = tmBoth
    TickStyle = tsAuto
    OnContextPopup = TrackBar1ContextPopup
    OnChange = TrackBar1Change
  end
  object MainMenu1: TMainMenu
    Left = 152
    Top = 48
    object N2: TMenuItem
      Caption = #1060#1072#1081#1083
      object WAV1: TMenuItem
        Caption = #1054#1090#1082#1088#1099#1090#1100' '#1079#1074#1091#1082' '#1074' "WAV"'
        OnClick = WAV1Click
      end
      object ADPCM1: TMenuItem
        Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100' '#1074' "ADPCM"'
        OnClick = ADPCM1Click
      end
      object N3: TMenuItem
        Caption = #1042#1099#1093#1086#1076
        OnClick = N3Click
      end
    end
    object Go1: TMenuItem
      Caption = '  [+]  '
      ShortCut = 113
      OnClick = Go1Click
    end
    object N1: TMenuItem
      Caption = '  [-]  '
      ShortCut = 114
      OnClick = N1Click
    end
    object N4: TMenuItem
      Caption = #1057#1087#1088#1072#1074#1082#1072
      ShortCut = 112
      OnClick = N4Click
    end
  end
  object OpenDialog1: TOpenDialog
    Filter = #1047#1074#1091#1082#1086#1074#1099#1077' '#1092#1072#1081#1083#1099' *.wav|*.wav|'#1042#1089#1077' '#1092#1072#1081#1083#1099'  *.*|*.*'
    InitialDir = '.'
    Left = 112
    Top = 56
  end
  object SaveDialog1: TSaveDialog
    DefaultExt = 'raw'
    Filter = #1060#1072#1081#1083#1099' '#1079#1074#1091#1082#1086#1074#1086#1081' '#1090#1088#1072#1085#1089#1083#1103#1094#1080#1080' *.raw|*.raw'
    InitialDir = '.'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofEnableSizing]
    Left = 112
    Top = 88
  end
end
