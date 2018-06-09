
#include "stdafx.h"

namespace MusicStudioCX
{

	WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
	WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
	LPRTA_DEVICE_INFO lpCaptureDevices = nullptr;
	LPRTA_DEVICE_INFO lpRenderDevices = nullptr;
	HANDLE g_hCaptureThread = nullptr;
	HANDLE g_hRenderThread = nullptr;
	LPCWSTR STATUS_READY = L"Ready";

	HWND hwndMainWindow = nullptr;
	HWND hwndStatus = nullptr;
	HWND ZoomInButton = nullptr;
	HWND ZoomOutButton = nullptr;
	HWND AddTrackButton = nullptr;
	HWND RecButton = nullptr;
	HWND PlayButton = nullptr;
	HWND StopButton = nullptr;

	WAVEFORMATEX StandardFormatInDefault = {
		WAVE_FORMAT_PCM,
		NUM_CHANNELS,
		SAMPLES_PER_SEC,
		AVG_BYTES_PER_SEC,
		BLOCK_ALIGN,
		BITS_PER_SAMPLE,
		0 // extra bytes
	};

	WAVEFORMATEX StandardFormatOutDefault = {
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

	void CaptureDataHandler(BYTE* buffer, UINT32 frameCount, BOOL* Cancel)
	{
		wchar_t msg[256];
		FRAME2CHSHORT *CapturedDataBuffer = (FRAME2CHSHORT*)buffer;

		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		if (frameCount < (mctx->max_frames - mctx->frame_offset)) {
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (mctx->TrackContextList[TrackIndex] != nullptr) {
					if (MusicStudioCX::TrackIsArmed(mctx->TrackContextList[TrackIndex]->state)) {
						for (UINT32 FrameIndex = 0; FrameIndex < frameCount; FrameIndex++) {
							mctx->TrackContextList[TrackIndex]->monobuffershort[mctx->frame_offset + FrameIndex] =
								CapturedDataBuffer[FrameIndex].channel[mctx->TrackContextList[TrackIndex]->InputChannelIndex];
						}
					}
				}
			}
			mctx->frame_offset += frameCount;

			memset(msg, 0, 256 * sizeof(wchar_t));
			swprintf_s(msg, 256, L"Frame Offset %i", mctx->frame_offset);
			SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		}
		else {
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (mctx->TrackContextList[TrackIndex] != nullptr) {
					if (MusicStudioCX::TrackIsArmed(mctx->TrackContextList[TrackIndex]->state)) {
						UINT32 nFrames = mctx->max_frames - mctx->frame_offset;
						for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
							mctx->TrackContextList[TrackIndex]->monobuffershort[mctx->frame_offset + FrameIndex] =
								CapturedDataBuffer[FrameIndex].channel[mctx->TrackContextList[TrackIndex]->InputChannelIndex];
						}
					}
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

		for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < 16; TrackIndex++) {
			if (mctx->TrackContextList[TrackIndex] != nullptr) {
				InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
			}
		}

		return 0;
	}

