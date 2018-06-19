
#include "stdafx.h"

#define BTN_ZOOM_IN 30000
#define BTN_ZOOM_OUT 30001
#define BTN_REC 30002
#define BTN_PLAY 30003
#define BTN_STOP 30004
#define ID_SB_PROGRESS_BAR 30005
#define ID_STATIC_STATUS 30006

namespace MusicStudioCX
{

	WCHAR m_szWindowClass[MAX_LOADSTRING];            // the main window class name
	WCHAR m_szTitle[MAX_LOADSTRING];                  // The title bar text
	LPRTA_DEVICE_INFO m_lpCaptureDevices = nullptr;
	LPRTA_DEVICE_INFO m_lpRenderDevices = nullptr;
	HANDLE m_hCaptureThread = nullptr;
	HANDLE m_hRenderThread = nullptr;
	LPCWSTR m_STATUS_READY = L"Ready";

	HWND m_hwndMainWindow = nullptr;
	HWND m_hwndStaticStatus = nullptr;
	HWND m_hwndProgBar = nullptr;

	BOOL m_TriggerStop = FALSE;

	WAVEFORMATEX m_StandardFormatInDefault = {
		WAVE_FORMAT_PCM,
		NUM_CHANNELS,
		SAMPLES_PER_SEC,
		AVG_BYTES_PER_SEC,
		BLOCK_ALIGN,
		BITS_PER_SAMPLE,
		0 // extra bytes
	};

	WAVEFORMATEX m_StandardFormatOutDefault = {
		WAVE_FORMAT_PCM,
		NUM_CHANNELS,
		SAMPLES_PER_SEC,
		AVG_BYTES_PER_SEC,
		BLOCK_ALIGN,
		BITS_PER_SAMPLE,
		0 // extra bytes
	};

	void RedrawTimeBar();

	/*
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
		SetWindowPos(hwndStatus, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		
		int iBarWidths[] = { 120, 285, -1 };
		//static HWND StatusBar, hProgressBar;

		// Creating a Status Bar 
		//hStatusBar = CreateWindowEx(NULL, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		//	hwnd, (HMENU)IDD_STATUSBAR, hInst, NULL);
		SendMessage(hwndStatus, SB_SETPARTS, 3, (LPARAM)iBarWidths);
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)STATUS_READY);
		//SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "some text 2");

		// Creating and place the  Progress Bar inside the StatusBar 
		HWND hpb = CreateWindowEx(NULL, L"msctls_progress32", NULL, WS_CHILD | WS_VISIBLE, 122, 2, 163, 18,
			hwndStatus, (HMENU)ID_SB_PROGRESS_BAR, GetModuleHandle(nullptr), NULL);
		SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
		
	}
	*/

	void StartNewProject()
	{
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);

		mctx->auto_position_timebar = false;
		mctx->frame_offset = 0;
		mctx->hscroll_pos = 0;
		mctx->max_frames = SAMPLES_PER_SEC * REC_TIME_SECONDS;
		mctx->ProjectDir = std::wstring();
		mctx->rec_time_seconds = REC_TIME_SECONDS;
		mctx->vscroll_pos = 0;
		memset(mctx->WavFileName, 0, 1024 * sizeof(wchar_t));
		mctx->zoom_mult = 256;

