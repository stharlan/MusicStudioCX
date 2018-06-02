// MusicStudioCX.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MusicStudioCX.h"

#pragma comment(lib, "Rtworkq.lib")
#pragma comment(lib, "Comctl32.lib")

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hwndMainWindow = nullptr;
HWND hwndStatus = nullptr;
HWND HwndTrackTest = nullptr;

LPCWSTR STATUS_READY = L"Ready";

LPRTA_DEVICE_INFO lpCaptureDevices = nullptr;
LPRTA_DEVICE_INFO lpRenderDevices = nullptr;
LPRTA_DEVICE_INFO CaptureDevInfo = nullptr;
LPRTA_DEVICE_INFO RenderDevInfo = nullptr;
HANDLE g_hCaptureThread = nullptr;

WAVEFORMATEX STD_FORMAT = {
	WAVE_FORMAT_PCM,
	NUM_CHANNELS,
	SAMPLES_PER_SEC,
	AVG_BYTES_PER_SEC,
	BLOCK_ALIGN,
	BITS_PER_SAMPLE,
	0 // extra bytes
};

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SetupDlgProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	CoInitialize(nullptr);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MUSICSTUDIOCX, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

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



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

	MusicStudioCX::initialize_track_window();

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MUSICSTUDIOCX));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MUSICSTUDIOCX);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void CreateStatusBar(HWND hwndParent)
{
	InitCommonControls();
	 
	hwndStatus = CreateWindowEx(
		0,						// no extended styles
		STATUSCLASSNAME,        // name of status bar class
		(PCTSTR)NULL,           // no text when first created
		SBARS_SIZEGRIP |        // includes a sizing grip
		WS_CHILD | WS_VISIBLE,  // creates a visible child window
		0, 0, 0, 0,             // ignores size and position
		hwndParent,             // handle to parent window
		(HMENU)STATUS_BAR_ID,	// child window identifier
		GetModuleHandle(nullptr),	// handle to application instance
		NULL);                  // no window creation data

	int PartRight = -1;
	SendMessage(hwndStatus, SB_SETPARTS, (WPARAM)1, (LPARAM)&PartRight);
	SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)STATUS_READY);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // before show window
   CreateStatusBar(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   hwndMainWindow = hWnd;

   return TRUE;
}

void CaptureDataHandler(BYTE* buffer, UINT32 frameCount, BOOL* Cancel)
{
	wchar_t msg[256];

	TrackContext* ctx = (TrackContext*)GetWindowLongPtr(HwndTrackTest, GWLP_USERDATA);
	if (frameCount < (ctx->max_frames - ctx->frame_offset)) {
		FRAME* pFrame = (FRAME*)ctx->buffer;
		memcpy(pFrame + ctx->frame_offset, buffer, frameCount * FRAME_SIZE);
		ctx->frame_offset += frameCount;

		memset(msg, 0, 256 * sizeof(wchar_t));
		swprintf_s(msg, 256, L"Frame Offset %i", ctx->frame_offset);
		SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
	}
	else {
		swprintf_s(msg, 256, L"Done.");
		SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		*Cancel = TRUE;
	}
}

DWORD CaptureThread(LPVOID lpThreadParameter)
{
	LPRTA_DEVICE_INFO devInfo = (LPRTA_DEVICE_INFO)lpThreadParameter;
	rta_capture_frames_rtwq(devInfo, nullptr, CaptureDataHandler);

	InvalidateRect(HwndTrackTest, nullptr, TRUE);

	//std::ofstream wave_dbg_out("c:\\temp\\wave_debug_out.txt");
	//TrackContext* ctx = (TrackContext*)GetWindowLongPtr(HwndTrackTest, GWLP_USERDATA);
	//FRAME* pFrame = (FRAME*)ctx->buffer;
	//for (UINT32 FrameCount = 0; FrameCount < 500; FrameCount++) {
		//wave_dbg_out << "left: " << pFrame[FrameCount].left
			//<< "; right: " << pFrame[FrameCount].right << std::endl;
	//}

	return 0;
}

