#pragma once

bool SetupExternalGUI(void);

#define CONSTRAIN_POSITION L"ConstrainPosition"
#define X_POS              L"XPos"
#define Y_POS              L"YPos"
#define Z_POS              L"ZPos"
#define X_AXIS             L"XAxis"
#define Y_AXIS             L"YAxis"
#define Z_AXIS             L"ZAxis"

typedef void(__stdcall *ButtonCallback)(const wchar_t* Name);
typedef void(__stdcall *CheckBoxCallback)(const wchar_t* Name, bool IsChecked);
typedef void(__stdcall *IdleCallback)(void);

extern void (__stdcall *aegInitialize)(void);
extern void (__stdcall *aegSetIdleCallback)(IdleCallback Callback);
extern void (__stdcall *aegSetOpenGLWindow)(HWND Window);
extern void (__stdcall *aegSetButtonCallback)(ButtonCallback Callback);
extern void (__stdcall *aegSetCheckBoxCallback)(CheckBoxCallback Callback);
extern void (__stdcall *aegSetEnabled)(const wchar_t* Name, bool IsEnabled);
extern void (__stdcall *aegSetChecked)(const wchar_t* Name, bool IsChecked);
extern void (__stdcall *aegRun)(void);