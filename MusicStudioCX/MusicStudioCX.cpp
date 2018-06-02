// MusicStudioCX.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MusicStudioCX.h"

#pragma comment(lib, "Rtworkq.lib")
#pragma comment(lib, "Comctl32.lib")

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd = MusicStudioCX::create_main_window();

	if (!hWnd)
	{
		return FALSE;
	}

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

    // TODO: Place code here.
	CoInitialize(nullptr);

	MusicStudioCX::initialize_main_window();
	MusicStudioCX::initialize_track_window();

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
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

    return (int) msg.wParam;
}