void StartRecording()
{
	wchar_t msg[256];
	memset(msg, 0, 256 * sizeof(wchar_t));
	swprintf_s(msg, 256, L"%i sps %i bps %i ch %zi szflt", 
		STD_FORMAT.nSamplesPerSec,
		STD_FORMAT.wBitsPerSample,
		STD_FORMAT.nChannels,
		sizeof(float));
	SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
	TrackContext* ctx = (TrackContext*)GetWindowLongPtr(HwndTrackTest, GWLP_USERDATA);
	ctx->frame_offset = 0;
	if (TRUE == rta_initialize_device_2(CaptureDevInfo, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, &STD_FORMAT))
	{
		if (g_hCaptureThread) CloseHandle(g_hCaptureThread);
		g_hCaptureThread = INVALID_HANDLE_VALUE;
		g_hCaptureThread = CreateThread(nullptr, (SIZE_T)0, CaptureThread, (LPVOID)CaptureDevInfo, 0, nullptr);
	}
 }

void StopRecording()
{

}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		rta_list_supporting_devices_2(&lpCaptureDevices, &STD_FORMAT);
		CaptureDevInfo = lpCaptureDevices;
		rta_list_supporting_devices_2(&lpRenderDevices, &STD_FORMAT, DEVICE_STATE_ACTIVE, eRender);
		RenderDevInfo = lpRenderDevices;
		HwndTrackTest = MusicStudioCX::create_track_window(hWnd, L"Track1");
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case ID_FILE_SETUP:
				DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_SETUPDLG), hWnd, SetupDlgProc);
				break;
			case ID_TEST_START:
				StartRecording();
				break;
			case ID_TEST_STOP:
				//StopRecording();
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		// TODO
		// if this is running, need to gracefully
		// stop the thread, wait, and close
		if (g_hCaptureThread) CloseHandle(g_hCaptureThread);
		DestroyWindow(HwndTrackTest);
		if (lpCaptureDevices) rta_free_device_list(lpCaptureDevices);
		if (lpRenderDevices) rta_free_device_list(lpRenderDevices);
		lpCaptureDevices = nullptr;
		lpRenderDevices = nullptr;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void PopulateDeviceDropdown(EDataFlow dataFlow, HWND hDlg)
{
	HWND cwnd = nullptr;
	UINT DevCtr = 0;
	LPRTA_DEVICE_INFO lpDevices = nullptr;

	if (dataFlow == EDataFlow::eCapture) {
		cwnd = GetDlgItem(hDlg, IDC_CMBINPUTIFX);
		if (!cwnd) return;
		lpDevices = lpCaptureDevices;
	}
	else {
		cwnd = GetDlgItem(hDlg, IDC_CMBOUTPUTIFX);
		if (!cwnd) return;
		lpDevices = lpRenderDevices;
	}

	if (lpDevices != nullptr) {
		LPRTA_DEVICE_INFO lpThis = lpDevices;
		while (lpThis != nullptr) {
			SendMessage(cwnd, CB_ADDSTRING, (WPARAM)0, (LPARAM)lpThis->DeviceName);
			SendMessage(cwnd, CB_SETITEMDATA, (WPARAM)DevCtr++, (LPARAM)lpThis);
			lpThis = (LPRTA_DEVICE_INFO)lpThis->pNext;
		}
		SendMessage(cwnd, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	}
}

INT_PTR CALLBACK SetupDlgProc(
	_In_ HWND   hDlg,
	_In_ UINT   message,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	LRESULT lr = 0;
	HWND cwnd = nullptr;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		PopulateDeviceDropdown(EDataFlow::eCapture, hDlg);
		PopulateDeviceDropdown(EDataFlow::eRender, hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			cwnd = GetDlgItem(hDlg, IDC_CMBINPUTIFX);
			lr = SendMessage(cwnd, CB_GETCURSEL, 0, 0);
			CaptureDevInfo = (LPRTA_DEVICE_INFO)SendMessage(cwnd, CB_GETITEMDATA, lr, 0);
			cwnd = GetDlgItem(hDlg, IDC_CMBOUTPUTIFX);
			lr = SendMessage(cwnd, CB_GETCURSEL, 0, 0);
			RenderDevInfo = (LPRTA_DEVICE_INFO)SendMessage(cwnd, CB_GETITEMDATA, lr, 0);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
