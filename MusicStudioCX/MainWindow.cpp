
#include "stdafx.h"

namespace MusicStudioCX
{

	WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
	WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
	LPRTA_DEVICE_INFO lpCaptureDevices = nullptr;
	LPRTA_DEVICE_INFO lpRenderDevices = nullptr;
	LPRTA_DEVICE_INFO CaptureDevInfo = nullptr;
	LPRTA_DEVICE_INFO RenderDevInfo = nullptr;
	HANDLE g_hCaptureThread = nullptr;
	LPCWSTR STATUS_READY = L"Ready";

	HWND hwndMainWindow = nullptr;
	HWND hwndStatus = nullptr;
	HWND ZoomInButton = nullptr;
	HWND ZoomOutButton = nullptr;

	WAVEFORMATEX STD_FORMAT = {
		WAVE_FORMAT_PCM,
		NUM_CHANNELS,
		SAMPLES_PER_SEC,
		AVG_BYTES_PER_SEC,
		BLOCK_ALIGN,
		BITS_PER_SAMPLE,
		0 // extra bytes
	};

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

	HWND CreateButton(HWND hWnd, int x, int y, int w, int h, const wchar_t* text, DWORD btnId)
	{
		HWND hwndButton = CreateWindow(
			L"BUTTON",  // Predefined class; Unicode assumed 
			text,      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			x,         // x position 
			y,         // y position 
			w,        // Button width
			h,        // Button height
			hWnd,     // Parent window
			(HMENU)btnId,       // No menu.
			GetModuleHandle(nullptr),
			NULL);      // Pointer not needed.
		return hwndButton;
	}

	void CaptureDataHandler(BYTE* buffer, UINT32 frameCount, BOOL* Cancel)
	{
		wchar_t msg[256];

		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		if (frameCount < (mctx->max_frames - mctx->frame_offset)) {
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (mctx->tracks[TrackIndex] != nullptr) {
					TrackContext* ctx = MusicStudioCX::get_track_context(mctx->tracks[TrackIndex]);
					FRAME* pFrame = (FRAME*)ctx->buffer;
					memcpy(pFrame + mctx->frame_offset, buffer, frameCount * FRAME_SIZE);
				}
			}
			mctx->frame_offset += frameCount;

			memset(msg, 0, 256 * sizeof(wchar_t));
			swprintf_s(msg, 256, L"Frame Offset %i", mctx->frame_offset);
			SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		}
		else {
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (mctx->tracks[TrackIndex] != nullptr) {
					TrackContext* ctx = MusicStudioCX::get_track_context(mctx->tracks[TrackIndex]);
					FRAME* pFrame = (FRAME*)ctx->buffer;
					memcpy(pFrame + mctx->frame_offset, buffer, (mctx->max_frames - mctx->frame_offset) * FRAME_SIZE);
				}
			}
			mctx->frame_offset += (mctx->max_frames - mctx->frame_offset);

