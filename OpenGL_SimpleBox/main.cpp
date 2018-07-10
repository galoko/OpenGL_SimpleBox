#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <WindowsX.h>

#include <string>
#include <iostream>

#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glu.h>			/* OpenGL utilities header file */

#include <glm/glm.hpp>

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

using namespace std;

static HWND WindowHandle;
static HDC WindowDC;
static BOOL IsInFocus;

void OpenConsole(void) {

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
}

bool ProcessMessages(void) {

	MSG Msg;

	// messages
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

		if (Msg.message == WM_QUIT)
			return false;

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return true;
}

static LARGE_INTEGER Freq;

void InitTime(void) {
	QueryPerformanceFrequency(&Freq);
}

LONGLONG GetTime(void) {

	LARGE_INTEGER Result;

	QueryPerformanceCounter(&Result);

	return Result.QuadPart;
}

double TimeToSeconds(LONGLONG Time) {

	return (double)Time / (double)Freq.QuadPart;
} 

void ProcessMouseInput(LONG dx, LONG dy) {

}

static GLfloat trans[3];			/* current translation */
static GLfloat rot[2];				/* current rotation */

LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_CREATE: {

		PIXELFORMATDESCRIPTOR PixelFormatDesc =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
			32,                   // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                   // Number of bits for the depthbuffer
			8,                    // Number of bits for the stencilbuffer
			0,                    // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		WindowDC = GetDC(hWnd);
		int PixelFormat = ChoosePixelFormat(WindowDC, &PixelFormatDesc);
		SetPixelFormat(WindowDC, PixelFormat, &PixelFormatDesc);

		HGLRC hGL = wglCreateContext(WindowDC);
		if (!wglMakeCurrent(WindowDC, hGL))
			throw new runtime_error("Couldn't assign DC to OpenGL");

		glEnable(GL_DEPTH_TEST);

		break;
	}
	case WM_INPUT: {
		UINT RimType;
		HRAWINPUT RawHandle;
		RAWINPUT RawInput;
		UINT RawInputSize, Res;

		RimType = GET_RAWINPUT_CODE_WPARAM(wParam);
		RawHandle = HRAWINPUT(lParam);

		RawInputSize = sizeof(RawInput);
		Res = GetRawInputData(RawHandle, RID_INPUT, &RawInput, &RawInputSize, sizeof(RawInput.header));

		if (Res != (sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE)))
			break;

		if ((RawInput.data.mouse.usFlags & 1) != MOUSE_MOVE_RELATIVE)
			break;

		if (IsInFocus)
			ProcessMouseInput(RawInput.data.mouse.lLastX, RawInput.data.mouse.lLastY);

		break;
	}
	case WM_SIZE: {

		int Width = LOWORD(lParam);
		int Height = HIWORD(lParam);

		glViewport(0, 0, Width, Height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0, (float)Width / Height, 0.001, 100.0);
		glMatrixMode(GL_MODELVIEW);
		gluLookAt(0, -5, 0, 0, 0, 0, 0, 0, 1);
	}
	case WM_KEYDOWN: {

		int Key = (int)wParam;
		// ...?
		break;
	}
	case WM_KEYUP: {

		int Key = (int)wParam;
		// ...?
		break;
	}
	case WM_RBUTTONDOWN:

		break;
	case WM_MOUSEWHEEL: {

		int WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		// ...?
		break;
	}
	case WM_PAINT: {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glClearColor(0, 0, 0, 1.0f);

		rot[0] += 1.0f;
		rot[1] = rot[0];

		glPushMatrix();
		glTranslatef(trans[0], trans[1], trans[2]);
		glRotatef(rot[0], 0.0f, 0.0f, 1.0f);
		// glRotatef(rot[0], 0.0f, 1.0f, 0.0f);
		// glRotatef(rot[1], 0.0f, 0.0f, 1.0f);
		glBegin(GL_TRIANGLES);	
		
#define TOP glIndexi(1); glColor3f(1.0f, 0.0f, 0.0f); glVertex3i( 0,  0,  1)
#define FR  glIndexi(2); glColor3f(0.0f, 1.0f, 0.0f); glVertex3i( 1,  1, -1)
#define FL  glIndexi(3); glColor3f(0.0f, 0.0f, 1.0f); glVertex3i(-1,  1, -1)
#define BR  glIndexi(3); glColor3f(0.0f, 0.0f, 1.0f); glVertex3i( 1, -1, -1)
#define BL  glIndexi(2); glColor3f(0.0f, 1.0f, 0.0f); glVertex3i(-1, -1, -1)
		
		TOP; FL; FR;
		TOP; FR; BR;
		TOP; BR; BL;
		TOP; BL; FL;
		FR; FL; BL;
		BL; BR; FR;

		// glm::perspective()
		

		/*
		glIndexi(1); 
		glColor3f(1.0f, 0.0f, 0.0f);

		glVertex3i(-1, 1, -1);
		*/

		glEnd();

		glPopMatrix();

		glFlush();
		SwapBuffers(WindowDC);

		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM RegisterClass(LPCWSTR ClassName, WNDPROC WndProc, HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = ClassName;
	wcex.hIconSm = 0;

	return RegisterClassExW(&wcex);
}

void CreateMainWindow(HINSTANCE hInstance) {

	const WCHAR* WindowClass = L"OpenGLTest";
	const WCHAR* Title = L"OpenGL Test Box";
	const int Width = 1280;
	const int Height = 960;

	RegisterClass(WindowClass, WndProcCallback, hInstance);

	POINT WindowPos;
	WindowPos.x = (GetSystemMetrics(SM_CXSCREEN) - Width) / 2;
	WindowPos.y = (GetSystemMetrics(SM_CYSCREEN) - Height) / 2;

	WindowHandle = CreateWindowW(WindowClass, Title, WS_POPUP, WindowPos.x, WindowPos.y, Width, Height, nullptr, nullptr, hInstance, nullptr);
	if (WindowHandle == 0)
		throw new runtime_error("Couldn't create window");

	// register raw input

	RAWINPUTDEVICE Rid[1] = {};

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = WindowHandle;

	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])))
		throw new runtime_error("Couldn't register raw devices");

	// glewInit();

	// show the whole thing

	ShowWindow(WindowHandle, SW_SHOW);
}

void Tick(double dt) {

	RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OpenConsole();

	InitTime();

	CreateMainWindow(hInstance);

	LONGLONG LastTick = GetTime();

	// Main message loop:
	while (TRUE) {

		// process input
		if (!ProcessMessages())
			break;

		// time calculation
		LONGLONG Now = GetTime();
		double dt = TimeToSeconds(Now - LastTick);
		LastTick = Now;

		// actual draw call
		Tick(dt);
	}
}