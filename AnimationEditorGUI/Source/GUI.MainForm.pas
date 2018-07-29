unit GUI.MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls, Vcl.AppEvnts,
  GUI.ExternalInterface, Vcl.ComCtrls;

type
  TAnimationEditorForm = class(TForm)
    LowerControlPanel: TPanel;
    OpenGLPanel: TPanel;
    UpperControlPanel: TPanel;
    ApplicationEvents: TApplicationEvents;
    ConstrainPosition: TButton;
    XPos: TCheckBox;
    YPos: TCheckBox;
    ZPos: TCheckBox;
    XAxis: TCheckBox;
    YAxis: TCheckBox;
    ZAxis: TCheckBox;
    XPosInput: TEdit;
    YPosInput: TEdit;
    ZPosInput: TEdit;
    XAngleInput: TEdit;
    YAngleInput: TEdit;
    ZAngleInput: TEdit;
    XAxisBar: TTrackBar;
    YAxisBar: TTrackBar;
    ZAxisBar: TTrackBar;
    procedure ApplicationEventsMessage(var Msg: tagMSG; var Handled: Boolean);
    procedure FormCreate(Sender: TObject);
    procedure ButtonClick(Sender: TObject);
    procedure CheckBoxClick(Sender: TObject);
    procedure EditKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure EditKeyPress(Sender: TObject; var Key: Char);
    procedure TrackBarChange(Sender: TObject);
  private
    { Private declarations }
    IsInSetMode: Boolean;

    OpenGLWindow: HWND;

    ButtonCallback: TButtonCallback;
    CheckBoxCallback: TCheckBoxCallback;
    EditCallback: TEditCallback;
    TrackBarCallback: TTrackBarCallback;

    procedure SetupCallbacks(Component: TComponent);
  public
    { Public declarations }
    procedure SetOpenGLWindow(Window: HWND);

    procedure SetButtonCallback(Callback: TButtonCallback);
    procedure SetCheckBoxCallback(Callback: TCheckBoxCallback);
    procedure SetEditCallback(Callback: TEditCallback);
    procedure SetTrackBarCallback(Callback: TTrackBarCallback);

    procedure SetEnabled(const Name: String; IsEnabled: Boolean); reintroduce;
    procedure SetChecked(const Name: String; IsChecked: Boolean);
    procedure SetText(const Name, Text: String);
    procedure SetPosition(const Name: String; t: Single);
  end;

var
  AnimationEditorForm: TAnimationEditorForm;

implementation

{$R *.dfm}

procedure TAnimationEditorForm.FormCreate(Sender: TObject);
begin
  SetupCallbacks(Self);
end;

procedure TAnimationEditorForm.SetupCallbacks(Component: TComponent);
var
  SubComponent: TComponent;
begin
  if Component is TButton then
    TButton(Component).OnClick:= ButtonClick
  else
  if Component is TCheckBox then
    TCheckBox(Component).OnClick:= CheckBoxClick
  else
  if Component is TEdit then
  begin
    TEdit(Component).OnKeyDown:= EditKeyDown;
    TEdit(Component).OnKeyPress:= EditKeyPress;
  end
  else
  if Component is TTrackBar then
    TTrackBar(Component).OnChange:= TrackBarChange;

  for SubComponent in Component do
    SetupCallbacks(SubComponent);
end;

procedure TAnimationEditorForm.ButtonClick(Sender: TObject);
var
  Button: TButton;
begin
  if not IsInSetMode and Assigned(ButtonCallback) then
  begin
    Button:= Sender as TButton;
    ButtonCallback(PWideChar(Button.Name));
  end;
end;

procedure TAnimationEditorForm.CheckBoxClick(Sender: TObject);
var
  CheckBox: TCheckBox;
begin
  if not IsInSetMode and Assigned(CheckBoxCallback) then
  begin
    CheckBox:= Sender as TCheckBox;
    CheckBoxCallback(PWideChar(CheckBox.Name), CheckBox.Checked);
  end;
end;

procedure TAnimationEditorForm.EditKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
var
  Edit: TEdit;
begin
  Edit:= Sender as TEdit;

  if Key = VK_RETURN then
  begin
    Key:= 0;

    if Assigned(EditCallback) then
      EditCallback(PWideChar(Edit.Name), PWideChar(Edit.Text));

    Edit.Parent.SetFocus;
  end;
end;

procedure TAnimationEditorForm.EditKeyPress(Sender: TObject; var Key: Char);
begin
  if Ord(Key) = VK_RETURN then
    Key:= #0;
end;

procedure TAnimationEditorForm.TrackBarChange(Sender: TObject);
var
  TrackBar: TTrackBar;
  t: Single;
begin
  if not IsInSetMode and Assigned(TrackBarCallback) then
  begin
    TrackBar:= Sender as TTrackBar;

    t:= (TrackBar.Position - TrackBar.Min) / (TrackBar.Max - TrackBar.Min);

    TrackBarCallback(PWideChar(TrackBar.Name), t);
  end;
end;

procedure TAnimationEditorForm.ApplicationEventsMessage(var Msg: tagMSG;
  var Handled: Boolean);
begin
  // keys redirect
  if Msg.hwnd <> OpenGLWindow then
    if (Msg.message = WM_KEYUP) or (Msg.message = WM_KEYDOWN) then
      PostMessage(OpenGLWindow, Msg.message, Msg.wParam, Msg.lParam);
end;

procedure TAnimationEditorForm.SetOpenGLWindow(Window: HWND);
begin
  OpenGLWindow:= Window;

  Winapi.Windows.SetParent(OpenGLWindow, OpenGLPanel.Handle);
  SetWindowPos(OpenGLWindow, 0, 0, 0, 0, 0, SWP_NOSIZE or SWP_NOZORDER);
  ShowWindow(OpenGLWindow, SW_SHOWNA);
end;

procedure TAnimationEditorForm.SetButtonCallback(Callback: TButtonCallback);
begin
  ButtonCallback:= Callback;
end;

procedure TAnimationEditorForm.SetCheckBoxCallback(Callback: TCheckBoxCallback);
begin
  CheckBoxCallback:= Callback;
end;

procedure TAnimationEditorForm.SetEditCallback(Callback: TEditCallback);
begin
  EditCallback:= Callback;
end;

procedure TAnimationEditorForm.SetTrackBarCallback(Callback: TTrackBarCallback);
begin
  TrackBarCallback:= Callback;
end;

procedure TAnimationEditorForm.SetEnabled(const Name: String;
  IsEnabled: Boolean);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TWinControl then
  begin
    IsInSetMode:= True;
    TWinControl(Component).Enabled:= IsEnabled;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetChecked(const Name: String;
  IsChecked: Boolean);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TCheckBox then
  begin
    IsInSetMode:= True;
    TCheckBox(Component).Checked:= IsChecked;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetText(const Name, Text: String);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TEdit then
  begin
    if TEdit(Component).Focused then
      Exit;

    IsInSetMode:= True;
    TEdit(Component).Text:= Text;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetPosition(const Name: String; t: Single);
var
  Component: TComponent;
  TrackBar: TTrackBar;
  Position: Integer;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TTrackBar then
  begin
    TrackBar:= TTrackBar(Component);
    (*
    if TrackBar.Focused then
      Exit;
    *)

    Position:= TrackBar.Min + Round((TrackBar.Max - TrackBar.Min) * t);

    IsInSetMode:= True;
    TrackBar.Position:= Position;
    IsInSetMode:= False;
  end;
end;

end.