			swprintf_s(msg, 256, L"Done.");
			SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
			*Cancel = TRUE;
		}
	}

	DWORD CaptureThread(LPVOID lpThreadParameter)
	{
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		LPRTA_DEVICE_INFO devInfo = (LPRTA_DEVICE_INFO)lpThreadParameter;
		rta_capture_frames_rtwq(devInfo, nullptr, CaptureDataHandler);

		for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
			if (mctx->tracks[TrackIndex] != nullptr) {
				InvalidateRect(mctx->tracks[TrackIndex], nullptr, TRUE);
			}
		}

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
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		mctx->frame_offset = 0;
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
		MainWindowContext* ctx = nullptr;
		HWND hwndCommand = nullptr;

		switch (message)
		{
		case WM_NCCREATE:
			ctx = (MainWindowContext*)malloc(sizeof(MainWindowContext));
			memset(ctx, 0, sizeof(MainWindowContext));
			ctx->rec_time_seconds = 3;
			ctx->max_frames = SAMPLES_PER_SEC * ctx->rec_time_seconds;
			ctx->zoom_mult = 1;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
			return TRUE;
		case WM_NCDESTROY:
			ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (ctx) free(ctx);
			break;
		case WM_COMMAND:
		{
			ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			hwndCommand = (HWND)lParam;
			if (hwndCommand == ZoomInButton) {
				ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				if (ctx->zoom_mult > 1) ctx->zoom_mult /= 2;
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					if (ctx->tracks[TrackIndex] != nullptr) {
						InvalidateRect(ctx->tracks[TrackIndex], nullptr, TRUE);
					}
				}
			}
			else if (hwndCommand == ZoomOutButton) {
				ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				ctx->zoom_mult *= 2;
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					if (ctx->tracks[TrackIndex] != nullptr) {
						InvalidateRect(ctx->tracks[TrackIndex], nullptr, TRUE);
					}
				}
			}
			else {
				int wmId = LOWORD(wParam);
				// Parse the menu selections:
				switch (wmId)
				{
				case IDM_ABOUT:
					DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
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
			ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (g_hCaptureThread) CloseHandle(g_hCaptureThread);
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (ctx->tracks[TrackIndex] != nullptr) {
					DestroyWindow(ctx->tracks[TrackIndex]);
				}
			}
			if (lpCaptureDevices) rta_free_device_list(lpCaptureDevices);
			if (lpRenderDevices) rta_free_device_list(lpRenderDevices);
			lpCaptureDevices = nullptr;
			lpRenderDevices = nullptr;
			PostQuitMessage(0);
			break;
		case WM_HSCROLL:
			ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			switch (LOWORD(wParam))
			{
			case SB_THUMBTRACK:
				ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				ctx->scroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					if (ctx->tracks[TrackIndex] != nullptr) {
						InvalidateRect(ctx->tracks[TrackIndex], nullptr, TRUE);
					}
				}
				break;
			case SB_THUMBPOSITION:
				SetScrollPos(hWnd, SB_HORZ, HIWORD(wParam), TRUE);
				ctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				ctx->scroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					if (ctx->tracks[TrackIndex] != nullptr) {
						InvalidateRect(ctx->tracks[TrackIndex], nullptr, TRUE);
					}
				}
				break;
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void initialize_main_window()
	{
		WNDCLASSEXW wcex;

		LoadStringW(GetModuleHandle(nullptr), IDC_MUSICSTUDIOCX, szWindowClass, MAX_LOADSTRING);
		LoadStringW(GetModuleHandle(nullptr), IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(MainWindowContext*);
		wcex.hInstance = GetModuleHandle(nullptr);
		wcex.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MUSICSTUDIOCX));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MUSICSTUDIOCX);
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassExW(&wcex);
	}

	HWND create_main_window()
	{
		hwndMainWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
		CreateStatusBar(hwndMainWindow);

		ZoomInButton = CreateButton(hwndMainWindow, 0, 0, 64, 32, L"ZIN", 1);
		ZoomOutButton = CreateButton(hwndMainWindow, 64, 0, 64, 32, L"ZOUT", 2);

		// set scroll bar info
		MainWindowContext* ctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.nMin = 0;
		si.nMax = 65535;
		si.nPos = 0;
		si.fMask = SIF_RANGE | SIF_POS;
		SetScrollInfo(hwndMainWindow, SB_HORZ, &si, TRUE);

		ctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		rta_list_supporting_devices_2(&lpCaptureDevices, &STD_FORMAT);
		CaptureDevInfo = lpCaptureDevices;
		rta_list_supporting_devices_2(&lpRenderDevices, &STD_FORMAT, DEVICE_STATE_ACTIVE, eRender);
		RenderDevInfo = lpRenderDevices;
		ctx->tracks[0] = MusicStudioCX::create_track_window(hwndMainWindow, L"Track1");
		ctx->tracks[1] = MusicStudioCX::create_track_window(hwndMainWindow, L"Track2");

		return hwndMainWindow;
	}

}