	void StartRecording()
	{
		wchar_t msg[256];
		memset(msg, 0, 256 * sizeof(wchar_t));
		SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		mctx->frame_offset = 0;
		if (TRUE == rta_initialize_device_2(mctx->CaptureDevInfo, AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
		{
			swprintf_s(msg, 256, L"%i sps %i bps %i ch %zi szflt",
				mctx->CaptureDevInfo->WaveFormat.nSamplesPerSec,
				mctx->CaptureDevInfo->WaveFormat.wBitsPerSample,
				mctx->CaptureDevInfo->WaveFormat.nChannels,
				sizeof(float));
			if (g_hCaptureThread) CloseHandle(g_hCaptureThread);
			g_hCaptureThread = INVALID_HANDLE_VALUE;
			g_hCaptureThread = CreateThread(nullptr, (SIZE_T)0, CaptureThread, (LPVOID)mctx->CaptureDevInfo, 0, nullptr);
		}
	}

	void StopRecording()
	{

	}

	void RenderDataHandler(BYTE* buffer, UINT32 frameCount, BOOL* Cancel)
	{
		wchar_t msg[256];
		FRAME2CHSHORT *CapturedDataBuffer = (FRAME2CHSHORT*)buffer;
		TrackContext* lpTrackCtx = nullptr;
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);	
		// max number of output channels = 2
		float fval[2];

		if (frameCount < (mctx->max_frames - mctx->frame_offset)) {
			UINT32 nFrames = frameCount;
			for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
				memset(fval, 0, sizeof(float) * 2);
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					lpTrackCtx = mctx->TrackContextList[TrackIndex];
					if (lpTrackCtx != nullptr) {
						if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state)) {
							fval[0] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
							fval[1] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
						}
					}
				}
				if (fval[0] > 32767.0f) fval[0] = 32767.0f;
				if (fval[0] < -32767.0f) fval[0] = -32767.0f;
				CapturedDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				CapturedDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}

			mctx->frame_offset += frameCount;

			FRAME2CHSHORT* pOut = (FRAME2CHSHORT*)buffer;
			memset(msg, 0, 256 * sizeof(wchar_t));
			swprintf_s(msg, 256, L"Render Data %i", mctx->frame_offset);
			SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		}
		else {
			UINT32 nFrames = mctx->max_frames - mctx->frame_offset;
			for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
				memset(fval, 0, sizeof(float) * 2);
				for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
					lpTrackCtx = mctx->TrackContextList[TrackIndex];
					if (lpTrackCtx != nullptr) {
						if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state)) {
							fval[0] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
							fval[1] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
						}
					}
				}
				if (fval[0] > 32767.0f) fval[0] = 32767.0f;
				if (fval[0] < -32767.0f) fval[0] = -32767.0f;
				CapturedDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				CapturedDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}

			mctx->frame_offset += nFrames;

			swprintf_s(msg, 256, L"Render Data; Done.");
			SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
			*Cancel = TRUE;
		}
	}

	DWORD RenderThread(LPVOID lpThreadParameter)
	{
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		LPRTA_DEVICE_INFO devInfo = (LPRTA_DEVICE_INFO)lpThreadParameter;
		rta_render_frames_rtwq(devInfo, RenderDataHandler);
		return 0;
	}

	void StartPlayback()
	{
		wchar_t msg[256];
		memset(msg, 0, 256 * sizeof(wchar_t));
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		swprintf_s(msg, 256, L"%i sps %i bps %i ch %zi szflt",
			mctx->RenderDevInfo->WaveFormat.nSamplesPerSec,
			mctx->RenderDevInfo->WaveFormat.wBitsPerSample,
			mctx->RenderDevInfo->WaveFormat.nChannels,
			sizeof(float));
		SendMessage(hwndStatus, SB_SETTEXT, MAKEWORD(0, 0), (LPARAM)msg);
		mctx->frame_offset = 0;
		if (TRUE == rta_initialize_device_2(mctx->RenderDevInfo, AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
		{
			if (g_hRenderThread) CloseHandle(g_hRenderThread);
			g_hRenderThread = INVALID_HANDLE_VALUE;
			g_hRenderThread = CreateThread(nullptr, (SIZE_T)0, RenderThread, (LPVOID)mctx->RenderDevInfo, 0, nullptr);
		}
	}

	void StopPlayback()
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
				MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);

				cwnd = GetDlgItem(hDlg, IDC_CMBINPUTIFX);
				lr = SendMessage(cwnd, CB_GETCURSEL, 0, 0);
				// capture dev info changed
				mctx->CaptureDevInfo = (LPRTA_DEVICE_INFO)SendMessage(cwnd, CB_GETITEMDATA, lr, 0);

				cwnd = GetDlgItem(hDlg, IDC_CMBOUTPUTIFX);
				lr = SendMessage(cwnd, CB_GETCURSEL, 0, 0);
				mctx->RenderDevInfo = (LPRTA_DEVICE_INFO)SendMessage(cwnd, CB_GETITEMDATA, lr, 0);

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

	void DrawTimeBar(HWND hwnd, HDC hdc)
	{
		RECT cr;
		GetClientRect(hwnd, &cr);
		RECT r = { 0 };
		r.left = WVFRM_OFFSET;
		r.top = 32;
		r.right = cr.right;
		r.bottom = 64;
		FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
		COLORREF oldTextColor = SetTextColor(hdc, RGB(255, 255, 255));
		COLORREF oldBackColor = SetBkColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, WVFRM_OFFSET, 32, L"TIME BAR", 8);
		SetTextColor(hdc, oldTextColor);
		SetBkColor(hdc, oldBackColor);
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
		MainWindowContext* mctx = nullptr;
		HWND hwndCommand = nullptr;
		int pos = 0;

		switch (message)
		{
		case WM_NCCREATE:
			mctx = (MainWindowContext*)malloc(sizeof(MainWindowContext));
			memset(mctx, 0, sizeof(MainWindowContext));
			mctx->rec_time_seconds = 5 * 60; // five minutes
			mctx->max_frames = SAMPLES_PER_SEC * mctx->rec_time_seconds;
			mctx->zoom_mult = 1;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)mctx);
			return TRUE;
		case WM_NCDESTROY:
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (mctx) free(mctx);
			break;
		case WM_COMMAND:
		{
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			hwndCommand = (HWND)lParam;
			if (hwndCommand == ZoomInButton) {
				if (mctx->zoom_mult > 1) mctx->zoom_mult /= 2;
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < 16; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
			}
			else if (hwndCommand == ZoomOutButton) {
				mctx->zoom_mult *= 2;
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < 16; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
			}
			else if (hwndCommand == AddTrackButton) {
				MusicStudioCX::create_track_window_a(hWnd, L"New Track", 0);
			}
			else if (hwndCommand == PlayButton) {
				StartPlayback();
			}
			else if (hwndCommand == RecButton) {
				StartRecording();
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
			DrawTimeBar(hWnd, hdc);
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			// TODO
			// if this is running, need to gracefully
			// stop the thread, wait, and close
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (g_hCaptureThread) CloseHandle(g_hCaptureThread);
			if (g_hRenderThread) CloseHandle(g_hRenderThread);
			for (UINT32 TrackIndex = 0; TrackIndex < 16; TrackIndex++) {
				if (mctx->TrackContextList[TrackIndex] != nullptr) {
					DestroyWindow(mctx->TrackContextList[TrackIndex]->TrackWindow);
				}
			}
			if (lpCaptureDevices) rta_free_device_list(lpCaptureDevices);
			if (lpRenderDevices) rta_free_device_list(lpRenderDevices);
			lpCaptureDevices = nullptr;
			lpRenderDevices = nullptr;
			PostQuitMessage(0);
			break;
		case WM_VSCROLL:
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:
			case SB_PAGEUP:
				pos = GetScrollPos(hWnd, SB_VERT);
				if (pos > 0) {
					pos--;
					SetScrollPos(hWnd, SB_VERT, pos, TRUE);
					mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					mctx->vscroll_pos = pos;
					reposition_all_tracks(mctx);
				}
				break;
			case SB_LINEDOWN:
			case SB_PAGEDOWN:
				pos = GetScrollPos(hWnd, SB_VERT);
				if (pos < 15) {
					pos++;
					SetScrollPos(hWnd, SB_VERT, pos, TRUE);
					mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					mctx->vscroll_pos = pos;
					reposition_all_tracks(mctx);
				}
				break;
			case SB_THUMBTRACK:
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				if (mctx->vscroll_pos != HIWORD(wParam))
				{
					mctx->vscroll_pos = HIWORD(wParam);
					reposition_all_tracks(mctx);
				}
				break;
			case SB_THUMBPOSITION:
				SetScrollPos(hWnd, SB_VERT, HIWORD(wParam), TRUE);
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				mctx->vscroll_pos = HIWORD(wParam);
				reposition_all_tracks(mctx);
				break;
			}
			break;
		case WM_HSCROLL:
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			switch (LOWORD(wParam))
			{
			case SB_THUMBTRACK:
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				mctx->hscroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < 16; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				break;
			case SB_THUMBPOSITION:
				SetScrollPos(hWnd, SB_HORZ, HIWORD(wParam), TRUE);
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				mctx->hscroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < 16; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				break;
			}
			break;
		case WM_SIZE:
			SendMessage(hwndStatus, WM_SIZE, wParam, lParam);
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			for (int i = 0; i < 16; i++) {
				if (mctx->TrackContextList[i] != nullptr) 
					SendMessage(mctx->TrackContextList[i]->TrackWindow, WM_SIZE, wParam, lParam);
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

		ZoomInButton = CXCommon::CreateButton(hwndMainWindow, 0, 0, 64, 32, L"ZIN", 1);
		ZoomOutButton = CXCommon::CreateButton(hwndMainWindow, 64, 0, 64, 32, L"ZOUT", 2);
		AddTrackButton = CXCommon::CreateButton(hwndMainWindow, 128, 0, 64, 32, L"NWTR", 3);
		PlayButton = CXCommon::CreateButton(hwndMainWindow, 192, 0, 64, 32, L"PLAY", 3);
		RecButton = CXCommon::CreateButton(hwndMainWindow, 256, 0, 64, 32, L"RECD", 3);
		StopButton = CXCommon::CreateButton(hwndMainWindow, 320, 0, 64, 32, L"STOP", 3);

		// set scroll bar info
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hwndMainWindow, GWLP_USERDATA);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.nMin = 0;
		si.nMax = 65535;
		si.nPos = 0;
		si.fMask = SIF_RANGE | SIF_POS;
		SetScrollInfo(hwndMainWindow, SB_HORZ, &si, TRUE);

		si.nMax = 15;
		si.nPage = 1;
		si.fMask |= SIF_PAGE;
		SetScrollInfo(hwndMainWindow, SB_VERT, &si, TRUE);

		rta_list_supporting_devices_2(&lpCaptureDevices, &StandardFormatInDefault, DEVICE_STATE_ACTIVE, 
			eCapture, AUDCLNT_SHAREMODE_EXCLUSIVE, TRUE);
		mctx->CaptureDevInfo = lpCaptureDevices;

		rta_list_supporting_devices_2(&lpRenderDevices, &StandardFormatOutDefault, DEVICE_STATE_ACTIVE, 
			eRender, AUDCLNT_SHAREMODE_EXCLUSIVE, FALSE);
		mctx->RenderDevInfo = lpRenderDevices;

		wchar_t TrackName[] = L"Track##";
		wchar_t digit = '0';
		TrackContext* prevContext = nullptr;
		for (int i = 0; i < 16; i++) {
			if (i < 9) {
				TrackName[5] = '0';
				TrackName[6] = digit + i + 1;
			}
			else {
				TrackName[5] = '1';
				TrackName[6] = digit + (i - 9);
			}
			TrackContext* ctx = MusicStudioCX::create_track_window_a(hwndMainWindow, TrackName, 0);
			ctx->TrackIndex = i;
			if (prevContext != nullptr) {
				prevContext->NextTrackWindow = ctx->TrackWindow;
				ctx->PrevTrackWindow = prevContext->TrackWindow;
			}
			prevContext = ctx;
		}

		//TrackContext* ctx = MusicStudioCX::create_track_window_a(hwndMainWindow, L"Track1", 0);
		//generate_sine(261.626f, mctx->rec_time_seconds, ctx->monobuffershort, 0.5f);

		//ctx = MusicStudioCX::create_track_window_a(hwndMainWindow, L"Track2", 1);
		//generate_sine(329.628f, mctx->rec_time_seconds, ctx->monobuffershort, 0.5f);

		return hwndMainWindow;
	}

	void reposition_all_tracks(MainWindowContext* mctx)
	{
		UINT32 offset = 0;
		RECT r;
		for (int i = 0; i < 16; i++) {
			TrackContext* ctx = mctx->TrackContextList[i];
			if (ctx->TrackIndex < mctx->vscroll_pos) {
				if(TRUE == IsWindowVisible(ctx->TrackWindow)) ShowWindow(ctx->TrackWindow, SW_HIDE);
			}
			else {
				if (FALSE == IsWindowVisible(ctx->TrackWindow)) ShowWindow(ctx->TrackWindow, SW_SHOW);
				GetClientRect(ctx->TrackWindow, &r);
				int cmdShow = SW_HIDE;
				if (TRUE == ctx->IsMinimized) {
					cmdShow = SW_HIDE;
					SetWindowPos(ctx->TrackWindow, nullptr, 0, offset + MAIN_WINDOW_HEADER_HEIGHT, r.right - r.left, 32, SWP_NOZORDER);
					offset += 32;
				}
				else {
					cmdShow = SW_SHOW;
					SetWindowPos(ctx->TrackWindow, nullptr, 0, offset + MAIN_WINDOW_HEADER_HEIGHT, r.right - r.left, 128, SWP_NOZORDER);
					offset += 128;
				}
				ShowWindow(ctx->buttons[0], cmdShow);
				ShowWindow(ctx->buttons[1], cmdShow);
				ShowWindow(ctx->buttons[2], cmdShow);
				InvalidateRect(ctx->TrackWindow, nullptr, FALSE);
			}
		}
	}

}