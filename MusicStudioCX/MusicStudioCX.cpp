// MusicStudioCX.cpp : Defines the entry point for the application.
//

// TODO remember audio device choices in registry
// TODO code stop button 
// TODO add onclick to waves to set position
// TODO add drag to set selection

#include "stdafx.h"

#pragma comment(lib, "Rtworkq.lib")
#pragma comment(lib, "Comctl32.lib")

namespace MusicStudioCommon {

	HWND CreateButton(HWND hWnd, HINSTANCE hInst, int x, int y, int w, int h, const wchar_t* text, DWORD btnId, DWORD AddlStyles)
	{
		HWND hwndButton = CreateWindow(
			L"BUTTON",  // Predefined class; Unicode assumed 
			text,      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | AddlStyles,  // Styles 
			x,         // x position 
			y,         // y position 
			w,        // Button width
			h,        // Button height
			hWnd,     // Parent window
			(HMENU)btnId,       // No menu.
			hInst,
			NULL);      // Pointer not needed.
		return hwndButton;
	}

}

BOOL InitInstance(int nCmdShow)
{
	HWND hWnd = MainWindow::create_main_window();

	if (!hWnd)
	{
		return FALSE;
	}

	MainWindow::RegisterHotKeys(hWnd);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	AllocConsole();
	FILE* con = nullptr;
	freopen_s(&con, "CONIN$", "r", stdin);
	freopen_s(&con, "CONOUT$", "w", stdout);
	freopen_s(&con, "CONOUT$", "w", stderr);
#endif

    // TODO: Place code here.
	CoInitialize(nullptr);

	// Initialize common controls.
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	MainWindow::initialize_main_window(hInstance);
	TrackControl::initialize_track_window(hInstance);

    // Perform application initialization:
    if (!InitInstance (nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MUSICSTUDIOCX));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	CoUninitialize();

#ifdef _DEBUG
	FreeConsole();
#endif

    return (int) msg.wParam;
}
