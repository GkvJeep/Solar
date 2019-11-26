object FormIpg: TFormIpg
  Left = 0
  Top = 0
  Caption = 'Solar IPG'
  ClientHeight = 543
  ClientWidth = 1153
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 16
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 1153
    Height = 41
    Align = alTop
    TabOrder = 0
    object Edit1: TEdit
      Left = 409
      Top = 8
      Width = 49
      Height = 24
      BiDiMode = bdRightToLeft
      ParentBiDiMode = False
      TabOrder = 0
      Text = '600'
      OnKeyPress = Edit1KeyPress
    end
    object Button5: TButton
      Left = 87
      Top = 10
      Width = 91
      Height = 25
      Caption = 'Open'
      TabOrder = 1
      OnClick = Button5Click
    end
    object ComboBox1: TComboBox
      Left = 8
      Top = 8
      Width = 73
      Height = 24
      Enabled = False
      TabOrder = 2
      Text = 'Not aviable COM Port'
    end
    object Button1: TButton
      Left = 474
      Top = 10
      Width = 101
      Height = 25
      Caption = 'Set TResHold'
      TabOrder = 3
      OnClick = Button1Click
    end
    object Button2: TButton
      Left = 640
      Top = 10
      Width = 75
      Height = 25
      Caption = 'Get Status'
      TabOrder = 4
      OnClick = Button2Click
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 512
    Width = 1153
    Height = 31
    Panels = <
      item
        Text = 'COM ???'
        Width = 150
      end
      item
        Text = 'SMS Out 0'
        Width = 150
      end
      item
        Text = 'SMS In 0'
        Width = 150
      end
      item
        Width = 50
      end>
  end
  object LogMemo: TMemo
    Left = 792
    Top = 41
    Width = 361
    Height = 471
    Align = alRight
    Color = clNone
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clLime
    Font.Height = -16
    Font.Name = 'Consolas'
    Font.Style = []
    Lines.Strings = (
      'Time')
    ParentFont = False
    ScrollBars = ssVertical
    TabOrder = 2
    OnDblClick = LogMemoDblClick
  end
  object Chart1: TChart
    Left = 0
    Top = 41
    Width = 792
    Height = 471
    Cursor = crCross
    Legend.Visible = False
    Title.Text.Strings = (
      'TChart')
    Title.Visible = False
    AxisBehind = False
    BottomAxis.Grid.SmallDots = True
    BottomAxis.Grid.SmallSpace = 4
    BottomAxis.LabelsFormat.Font.Color = clSilver
    BottomAxis.Title.Caption = 'Time'
    BottomAxis.Title.Font.Color = clSilver
    DepthTopAxis.LabelsFormat.Font.Color = clSilver
    DepthTopAxis.Title.Font.Color = clSilver
    Hover.Visible = False
    LeftAxis.Axis.Style = psDot
    LeftAxis.Axis.Visible = False
    LeftAxis.Grid.SmallDots = True
    LeftAxis.Grid.SmallSpace = 10
    LeftAxis.Increment = 10.000000000000000000
    LeftAxis.LabelsFormat.Font.Color = clSilver
    LeftAxis.MinorTicks.Visible = False
    LeftAxis.TickLength = 0
    LeftAxis.TicksInner.Visible = False
    LeftAxis.TickOnLabelsOnly = False
    LeftAxis.Title.Caption = 'FLX*10'
    LeftAxis.Title.Font.Color = clSilver
    LeftAxis.Title.Font.Height = -13
    Shadow.Visible = False
    TopAxis.Grid.SmallDots = True
    View3D = False
    Zoom.Pen.Style = psDot
    Zoom.Pen.Visible = False
    Align = alClient
    Color = clBlack
    TabOrder = 3
    DefaultCanvas = 'TGDIPlusCanvas'
    PrintMargins = (
      15
      20
      15
      20)
    ColorPaletteIndex = 13
    object Series1: TLineSeries
      SeriesColor = 33023
      Brush.BackColor = clDefault
      Dark3D = False
      DrawStyle = dsCurve
      OutLine.Color = 33023
      OutLine.Width = 2
      OutLine.Visible = True
      Pointer.InflateMargins = True
      Pointer.Style = psRectangle
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
  end
  object OpenDialog1: TOpenDialog
    Left = 304
    Top = 9
  end
  object MainMenu1: TMainMenu
    Left = 568
    Top = 280
    object File1: TMenuItem
      Caption = '&File'
      object DataTest1: TMenuItem
        Caption = 'DataTest'
        OnClick = DataTest1Click
      end
    end
    object Option1: TMenuItem
      Caption = '&Option'
    end
    object Help1: TMenuItem
      Caption = '&Help'
    end
  end
end
