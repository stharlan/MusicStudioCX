
#include "stdafx.h"

#define BTN_ARM_REC 20000
#define BTN_MUTE_TRACK 20001
#define BTN_TRACK_PROP 20002

namespace MusicStudioCX
{

	INT_PTR CALLBACK TrackPropsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	void DrawTrackWindow(HWND hWnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		TrackContext * ctx = get_track_context(hWnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
		RECT r, r1;
		HGDIOBJ OldPen = nullptr;
		wchar_t WindowName[16];

		if(FALSE == CheckState(ctx, TRACK_STATE_MINIMIZED)) {
			HBRUSH GreenBrush = CreateSolidBrush(RGB(0, 255, 0));
			HBRUSH RedBrush = CreateSolidBrush(RGB(255, 0, 0));

			r1.left = WVFRM_OFFSET - 8;
			r1.right = WVFRM_OFFSET;
			r1.top = 32;
			r1.bottom = 64;
			FillRect(hdc, &r1, (CheckState(ctx, TRACK_STATE_ARMED) ? GreenBrush : RedBrush));

			r1.top += 32;
			r1.bottom += 32;
			FillRect(hdc, &r1, (CheckState(ctx, TRACK_STATE_MUTE) ? GreenBrush : RedBrush));

			DeleteObject(GreenBrush);
			DeleteObject(RedBrush);
		}

		GetClientRect(hWnd, &r);
		r.left += WVFRM_OFFSET;

		HBRUSH grayBrush = CreateSolidBrush(RGB(0x33, 0x33, 0x33));
		HBRUSH gray2Brush = CreateSolidBrush(RGB(0x44, 0x44, 0x44));
		HPEN grayPen = CreatePen(PS_SOLID, 1, RGB(0x66, 0x66, 0x66));
		HBRUSH grayHatch = CreateHatchBrush(HS_DIAGCROSS, RGB(0x66, 0x66, 0x66));
		HGDIOBJ oldPen1 = SelectObject(hdc, grayPen);
		if(TRUE == CheckState(ctx, TRACK_STATE_MINIMIZED)) {
			r.bottom = 32;
			COLORREF oldColor = SetBkColor(hdc, RGB(0x33, 0x33, 0x33));
			FillRect(hdc, &r, grayHatch);
			SetBkColor(hdc, oldColor);
		}
		else if (TRUE == CheckState(ctx, TRACK_STATE_SELECTED)) {
			FillRect(hdc, &r, gray2Brush);
		}
		else {
			FillRect(hdc, &r, grayBrush);
		}
		MoveToEx(hdc, WVFRM_OFFSET, r.bottom - 1, nullptr);
		LineTo(hdc, r.right - r.left + WVFRM_OFFSET, r.bottom - 1);
		SelectObject(hdc, oldPen1);
		DeleteObject(grayBrush);
		DeleteObject(gray2Brush);
		DeleteObject(grayPen);

		if(FALSE == CheckState(ctx, TRACK_STATE_MINIMIZED)) {
			HPEN GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
			HPEN bluePen = CreatePen(PS_SOLID, 1, RGB(0x66, 0x66, 0x66));
			OldPen = SelectObject(hdc, GreenPen);

			MoveToEx(hdc, WVFRM_OFFSET, 64, nullptr);

			// get scroll bar pos
			UINT32 StartFrame = MulDiv(mctx->hscroll_pos, mctx->max_frames, 65535);
			UINT32 rpos1 = 0;
			UINT32 rpos2 = 0;
			UINT32 RightLimit = (UINT32)r.right;
			for (UINT32 FrameCount = StartFrame; FrameCount < mctx->max_frames; FrameCount += mctx->zoom_mult) {
				rpos1 = (FrameCount - StartFrame) / mctx->zoom_mult + WVFRM_OFFSET;
				if (rpos1 > RightLimit) break;
				LineTo(hdc, rpos1, (ctx->monobuffershort[FrameCount] / 512) + 64);
			}
			if (mctx->sel_begin_frame >= StartFrame) {
				rpos1 = (mctx->sel_begin_frame - StartFrame) / mctx->zoom_mult + WVFRM_OFFSET;
				if (rpos1 < RightLimit) {
					SelectObject(hdc, bluePen);
					MoveToEx(hdc, rpos1, 0, nullptr);
					if (TRUE == CheckState(ctx, TRACK_STATE_MINIMIZED)) {
						LineTo(hdc, rpos1, 32);
					}
					else {
						LineTo(hdc, rpos1, r.bottom);
					}
				}
				if (mctx->sel_end_frame > mctx->sel_begin_frame) {
					rpos2 = (mctx->sel_end_frame - StartFrame) / mctx->zoom_mult + WVFRM_OFFSET;
					if (rpos2 < RightLimit) {
						SelectObject(hdc, bluePen);
						MoveToEx(hdc, rpos2, 0, nullptr);
						if (TRUE == CheckState(ctx, TRACK_STATE_MINIMIZED)) {
							LineTo(hdc, rpos2, 32);
							int oldMode = SetBkMode(hdc, TRANSPARENT);
							HGDIOBJ oldBrush = SelectObject(hdc, (HGDIOBJ)grayHatch);
							Rectangle(hdc, rpos1, 0, rpos2, 32);
							SetBkMode(hdc, oldMode);
							SelectObject(hdc, oldBrush);
						}
						else {
							LineTo(hdc, rpos2, r.bottom);
							int oldMode = SetBkMode(hdc, TRANSPARENT);
							HGDIOBJ oldBrush = SelectObject(hdc, (HGDIOBJ)grayHatch);
							Rectangle(hdc, rpos1, 0, rpos2, r.bottom);
							SetBkMode(hdc, oldMode);
							SelectObject(hdc, oldBrush);
						}
					}
				}
			}
			SelectObject(hdc, OldPen);
			DeleteObject(GreenPen);
			DeleteObject(bluePen);
		}

		DeleteObject(grayHatch);

		ZeroMemory(WindowName, 16 * sizeof(wchar_t));
		GetWindowText(hWnd, WindowName, 16);

		SetTextColor(hdc, 0x00000000);
		r1.left = r1.top = 0;
		r1.right = WVFRM_OFFSET;
		r1.bottom = 32;
		DrawText(hdc, WindowName, wcslen(WindowName), &r1, DT_LEFT | DT_TOP);

		EndPaint(hWnd, &ps);

	}

	void RedrawAllTracks(HWND twnd) 
	{
		HWND hParent = GetParent(twnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
		for (int ti = 0; ti < NUM_TRACKS; ti++) {
			InvalidateRect(mctx->TrackContextList[ti]->TrackWindow, nullptr, FALSE);
		}
	}

	void SelectRange(HWND twnd)
	{
		int FirstSelected = -1;
		int LastSelected = -1;
		int ThisIndex = -1;
		HWND hParent = GetParent(twnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
		for (int ti = 0; ti < NUM_TRACKS; ti++) {
			TrackContext* tctx = mctx->TrackContextList[ti];
			if (TRUE == CheckState(tctx, TRACK_STATE_SELECTED)) {
				if (FirstSelected == -1) {
					FirstSelected = ti;
				}
				else {
					LastSelected = ti;
				}
			}
			if (tctx->TrackWindow == twnd) ThisIndex = ti;
		}

		if (FirstSelected == -1) {
			SetState(mctx->TrackContextList[ThisIndex], TRACK_STATE_SELECTED);
			return;
		}

		if (LastSelected == -1) {
			if (ThisIndex < FirstSelected) {
				LastSelected = FirstSelected;
				FirstSelected = ThisIndex;
			}
			else if(ThisIndex > FirstSelected) {
				LastSelected = ThisIndex;
			}
		}
		else {
			if (ThisIndex < FirstSelected) FirstSelected = ThisIndex;
			if (ThisIndex > LastSelected) LastSelected = ThisIndex;
		}
				
		for (int ti = FirstSelected; ti < (LastSelected + 1); ti++) {
			SetState(mctx->TrackContextList[ti], TRACK_STATE_SELECTED);
		}
	}

	void SelectAdd(HWND twnd)
	{
		HWND hParent = GetParent(twnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
		for (int ti = 0; ti < NUM_TRACKS; ti++) {
			if (mctx->TrackContextList[ti]->TrackWindow == twnd) {
				SetState(mctx->TrackContextList[ti], TRACK_STATE_SELECTED);
				ti = NUM_TRACKS;
			}
		}
	}

	void SelectOnly(HWND twnd) 
	{
		HWND hParent = GetParent(twnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
		for (int ti = 0; ti < NUM_TRACKS; ti++) {
			if (mctx->TrackContextList[ti]->TrackWindow == twnd) {
				SetState(mctx->TrackContextList[ti], TRACK_STATE_SELECTED);
			}
			else {
				ClearState(mctx->TrackContextList[ti], TRACK_STATE_SELECTED);
			}
		}
	}

	UINT32 SnapFrameToBeat(UINT32 frame) {
		UINT32 outFrame = frame;
		// snap to beat
		// 120 bpmin
		// 2 bpsec
		// 48000 sample per sec
		// 24000 samples per beat
		UINT32 rem = outFrame % 24000;
		if (rem < 12000) {
			// snap down
			outFrame -= rem;
		}
		else {
			// snap up
			outFrame = outFrame - rem + 24000;
		}
		return outFrame;
	}

	LRESULT CALLBACK track_wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		TrackContext * tctx = nullptr;
		MainWindowContext* mctx = nullptr;
		WORD mx = 0, my = 0;
		RECT wr;
		UINT32 itemHeight = 0;
		BOOL IsArmed = FALSE;
		BOOL IsMute = FALSE;
		switch (message)
		{
		case WM_RBUTTONDOWN:
			if ((GET_X_LPARAM(lParam) >= WVFRM_OFFSET) && (GetKeyState(VK_MENU) < 0)) {
				mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
				mctx->sel_end_frame =
					MulDiv(mctx->hscroll_pos, mctx->max_frames, 65535) +
					((GET_X_LPARAM(lParam) - WVFRM_OFFSET) * mctx->zoom_mult);
				mctx->sel_end_frame = SnapFrameToBeat(mctx->sel_end_frame);
				RedrawAllTracks(hWnd);
#ifdef _DEBUG
				printf("frame is %i\n", mctx->sel_begin_frame);
#endif
			}
			break;
		case WM_LBUTTONDOWN:
			mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
			if ((GET_X_LPARAM(lParam) >= WVFRM_OFFSET) && (GetKeyState(VK_MENU) < 0)) {
				mctx->sel_begin_frame =
					MulDiv(mctx->hscroll_pos, mctx->max_frames, 65535) +
					((GET_X_LPARAM(lParam) - WVFRM_OFFSET) * mctx->zoom_mult);
				mctx->sel_begin_frame = SnapFrameToBeat(mctx->sel_begin_frame);
#ifdef _DEBUG
				printf("frame is %i\n", mctx->sel_begin_frame);
#endif
			}
			else {
				//mctx->sel_begin_frame = mctx->sel_end_frame = 0;
				if (wParam & MK_CONTROL)
				{
					SelectAdd(hWnd);
				}
				else if (wParam & MK_SHIFT) {
					SelectRange(hWnd);
				}
				else {
					SelectOnly(hWnd);
				}
			}
			RedrawAllTracks(hWnd);
			break;
		case WM_LBUTTONDBLCLK:
			tctx = get_track_context(hWnd);
			mx = LOWORD(lParam);
			my = HIWORD(lParam);
			if (mx > 0 && mx < WVFRM_OFFSET && my > 0 && my < 32) {
				ToggleState(tctx, TRACK_STATE_MINIMIZED);
				mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
				reposition_all_tracks(mctx);
			}
			break;
		case WM_COMMAND:
			tctx = get_track_context(hWnd);
			switch (LOWORD(wParam)) {
			case BTN_ARM_REC:
				IsArmed = ToggleState(tctx, TRACK_STATE_ARMED);
				SetWindowText((HWND)lParam, (IsArmed ? L"REC" : L"Rec"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			case BTN_MUTE_TRACK:
				IsMute = ToggleState(tctx, TRACK_STATE_MUTE);
				SetWindowText((HWND)lParam, (IsMute ? L"MUTE" : L"Mute"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			case BTN_TRACK_PROP:
				DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_TRACKPROPS), hWnd, TrackPropsDialogProc, (LPARAM)tctx);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_NCDESTROY:
			tctx = get_track_context(hWnd);
			if (tctx->monobuffershort) free(tctx->monobuffershort);
			if (tctx) free(tctx);
			break;
		case WM_PAINT:
			DrawTrackWindow(hWnd);
			break;
		case WM_SIZE:
			tctx = get_track_context(hWnd);
			itemHeight = 128;
			if (tctx != nullptr) {
				if(TRUE == CheckState(tctx, TRACK_STATE_MINIMIZED)) itemHeight = 32;
			}
			SetWindowPos(hWnd, nullptr, 0, 0, LOWORD(lParam), itemHeight, SWP_NOMOVE | SWP_NOZORDER);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void initialize_track_window()
	{
		WNDCLASS wcctl;
		ZeroMemory(&wcctl, sizeof(WNDCLASS));

		wcctl.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcctl.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcctl.hInstance = GetModuleHandle(nullptr);
		wcctl.lpfnWndProc = track_wnd_callback;
		wcctl.lpszClassName = L"CXTrackWindowClass";
		wcctl.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcctl.cbWndExtra = sizeof(TrackContext*);  // <-- Here we are

		RegisterClass(&wcctl);
	}

	TrackContext* allocate_context(MainWindowContext* mctx, HWND parent, short channel)
	{
		// allocate context
		TrackContext* ctx = (TrackContext*)malloc(sizeof(TrackContext));
		ZeroMemory(ctx, sizeof(TrackContext));
		mctx = (MainWindowContext*)GetWindowLongPtr(parent, GWLP_USERDATA);
		// sixty seconds worth of samples
		ctx->wstate = 0;
		ctx->InputChannelIndex = channel;
		ctx->leftpan = 1.0f;
		ctx->rightpan = 1.0f;
		ctx->volume = 1.0f;
#ifdef _DEBUG
		printf("allocating %i bytes for monobuffershort\n", SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
		MEMORYSTATUSEX msex;
		ZeroMemory(&msex, sizeof(MEMORYSTATUSEX));
		msex.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&msex);
		printf("memory load %i%%\n", msex.dwMemoryLoad);
#endif
		ctx->monobuffershort = (short*)malloc(SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
		ZeroMemory(ctx->monobuffershort, SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
		return ctx;
	}

	TrackContext* create_track_window_a(HWND parent, LPCWSTR TrackName, short channel)
	{
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(parent, GWLP_USERDATA);
		RECT r = { 0 };
		UINT32 idx = 0;

		while (mctx->TrackContextList[idx] != nullptr) idx++;
		mctx->TrackContextList[idx] = allocate_context(mctx, parent, channel);

		GetWindowRect(parent, &r);
		HWND hwnd = CreateWindow(L"CXTrackWindowClass", TrackName,
			WS_CHILD | WS_VISIBLE,
			0, (idx * 128) + MAIN_WINDOW_HEADER_HEIGHT, r.right - r.left, 128,
			parent, nullptr, GetModuleHandle(nullptr), nullptr);

		mctx->TrackContextList[idx]->TrackWindow = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)mctx->TrackContextList[idx]);

		// create some controls here
		mctx->TrackContextList[idx]->buttons[0] = CXCommon::CreateButton(hwnd, 0, 32, 48, 32, L"Rec", BTN_ARM_REC);
		mctx->TrackContextList[idx]->buttons[1] = CXCommon::CreateButton(hwnd, 0, 64, 48, 32, L"Mute", BTN_MUTE_TRACK);
		mctx->TrackContextList[idx]->buttons[2] = CXCommon::CreateButton(hwnd, 0, 96, 48, 32, L"Prop", BTN_TRACK_PROP);

		return mctx->TrackContextList[idx];
	}

	TrackContext* get_track_context(HWND cwnd) {
		return (TrackContext*)GetWindowLongPtr(cwnd, GWLP_USERDATA);
	}

	void generate_sine(float frequency, UINT32 seconds, short* buffer, float max_amplitude)
	{
		for (UINT32 sample = 0; sample < SAMPLES_PER_SEC * seconds; sample++) {
			buffer[sample] = (short)(sinf((float)sample / ((float)SAMPLES_PER_SEC / frequency) * 2.0f * 3.14159f) * 32767.0f * max_amplitude);
		}
	}

	INT_PTR CALLBACK TrackPropsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);
		TrackContext* ctx = nullptr;
		LPARAM SliderPos = 100;

		switch (message)
		{
		case WM_INITDIALOG:
			ctx = (TrackContext*)lParam;
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)ctx);
			// pan value
			// left full is l=1, r=0
			// center is l=1, r=1
			// right full is l=0, r=1
			// Slider should go from 0 to 200
			if (ctx->leftpan < 1.0f) {
				// pan right
				// 100 to 200
				SliderPos = (LPARAM)((1.0f - ctx->leftpan) * 100.0f) + 100;
			}
			else {
				// pan left 0 to 100
				SliderPos = (LPARAM)(ctx->rightpan * 100.0f);
			}
			SendMessage(GetDlgItem(hDlg, IDC_PANSLIDER), TBM_SETRANGE, FALSE, MAKELONG(0, 200));
			SendMessage(GetDlgItem(hDlg, IDC_PANSLIDER), TBM_SETTICFREQ, 20, 0);
			SendMessage(GetDlgItem(hDlg, IDC_PANSLIDER), TBM_SETPOS, TRUE, SliderPos);

			SliderPos = (LPARAM)((1.0f - ctx->volume) * 100.0f);
			SendMessage(GetDlgItem(hDlg, IDC_VOLSLIDER), TBM_SETRANGE, FALSE, MAKELONG(0, 100));
			SendMessage(GetDlgItem(hDlg, IDC_VOLSLIDER), TBM_SETTICFREQ, 10, 0);
			SendMessage(GetDlgItem(hDlg, IDC_VOLSLIDER), TBM_SETPOS, TRUE, SliderPos);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				if (LOWORD(wParam) == IDOK) {
					ctx = (TrackContext*)GetWindowLongPtr(hDlg, DWLP_USER);
					SliderPos = SendMessage(GetDlgItem(hDlg, IDC_PANSLIDER), TBM_GETPOS, 0, 0);
					if (SliderPos == 100) {
						ctx->leftpan = 1.0f;
						ctx->rightpan = 1.0f;
					}
					else if (SliderPos < 100) {
						ctx->rightpan = (float)SliderPos / 100.0f;
						ctx->leftpan = 1.0f;
					}
					else {
						ctx->rightpan = 1.0f;
						ctx->leftpan = 1.0f - ((float)(SliderPos - 100) / 100.0f);
					}
					SliderPos = SendMessage(GetDlgItem(hDlg, IDC_VOLSLIDER), TBM_GETPOS, 0, 0);
					ctx->volume = 1.0f - ((float)SliderPos / 100.0f);
				}
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		}
		return (INT_PTR)FALSE;
	}

}