		for (int t = 0; t < 16; t++) {
			TrackContext* tctx = mctx->TrackContextList[t];
			tctx->InputChannelIndex = 0;
			tctx->IsMinimized = FALSE;
			tctx->leftpan = 1.0f;
			tctx->rightpan = 1.0f;
			tctx->state = 0;
			tctx->volume = 1.0f;
			memset(tctx->monobuffershort, 0, SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
		}

		SetScrollPos(m_hwndMainWindow, SB_HORZ, 0, TRUE);
		SetScrollPos(m_hwndMainWindow, SB_VERT, 0, TRUE);
		RedrawTimeBar();
		reposition_all_tracks(mctx);
		for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
			InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
		}

	}

	bool GetFolder(std::wstring& folderpath,
		const wchar_t* szCaption = NULL,
		HWND hOwner = NULL)
	{
		bool retVal = false;

		// The BROWSEINFO struct tells the shell 
		// how it should display the dialog.
		BROWSEINFO bi;
		memset(&bi, 0, sizeof(bi));

		bi.ulFlags = BIF_USENEWUI;
		bi.hwndOwner = hOwner;
		bi.lpszTitle = szCaption;

		// must call this if using BIF_USENEWUI
		::OleInitialize(NULL);

		// Show the dialog and get the itemIDList for the 
		// selected folder.
		LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);

		if (pIDL != NULL)
		{
			// Create a buffer to store the path, then 
			// get the path.
			wchar_t buffer[_MAX_PATH] = { '\0' };
			if (::SHGetPathFromIDList(pIDL, buffer) != 0)
			{
				// Set the string value.
				folderpath = std::wstring(buffer);
				retVal = true;
			}

			// free the item id list
			CoTaskMemFree(pIDL);
		}

		::OleUninitialize();

		return retVal;
	}

	void CaptureDataHandler(HANDLER_CONTEXT* lpHandlerContext, BOOL* lpCancel)
	{

		// get data from the capBuffer and put it in the track

		wchar_t msg[256];
		FRAME2CHSHORT *CapturedDataBuffer = (FRAME2CHSHORT*)(lpHandlerContext->capBuffer);
		FRAME2CHSHORT *RenderingDataBuffer = (FRAME2CHSHORT*)(lpHandlerContext->renBuffer);
		float fval[2];
		TrackContext* lpTrackCtx = nullptr;
		UINT32 CaptureFrameIndexAdjusted = 0;

		if (TRUE == m_TriggerStop) {
			*lpCancel = TRUE;
			return;
		}

		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		UINT32 TotalLastFrameCount = (
			lpHandlerContext->LastFrameCounts[0] + 
			lpHandlerContext->LastFrameCounts[1] + 
			lpHandlerContext->LastFrameCounts[2]);
		TrackContext** trackPtrs = mctx->TrackContextList;
		if (lpHandlerContext->frameCount < (mctx->max_frames - mctx->frame_offset)) {

			for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
				if (trackPtrs[TrackIndex] != nullptr) {
					if (MusicStudioCX::TrackIsArmed(trackPtrs[TrackIndex]->state)) {
						// record to this track
						for (UINT32 FrameIndex = 0; FrameIndex < lpHandlerContext->frameCount; FrameIndex++) {
							// start with the frame offset
							// add the frame index
							// subtract the last frame count to bring it inline with the rendering
							CaptureFrameIndexAdjusted = mctx->frame_offset + FrameIndex - TotalLastFrameCount;
							trackPtrs[TrackIndex]->monobuffershort[CaptureFrameIndexAdjusted] =
								CapturedDataBuffer[FrameIndex].channel[trackPtrs[TrackIndex]->InputChannelIndex];
						}
					}
				}
			}

			// begin render
			for (UINT32 FrameIndex = 0; FrameIndex < lpHandlerContext->frameCount; FrameIndex++) {
				fval[0] = 0.0f;
				fval[1] = 0.0f;
				for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
					lpTrackCtx = trackPtrs[TrackIndex];
					if (lpTrackCtx != nullptr) {
						if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state) &&
							FALSE == MusicStudioCX::TrackIsArmed(trackPtrs[TrackIndex]->state)) {
							fval[0] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
							fval[1] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
						}
					}
				}
				if (fval[0] > 32767.0f) fval[0] = 32767.0f;
				if (fval[0] < -32767.0f) fval[0] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}
			// end render

			mctx->frame_offset += lpHandlerContext->frameCount;

			memset(msg, 0, 256 * sizeof(wchar_t));
			UINT32 mframe = mctx->frame_offset % 48;
			UINT32 mmsec = mctx->frame_offset / 48;
			UINT32 msec = mmsec / 1000;
			UINT32 mmin = msec / 60;
			mmsec = mmsec % 1000;
			msec = msec % 60;
			swprintf_s(msg, 256, L"%i min %i sec %i ms %i frm", mmin, msec, mmsec, mframe);
			SetWindowText(m_hwndStaticStatus, msg);
			mctx->auto_position_timebar = TRUE;
			RedrawTimeBar();
		}
		else {
			UINT32 nFrames = mctx->max_frames - mctx->frame_offset;

			for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
				if (trackPtrs[TrackIndex] != nullptr) {
					if (MusicStudioCX::TrackIsArmed(trackPtrs[TrackIndex]->state)) {
						for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
							CaptureFrameIndexAdjusted = mctx->frame_offset + FrameIndex - TotalLastFrameCount;
							trackPtrs[TrackIndex]->monobuffershort[CaptureFrameIndexAdjusted] =
								CapturedDataBuffer[FrameIndex].channel[trackPtrs[TrackIndex]->InputChannelIndex];
						}
					}
				}
			}

			// begin render
			for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
				fval[0] = 0.0f;
				fval[1] = 0.0f;
				for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
					lpTrackCtx = trackPtrs[TrackIndex];
					if (lpTrackCtx != nullptr) {
						if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state) &&
							FALSE == MusicStudioCX::TrackIsArmed(trackPtrs[TrackIndex]->state)) {
							fval[0] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
							fval[1] += (float)lpTrackCtx->monobuffershort[mctx->frame_offset + FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
						}
					}
				}
				if (fval[0] > 32767.0f) fval[0] = 32767.0f;
				if (fval[0] < -32767.0f) fval[0] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}
			// end render

			mctx->frame_offset += nFrames;

			swprintf_s(msg, 256, L"Done.");
			SetWindowText(m_hwndStaticStatus, msg);
			*lpCancel = TRUE;
		}
	}

	DWORD CaptureThread(LPVOID lpThreadParameter)
	{
		MainWindowContext* mctx = (MainWindowContext*)lpThreadParameter;
		rta_capture_frames_rtwq(mctx->CaptureDevInfo, mctx->RenderDevInfo, CaptureDataHandler);

		for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
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
		SetWindowText(m_hwndStaticStatus, msg);
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		mctx->frame_offset = 0;
		BOOL isCapOk = rta_initialize_device_2(mctx->CaptureDevInfo, AUDCLNT_STREAMFLAGS_EVENTCALLBACK);
		BOOL isRenOk = rta_initialize_device_2(mctx->RenderDevInfo, 0);
		if (isCapOk && isRenOk)
		{
			swprintf_s(msg, 256, L"%i sps %i bps %i ch %zi szflt",
				mctx->CaptureDevInfo->WaveFormat.nSamplesPerSec,
				mctx->CaptureDevInfo->WaveFormat.wBitsPerSample,
				mctx->CaptureDevInfo->WaveFormat.nChannels,
				sizeof(float));
			if (m_hCaptureThread) CloseHandle(m_hCaptureThread);
			m_hCaptureThread = INVALID_HANDLE_VALUE;
			m_TriggerStop = FALSE;
			m_hCaptureThread = CreateThread(nullptr, (SIZE_T)0, CaptureThread, (LPVOID)mctx, 0, nullptr);
		}	
	}

	void RenderDataHandler(HANDLER_CONTEXT* lpHandlerContext, BOOL* lpCancel)
	{
		wchar_t msg[256];
		FRAME2CHSHORT *RenderingDataBuffer = (FRAME2CHSHORT*)(lpHandlerContext->renBuffer);
		TrackContext* lpTrackCtx = nullptr;
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		// max number of output channels = 2
		float fval[2];

		if (TRUE == m_TriggerStop) {
			*lpCancel = TRUE;
			return;
		}

		if (lpHandlerContext->frameCount < (mctx->max_frames - mctx->frame_offset)) {
			UINT32 nFrames = lpHandlerContext->frameCount;
			for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
				fval[0] = 0.0f;
				fval[1] = 0.0f;
				for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
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
				RenderingDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}

			mctx->frame_offset += lpHandlerContext->frameCount;

			memset(msg, 0, 256 * sizeof(wchar_t));
			UINT32 mframe = mctx->frame_offset % 48;
			UINT32 mmsec = mctx->frame_offset / 48;
			UINT32 msec = mmsec / 1000;
			UINT32 mmin = msec / 60;
			mmsec = mmsec % 1000;
			msec = msec % 60;
			swprintf_s(msg, 256, L"%i min %i sec %i ms %i frm", mmin, msec, mmsec, mframe);
			SetWindowText(m_hwndStaticStatus, msg);
			mctx->auto_position_timebar = TRUE;
			RedrawTimeBar();
		}
		else {
			UINT32 nFrames = mctx->max_frames - mctx->frame_offset;
			for (UINT32 FrameIndex = 0; FrameIndex < nFrames; FrameIndex++) {
				fval[0] = 0.0f;
				fval[1] = 0.0f;
				for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
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
				RenderingDataBuffer[FrameIndex].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				RenderingDataBuffer[FrameIndex].channel[1] = (short)fval[1];
			}

			mctx->frame_offset += nFrames;

			swprintf_s(msg, 256, L"Render Data; Done.");
			SetWindowText(m_hwndStaticStatus, msg);
			*lpCancel = TRUE;
		}
	}

	DWORD RenderThread(LPVOID lpThreadParameter)
	{
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		LPRTA_DEVICE_INFO devInfo = (LPRTA_DEVICE_INFO)lpThreadParameter;
		rta_render_frames_rtwq(devInfo, RenderDataHandler);
		return 0;
	}

	void StartPlayback()
	{
		wchar_t msg[256];
		memset(msg, 0, 256 * sizeof(wchar_t));
		MainWindowContext *mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		swprintf_s(msg, 256, L"%i sps %i bps %i ch %zi szflt",
			mctx->RenderDevInfo->WaveFormat.nSamplesPerSec,
			mctx->RenderDevInfo->WaveFormat.wBitsPerSample,
			mctx->RenderDevInfo->WaveFormat.nChannels,
			sizeof(float));
		SetWindowText(m_hwndStaticStatus, msg);
		mctx->frame_offset = 0;
		if (TRUE == rta_initialize_device_2(mctx->RenderDevInfo, AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
		{
			if (m_hRenderThread) CloseHandle(m_hRenderThread);
			m_hRenderThread = INVALID_HANDLE_VALUE;
			m_TriggerStop = FALSE;
			m_hRenderThread = CreateThread(nullptr, (SIZE_T)0, RenderThread, (LPVOID)mctx->RenderDevInfo, 0, nullptr);
		}
	}

	void PopulateDeviceDropdown(EDataFlow dataFlow, HWND hDlg)
	{
		HWND cwnd = nullptr;
		UINT DevCtr = 0;
		LPRTA_DEVICE_INFO lpDevices = nullptr;

		if (dataFlow == EDataFlow::eCapture) {
			cwnd = GetDlgItem(hDlg, IDC_CMBINPUTIFX);
			if (!cwnd) return;
			lpDevices = m_lpCaptureDevices;
		}
		else {
			cwnd = GetDlgItem(hDlg, IDC_CMBOUTPUTIFX);
			if (!cwnd) return;
			lpDevices = m_lpRenderDevices;
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
				MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);

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

	void RedrawTimeBar()
	{
		RECT cr;
		GetClientRect(m_hwndMainWindow, &cr);
		RECT r = { 0 };
		r.left = WVFRM_OFFSET;
		r.top = 32;
		r.right = cr.right;
		r.bottom = 64;
		InvalidateRect(m_hwndMainWindow, &r, FALSE);
	}

	void DrawTimeBar(HWND hwnd, HDC hdc, MainWindowContext* mctx)
	{
		wchar_t timeval[16];
		RECT cr;
		GetClientRect(hwnd, &cr);
		RECT r = { 0 };
		r.left = WVFRM_OFFSET;
		r.top = 32;
		r.right = cr.right;
		r.bottom = 64;
		FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

		// @1024 every 2 sec
		UINT32 fpt = 48000;
		UINT32 secMult = 1;
		UINT32 secDiv = 1;
		if (mctx->zoom_mult > 8192)
		{
			secMult = 30;
		}
		else if (mctx->zoom_mult > 4096)
		{
			secMult = 10;
		}
		else if (mctx->zoom_mult > 2048)
		{
			secMult = 5;
		}
		else if (mctx->zoom_mult > 512)
		{
			secMult = 2;
		}
		else if (mctx->zoom_mult > 128) 
		{
			// do nothing
		}
		else if (mctx->zoom_mult > 64) 
		{
			secDiv = 2;
		}
		else if (mctx->zoom_mult > 32)
		{
			secDiv = 5;
		}
		else if (mctx->zoom_mult > 16)
		{
			secDiv = 10;
		}
		else if (mctx->zoom_mult > 8)
		{
			secDiv = 20;
		}
		else if (mctx->zoom_mult > 4)
		{
			secDiv = 50;
		}
		else if (mctx->zoom_mult > 2)
		{
			secDiv = 100;
		}
		else if (mctx->zoom_mult > 1)
		{
			secDiv = 200;
		}
		else
		{
			secDiv = 500;
		}
		fpt = MulDiv(fpt, secMult, secDiv);

		UINT32 StartFrame = (UINT32)((mctx->hscroll_pos / 65535.0f) * (float)mctx->max_frames);
		UINT32 StartSecond = StartFrame / (UINT32)fpt;
		if (StartFrame % fpt > 0) StartSecond++;

		UINT32 TotalFrames = mctx->zoom_mult * (cr.right - WVFRM_OFFSET);
		UINT32 EndFrame = StartFrame + TotalFrames;
		UINT32 EndSecond = EndFrame / (UINT32)fpt;
		EndSecond++;
		
		COLORREF oldTextColor = SetTextColor(hdc, RGB(255, 255, 255));
		COLORREF oldBackColor = SetBkColor(hdc, RGB(0, 0, 0));
		HPEN WhitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		HGDIOBJ oldPen = SelectObject(hdc, WhitePen);
		RECT tr;
		
		for (UINT32 sec = StartSecond; sec < EndSecond; sec++) {
			UINT32 pos = (((sec * fpt) - StartFrame) / mctx->zoom_mult) + WVFRM_OFFSET;
			MoveToEx(hdc, pos, 48, nullptr);
			LineTo(hdc, pos, 64);
			tr.left = pos;
			tr.right = pos + (fpt / mctx->zoom_mult);
			tr.top = 32;
			tr.bottom = 48;
			float time = (float)sec * (float)secMult / (float)secDiv;
			if (mctx->zoom_mult == 1) {
				wsprintf(timeval, L"%if", sec * 96);
				DrawText(hdc, timeval, (int)wcslen(timeval), &tr, DT_TOP | DT_LEFT);
			}
			else if (secDiv > 1) {
				time *= 1000;
				wsprintf(timeval, L"%ims", (int)time);
				DrawText(hdc, timeval, (int)wcslen(timeval), &tr, DT_TOP | DT_LEFT);
			}
			else {
				wsprintf(timeval, L"%is", (int)time);
				DrawText(hdc, timeval, (int)wcslen(timeval), &tr, DT_TOP | DT_LEFT);
			}
		}

		//for (UINT32 i = 0; i < cr.right - WVFRM_OFFSET; i++) {
		//	UINT32 FrameIndex = StartFrame + (i * mctx->zoom_mult);
		//	if (FrameIndex % 48000 == 0) {
		//		MoveToEx(hdc, i + WVFRM_OFFSET, 48, nullptr);
		//		LineTo(hdc, i + WVFRM_OFFSET, 64);
		//		tr.left = i + WVFRM_OFFSET;
		//		tr.right = i + WVFRM_OFFSET + 96;
		//		tr.top = 32;
		//		tr.bottom = 48;
		//		memset(timeval, 0, 16 * sizeof(wchar_t));
		//		UINT32 s = FrameIndex / 48000;
		//		wsprintf(timeval, L"%i s", s);
		//		DrawText(hdc, timeval, wcslen(timeval), &tr, DT_TOP | DT_LEFT);
		//	}
		//}
		SetTextColor(hdc, oldTextColor);
		SetBkColor(hdc, oldBackColor);

		if (mctx->frame_offset > StartFrame && mctx->frame_offset < EndFrame) {
			// draw a marker
			UINT32 pos = ((mctx->frame_offset - StartFrame) / mctx->zoom_mult) + WVFRM_OFFSET;
			MoveToEx(hdc, pos, 48, nullptr);
			LineTo(hdc, pos, 64);
		}

		SelectObject(hdc, oldPen);
		DeleteObject(WhitePen);

		// if we're past the end of the window
		// adjust the scoll pos and redraw tracks
		if (TRUE == mctx->auto_position_timebar) {
			if (mctx->frame_offset < StartFrame || mctx->frame_offset > EndFrame)
			{
				mctx->hscroll_pos = MulDiv(mctx->frame_offset, 65535, mctx->max_frames);
				SetScrollPos(m_hwndMainWindow, SB_HORZ, mctx->hscroll_pos, TRUE);
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
			}
		}
	}

	BOOL WriteABlockOfFrames(FRAME2CHSHORT* PointerToFrames, UINT32 FramesToWrite, HANDLE hFile)
	{
		DWORD nbw = 0;

		// write the frames
		UINT32 DataSizeToWrite = FramesToWrite * sizeof(FRAME2CHSHORT);
		BOOL result = WriteFile(hFile, (void*)(PointerToFrames),
			DataSizeToWrite, &nbw, nullptr);

		if (result == FALSE) {
			DWORD err = GetLastError();
			if (ERROR_INVALID_USER_BUFFER == err) {
				MessageBox(nullptr, L"invalid user buffer", L"ERROR", MB_OK);
				return FALSE;
			}
			else {
				MessageBox(nullptr, L"failed to write", L"ERROR", MB_OK);
				return FALSE;
			}
		}
		else if (nbw < DataSizeToWrite) {
			MessageBox(nullptr, L"not all bytes written", L"ERROR", MB_OK);
			return FALSE;
		}

		return TRUE;
	}

	// assumes 2 channel 16 bit
	void WriteWaveFile(LPCWSTR WaveFileName, UINT32 RecordingTimeSeconds, UINT32 SamplesPerSec, 
		TrackContext** lppTca, int TrackId = -1)
	{

		wprintf(L"writing %s\n", WaveFileName);

		DWORD nbw = 0;
		UINT32 SampleSize = sizeof(short);
		UINT32 NumberOfChannels = 2;

		HANDLE hFile = CreateFile(WaveFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (hFile != nullptr) {

			SetWindowText(m_hwndStaticStatus, L"Saving...");

			// write a wav file
			UINT32 DataSize =
				RecordingTimeSeconds *	// secs of rec time
				SamplesPerSec *			// samples per sec
				SampleSize *			// size of a single sample
				NumberOfChannels;		// channels

			WriteFile(hFile, "RIFF", 4, &nbw, nullptr);

			// chunk size
			// 36 + subchunk2size
			// subchunk2size = samples * channels * bitspersample / 8
			UINT32 val32 = 36 + DataSize;
			WriteFile(hFile, &val32, sizeof(UINT32), &nbw, nullptr);

			// format
			WriteFile(hFile, "WAVE", 4, &nbw, nullptr);

			// sub chunk id
			WriteFile(hFile, "fmt ", 4, &nbw, nullptr);

			// sub chunk size
			val32 = 16;
			WriteFile(hFile, &val32, sizeof(UINT32), &nbw, nullptr);

			// audio format = 1
			UINT16 val16 = 1;
			WriteFile(hFile, &val16, sizeof(UINT16), &nbw, nullptr);

			// channels
			val16 = NumberOfChannels;
			WriteFile(hFile, &val16, sizeof(UINT16), &nbw, nullptr);

			// sample rate
			val32 = SamplesPerSec;
			WriteFile(hFile, &val32, sizeof(UINT32), &nbw, nullptr);

			// byte rate
			val32 = SamplesPerSec * SampleSize * NumberOfChannels;
			WriteFile(hFile, &val32, sizeof(UINT32), &nbw, nullptr);

			// block align = channels * bits per sample / 8
			val16 = SampleSize * NumberOfChannels;
			WriteFile(hFile, &val16, sizeof(UINT16), &nbw, nullptr);

			// bits per sample
			val16 = SampleSize * 8;
			WriteFile(hFile, &val16, sizeof(UINT16), &nbw, nullptr);

			// sub chunk id
			WriteFile(hFile, "data", 4, &nbw, nullptr);

			// data size
			WriteFile(hFile, &DataSize, sizeof(UINT32), &nbw, nullptr);

			float fval[2];
			FRAME2CHSHORT* FramesToWrite = (FRAME2CHSHORT*)malloc(SamplesPerSec * sizeof(FRAME2CHSHORT));
			UINT32 FrameCounter = 0;
			TrackContext* lpTrackCtx = nullptr;
			for (UINT32 FrameIndex = 0; FrameIndex < SamplesPerSec * RecordingTimeSeconds; FrameIndex++) {
				memset(fval, 0, sizeof(float) * 2);
				if (TrackId > -1) {
					lpTrackCtx = lppTca[TrackId];
					if (lpTrackCtx != nullptr) {
						if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state)) {
							fval[0] += (float)lpTrackCtx->monobuffershort[FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
							fval[1] += (float)lpTrackCtx->monobuffershort[FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
						}
					}
				}
				else {
					for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
						lpTrackCtx = lppTca[TrackIndex];
						if (lpTrackCtx != nullptr) {
							if (FALSE == MusicStudioCX::TrackIsMute(lpTrackCtx->state)) {
								fval[0] += (float)lpTrackCtx->monobuffershort[FrameIndex] * lpTrackCtx->volume * lpTrackCtx->leftpan;
								fval[1] += (float)lpTrackCtx->monobuffershort[FrameIndex] * lpTrackCtx->volume * lpTrackCtx->rightpan;
							}
						}
					}
				}
				if (fval[0] > 32767.0f) fval[0] = 32767.0f;
				if (fval[0] < -32767.0f) fval[0] = -32767.0f;
				FramesToWrite[FrameCounter].channel[0] = (short)fval[0];
				if (fval[1] > 32767.0f) fval[1] = 32767.0f;
				if (fval[1] < -32767.0f) fval[1] = -32767.0f;
				FramesToWrite[FrameCounter].channel[1] = (short)fval[1];

				FrameCounter++;
				if (FrameCounter == SamplesPerSec) {
					if (FALSE == WriteABlockOfFrames(FramesToWrite, FrameCounter, hFile))
						goto done;
					FrameCounter = 0;
					SendMessage(m_hwndProgBar, PBM_SETPOS, MulDiv(FrameIndex, 100, SamplesPerSec * RecordingTimeSeconds), 0);
				}
			}

		done:
			SendMessage(m_hwndProgBar, PBM_SETPOS, 0, 0);
			SetWindowText(m_hwndStaticStatus, L"Ready");

			// write any remaining frames
			if (FrameCounter > 0) WriteABlockOfFrames(FramesToWrite, FrameCounter, hFile);

			// free frame buffer
			if (FramesToWrite) free(FramesToWrite);

			// close file
			CloseHandle(hFile);
		}

	}

	DWORD WINAPI LoadProjectOnThread(LPVOID lpParm) {

		MainWindowContext* mctx = (MainWindowContext*)lpParm;
		UINT32 SampleSize = sizeof(short);
		UINT32 NumberOfChannels = 2;

		std::wstring PropFileName(mctx->ProjectDir);
		PropFileName.append(L"\\properties.json");

		wprintf(L"reading %s\n", PropFileName.c_str());

		HANDLE hFile = CreateFile(PropFileName.c_str(), GENERIC_READ, FILE_SHARE_READ,
			nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile) {
			DWORD fsize = GetFileSize(hFile, nullptr);
			char* buffer = (char*)malloc(fsize + 1);
			memset(buffer, 0, fsize + 1);
			DWORD nbr = 0;
			ReadFile(hFile, buffer, fsize, &nbr, nullptr);
			printf("read %i bytes\n", nbr);
			if (nbr == fsize) {

				rapidjson::Document d;
				d.Parse(buffer);

				mctx->rec_time_seconds = d["rec_time_seconds"].GetInt();
				mctx->zoom_mult = d["zoom_mult"].GetInt();
				mctx->frame_offset = d["frame_offset"].GetInt();
				mctx->max_frames = d["max_frames"].GetInt();
				mctx->hscroll_pos = d["hscroll_pos"].GetInt();
				mctx->vscroll_pos = d["vscroll_pos"].GetInt();
				mctx->auto_position_timebar = d["auto_position_timebar"].GetInt();

				printf("rec time secs %i\n", mctx->rec_time_seconds);
				printf("zoom mult %i\n", mctx->zoom_mult);
				printf("frame offset %i\n", mctx->frame_offset);
				printf("max_frames %i\n", mctx->max_frames);
				printf("hs pos %i\n", mctx->hscroll_pos);
				printf("vs pos %i\n", mctx->vscroll_pos);
				printf("auto pos tb %i\n", mctx->auto_position_timebar);

				rapidjson::Value::Array trackArray = d["TrackContextList"].GetArray();

				//for (rapidjson::Value::ConstValueIterator i = trackArray.Begin(); i != trackArray.End(); ++i)
				for(int i=0; i<16; i++)
				{

					TrackContext* tctx = mctx->TrackContextList[i];
					tctx->InputChannelIndex = trackArray[i]["InputChannelIndex"].GetInt();
					tctx->leftpan = (float)trackArray[i]["leftpan"].GetDouble();
					tctx->rightpan = (float)trackArray[i]["rightpan"].GetDouble();
					tctx->volume = (float)trackArray[i]["volume"].GetDouble();
					tctx->state = trackArray[i]["state"].GetInt();
					tctx->IsMinimized = trackArray[i]["IsMinimized"].GetBool();

					printf("=====\n");
					printf("track id %i\n", tctx->TrackIndex);
					printf("in ch %i\n", tctx->InputChannelIndex);
					printf("leftpan %.1f\n", tctx->leftpan);
					printf("rightpan %.1f\n", tctx->rightpan);
					printf("volume %.1f\n", tctx->volume);
					printf("state %i\n", tctx->state);
					printf("IsMinimized %i\n", tctx->IsMinimized);
				}

				LPCWSTR TrackFileNames[16] = {
					L"\\track01.wav",L"\\track02.wav",L"\\track03.wav",L"\\track04.wav",
					L"\\track05.wav",L"\\track06.wav",L"\\track07.wav",L"\\track08.wav",
					L"\\track09.wav",L"\\track10.wav",L"\\track11.wav",L"\\track12.wav",
					L"\\track13.wav",L"\\track14.wav",L"\\track15.wav",L"\\track16.wav",
				};

				UINT32 DataSize =
					REC_TIME_SECONDS *		// secs of rec time
					SAMPLES_PER_SEC *		// samples per sec
					SampleSize *			// size of a single sample
					NumberOfChannels;		// channels

				for (int t = 0; t < 16; t++) {
					std::wstring WaveFileName(mctx->ProjectDir);
					WaveFileName.append(TrackFileNames[t]);
					HANDLE WaveFile = CreateFile(WaveFileName.c_str(), GENERIC_READ, FILE_SHARE_READ,
						nullptr, OPEN_EXISTING, 0, nullptr);
					if (WaveFile) {
						DWORD fsize = GetFileSize(WaveFile, nullptr);
						SetFilePointer(WaveFile, 44, nullptr, FILE_BEGIN);
						TrackContext* tctx = mctx->TrackContextList[t];
						printf("reading %i bytes\n", DataSize);
						BOOL bres = ReadFile(WaveFile, tctx->monobuffershort, DataSize, &nbr, nullptr);
						// invalid access to the memory location, buffer too small?
						if (bres == FALSE) printf("result is false; %i\n", GetLastError());
						printf("read %i bytes\n", nbr);
						if (nbr != DataSize) printf("ERROR: Didn't read all wave bytes\n");
						CloseHandle(WaveFile);
					}
				}

				SetScrollPos(m_hwndMainWindow, SB_HORZ, mctx->hscroll_pos, TRUE);
				SetScrollPos(m_hwndMainWindow, SB_VERT, mctx->vscroll_pos, TRUE);
				RedrawTimeBar();
				reposition_all_tracks(mctx);
				for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
					InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
				}

			}
			else {
				printf("failed to read file");
				StartNewProject();
			}
			CloseHandle(hFile);
		}
		else {
			printf("failed to open props file\n");
		}
	}

	DWORD WINAPI SaveProjectOnThread(LPVOID lpParm) {

		char foo[MAX_PATH];
		size_t ncv = 0;

		MainWindowContext* mctx = (MainWindowContext*)lpParm;

		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();
		writer.String("rec_time_seconds");
		writer.Int(mctx->rec_time_seconds);
		writer.String("zoom_mult");
		writer.Int(mctx->zoom_mult);
		writer.String("frame_offset");
		writer.Int(mctx->frame_offset);
		writer.String("max_frames");
		writer.Int(mctx->max_frames);
		writer.String("hscroll_pos");
		writer.Int(mctx->hscroll_pos);
		writer.String("vscroll_pos");
		writer.Int(mctx->vscroll_pos);
		writer.String("auto_position_timebar");
		writer.Bool(mctx->auto_position_timebar);

		writer.String("TrackContextList");
		writer.StartArray();
		for (int t = 0; t < 16; t++) {
			writer.StartObject();
			TrackContext* tctx = mctx->TrackContextList[t];
			writer.String("InputChannelIndex");
			writer.Int(tctx->InputChannelIndex);
			writer.String("leftpan");
			writer.Double(tctx->leftpan);
			writer.String("rightpan");
			writer.Double(tctx->rightpan);
			writer.String("volume");
			writer.Double(tctx->volume);
			writer.String("state");
			writer.Int(tctx->state);
			writer.String("IsMinimized");
			writer.Bool(tctx->IsMinimized);
			writer.String("TrackIndex");
			writer.Int(tctx->TrackIndex);
			writer.EndObject();
		}
		writer.EndArray();

		memset(foo, 0, MAX_PATH);
		wcstombs_s(&ncv, foo, MAX_PATH, mctx->WavFileName, wcslen(mctx->WavFileName));
		writer.String("WavFileName");
		writer.String(foo);
		
		memset(foo, 0, MAX_PATH);
		wcstombs_s(&ncv, foo, MAX_PATH, mctx->ProjectDir.c_str(), mctx->ProjectDir.length());
		writer.String("ProjectDir");
		writer.String(foo);

		writer.EndObject();


		std::wstring PropFileName(mctx->ProjectDir);
		PropFileName.append(L"\\properties.json");
		std::ofstream pout(PropFileName);
		pout << buffer.GetString() << std::endl;
		pout.flush();
		pout.close();

		LPCWSTR TrackFileNames[16] = {
			L"\\track01.wav",L"\\track02.wav",L"\\track03.wav",L"\\track04.wav",
			L"\\track05.wav",L"\\track06.wav",L"\\track07.wav",L"\\track08.wav",
			L"\\track09.wav",L"\\track10.wav",L"\\track11.wav",L"\\track12.wav",
			L"\\track13.wav",L"\\track14.wav",L"\\track15.wav",L"\\track16.wav",
		};

		for (int t = 0; t < 16; t++) {
			std::wstring WaveFileName(mctx->ProjectDir);
			WaveFileName.append(TrackFileNames[t]);
			WriteWaveFile(WaveFileName.c_str(), mctx->rec_time_seconds, SAMPLES_PER_SEC, mctx->TrackContextList, t);
		}

		return 0;
	}

	DWORD WINAPI SaveOnThread(LPVOID lpParm)
	{
		MainWindowContext* mctx = (MainWindowContext*)lpParm;

		WriteWaveFile(mctx->WavFileName, REC_TIME_SECONDS, SAMPLES_PER_SEC, mctx->TrackContextList);

		return 0;
	}

	void SaveFileAs(HWND hwnd) {

		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		memset(mctx->WavFileName, 0, 1024 * sizeof(wchar_t));

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwnd;
		ofn.hInstance = GetModuleHandle(nullptr);
		ofn.lpstrFilter = L"*.wav";
		ofn.lpstrFile = mctx->WavFileName;
		ofn.nMaxFile = 1024;

		if (GetSaveFileName(&ofn)) {
			CreateThread(nullptr, 0, SaveOnThread, (LPVOID)mctx, 0, nullptr);
		}
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
			mctx->rec_time_seconds = REC_TIME_SECONDS; // five minutes
			mctx->max_frames = SAMPLES_PER_SEC * mctx->rec_time_seconds;
			mctx->zoom_mult = 256;
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
			int wmId = LOWORD(wParam);
			switch (wmId)
			{
			case BTN_ZOOM_IN:
				if (mctx->zoom_mult > 1) mctx->zoom_mult /= 2;
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				mctx->auto_position_timebar = FALSE;
				RedrawTimeBar();
				break;
			case BTN_ZOOM_OUT:
				mctx->zoom_mult *= 2;
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				mctx->auto_position_timebar = FALSE;
				RedrawTimeBar();
				break;
			case BTN_PLAY:
				StartPlayback();
				break;
			case BTN_REC:
				StartRecording();
				break;
			case BTN_STOP:
				mctx->auto_position_timebar = FALSE;
				m_TriggerStop = TRUE;
				break;
			case IDM_ABOUT:
				DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			case ID_FILE_SETUP:
				DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_SETUPDLG), hWnd, SetupDlgProc);
				break;
			case ID_FILE_SAVEAS:
				SaveFileAs(hWnd);
				break;
			case ID_FILE_SETPROJECTDIR:
				GetFolder(mctx->ProjectDir, L"Set Project Directory", hWnd);
				break;
			case ID_FILE_NEW:
				StartNewProject();
				break;
			case ID_FILE_LOADPROJECT:
				GetFolder(mctx->ProjectDir, L"Set Project Directory", hWnd);
				if (mctx->ProjectDir.length() > 0) {
					CreateThread(nullptr, 0, LoadProjectOnThread, (LPVOID)mctx, 0, nullptr);
				}
				break;
			case ID_FILE_SAVEPROJECT:
				if (mctx->ProjectDir.length() < 1)
				{
					GetFolder(mctx->ProjectDir, L"Set Project Directory", hWnd);
					if (mctx->ProjectDir.length() < 1) {
						MessageBox(hWnd, L"You must set a project directory.", L"ERROR", MB_OK);
					}
					else {
						CreateThread(nullptr, 0, SaveProjectOnThread, (LPVOID)mctx, 0, nullptr);
					}
				}
				else {
					CreateThread(nullptr, 0, SaveProjectOnThread, (LPVOID)mctx, 0, nullptr);
				}
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
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			DrawTimeBar(hWnd, hdc, mctx);
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			// TODO
			// if this is running, need to gracefully
			// stop the thread, wait, and close
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (m_hCaptureThread) CloseHandle(m_hCaptureThread);
			if (m_hRenderThread) CloseHandle(m_hRenderThread);
			for (UINT32 TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
				if (mctx->TrackContextList[TrackIndex] != nullptr) {
					DestroyWindow(mctx->TrackContextList[TrackIndex]->TrackWindow);
				}
			}
			if (m_lpCaptureDevices) rta_free_device_list(m_lpCaptureDevices);
			if (m_lpRenderDevices) rta_free_device_list(m_lpRenderDevices);
			m_lpCaptureDevices = nullptr;
			m_lpRenderDevices = nullptr;
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
			case SB_LINEUP:
			case SB_PAGEUP:
				pos = GetScrollPos(hWnd, SB_HORZ);
				if (pos > 0) {
					if ((LOWORD(wParam) == SB_PAGEUP) && (pos > 9)) {
						pos -= 10;
					}
					else {
						pos--;
					}
					SetScrollPos(hWnd, SB_HORZ, pos, TRUE);
					mctx->hscroll_pos = pos;
					for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
						if (mctx->TrackContextList[TrackIndex] != nullptr) {
							InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
						}
					}
					mctx->auto_position_timebar = FALSE;
					RedrawTimeBar();
				}
				break;
			case SB_LINEDOWN:
			case SB_PAGEDOWN:
				pos = GetScrollPos(hWnd, SB_HORZ);
				if (pos < 65535) {
					if ((LOWORD(wParam) == SB_PAGEDOWN) && (pos < (65535 - 9))) {
						pos += 10;
					}
					else {
						pos++;
					}
					SetScrollPos(hWnd, SB_HORZ, pos, TRUE);
					mctx->hscroll_pos = pos;
					for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
						if (mctx->TrackContextList[TrackIndex] != nullptr) {
							InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
						}
					}
					mctx->auto_position_timebar = FALSE;
					RedrawTimeBar();
				}
				break;
			case SB_THUMBTRACK:
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				mctx->hscroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				mctx->auto_position_timebar = FALSE;
				RedrawTimeBar();
				break;
			case SB_THUMBPOSITION:
				SetScrollPos(hWnd, SB_HORZ, HIWORD(wParam), TRUE);
				mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				mctx->hscroll_pos = HIWORD(wParam);
				for (UINT32 TrackIndex = mctx->vscroll_pos; TrackIndex < NUM_TRACKS; TrackIndex++) {
					if (mctx->TrackContextList[TrackIndex] != nullptr) {
						InvalidateRect(mctx->TrackContextList[TrackIndex]->TrackWindow, nullptr, FALSE);
					}
				}
				mctx->auto_position_timebar = FALSE;
				RedrawTimeBar();
				break;
			}
			break;
		case WM_SIZE:
			//SendMessage(hwndStatus, WM_SIZE, wParam, lParam);
			mctx = (MainWindowContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			for (int TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
				if (mctx->TrackContextList[TrackIndex] != nullptr)
					SendMessage(mctx->TrackContextList[TrackIndex]->TrackWindow, WM_SIZE, wParam, lParam);
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

		LoadStringW(GetModuleHandle(nullptr), IDC_MUSICSTUDIOCX, m_szWindowClass, MAX_LOADSTRING);
		LoadStringW(GetModuleHandle(nullptr), IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);

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
		wcex.lpszClassName = m_szWindowClass;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassExW(&wcex);
	}

	HWND create_main_window()
	{
		m_hwndMainWindow = CreateWindowW(m_szWindowClass, m_szTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
		//CreateStatusBar(hwndMainWindow);

		CXCommon::CreateButton(m_hwndMainWindow, 0, 0, 64, 32, L"ZIN", BTN_ZOOM_IN);
		CXCommon::CreateButton(m_hwndMainWindow, 64, 0, 64, 32, L"ZOUT", BTN_ZOOM_OUT);
		CXCommon::CreateButton(m_hwndMainWindow, 128, 0, 64, 32, L"PLAY", BTN_PLAY);
		CXCommon::CreateButton(m_hwndMainWindow, 192, 0, 64, 32, L"RECD", BTN_REC);
		CXCommon::CreateButton(m_hwndMainWindow, 256, 0, 64, 32, L"STOP", BTN_STOP);

		// create progress bar here
		m_hwndProgBar = CreateWindowEx(0, L"msctls_progress32", nullptr, WS_CHILD | WS_VISIBLE, 324, 8, 200, 16,
			m_hwndMainWindow, (HMENU)ID_SB_PROGRESS_BAR, GetModuleHandle(nullptr), nullptr);
		SendMessage(m_hwndProgBar, PBM_SETSTEP, (WPARAM)1, 0);
		SendMessage(m_hwndProgBar, PBM_SETRANGE, 0, MAKELONG(0,100));

		// create label here
		m_hwndStaticStatus = CreateWindow(L"STATIC", nullptr, WS_CHILD | WS_VISIBLE, 528, 8, 200, 16,
			m_hwndMainWindow, (HMENU)ID_STATIC_STATUS, GetModuleHandle(nullptr), nullptr);
		SetWindowText(m_hwndStaticStatus, L"Ready");

		// set scroll bar info
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(m_hwndMainWindow, GWLP_USERDATA);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.nMin = 0;
		si.nMax = 65535;
		si.nPos = 0;
		si.nPage = 1;
		si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
		SetScrollInfo(m_hwndMainWindow, SB_HORZ, &si, TRUE);

		si.nMax = 15;
		SetScrollInfo(m_hwndMainWindow, SB_VERT, &si, TRUE);

		rta_list_supporting_devices_2(&m_lpCaptureDevices, &m_StandardFormatInDefault, DEVICE_STATE_ACTIVE,
			eCapture, AUDCLNT_SHAREMODE_EXCLUSIVE, TRUE);
		mctx->CaptureDevInfo = m_lpCaptureDevices;

		rta_list_supporting_devices_2(&m_lpRenderDevices, &m_StandardFormatOutDefault, DEVICE_STATE_ACTIVE,
			eRender, AUDCLNT_SHAREMODE_EXCLUSIVE, FALSE);
		mctx->RenderDevInfo = m_lpRenderDevices;

		wchar_t TrackName[] = L"Track##";
		wchar_t digit = '0';
		TrackContext* prevContext = nullptr;
		for (int TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
			if (TrackIndex < 9) {
				TrackName[5] = '0';
				TrackName[6] = digit + TrackIndex + 1;
			}
			else {
				TrackName[5] = '1';
				TrackName[6] = digit + (TrackIndex - 9);
			}
			TrackContext* ctx = MusicStudioCX::create_track_window_a(m_hwndMainWindow, TrackName, 0);
			ctx->TrackIndex = TrackIndex;
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

		return m_hwndMainWindow;
	}

	void reposition_all_tracks(MainWindowContext* mctx)
	{
		UINT32 offset = 0;
		RECT r;
		for (int TrackIndex = 0; TrackIndex < NUM_TRACKS; TrackIndex++) {
			TrackContext* ctx = mctx->TrackContextList[TrackIndex];
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