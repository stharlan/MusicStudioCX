
#include "stdafx.h"

#define BTN_ARM_REC 20000
#define BTN_MUTE_TRACK 20001
#define BTN_TRACK_PROP 20002

namespace MusicStudioCX
{

	INT_PTR CALLBACK TrackPropsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	const UINT32 TRACK_STATE_MUTE = 0x01;
	const UINT32 TRACK_STATE_ARMED = 0x02;

	BOOL TrackIsMute(UINT32 state) {
		return (state & TRACK_STATE_MUTE ? TRUE : FALSE);
	}

	BOOL TrackIsArmed(UINT32 state) {
		return (state & TRACK_STATE_ARMED ? TRUE : FALSE);
	}

	void DrawTrackWindow(HWND hWnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		TrackContext * ctx = get_track_context(hWnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
		RECT r, r1;
		HGDIOBJ OldPen = nullptr;
		wchar_t WindowName[16];

		if (FALSE == ctx->IsMinimized) {
			HBRUSH GreenBrush = CreateSolidBrush(RGB(0, 255, 0));
			HBRUSH RedBrush = CreateSolidBrush(RGB(255, 0, 0));

			r1.left = WVFRM_OFFSET - 8;
			r1.right = WVFRM_OFFSET;
			r1.top = 32;
			r1.bottom = 64;
			FillRect(hdc, &r1, ((ctx->state & TRACK_STATE_ARMED) ? GreenBrush : RedBrush));

			r1.top += 32;
			r1.bottom += 32;
			FillRect(hdc, &r1, ((ctx->state & TRACK_STATE_MUTE) ? GreenBrush : RedBrush));

			DeleteObject(GreenBrush);
			DeleteObject(RedBrush);
		}

		GetClientRect(hWnd, &r);
		r.left += WVFRM_OFFSET;

		HBRUSH grayBrush = CreateSolidBrush(RGB(0x33, 0x33, 0x33));
		HPEN grayPen = CreatePen(PS_SOLID, 1, RGB(0x66, 0x66, 0x66));
		HBRUSH grayHatch = CreateHatchBrush(HS_DIAGCROSS, RGB(0x66, 0x66, 0x66));
		HGDIOBJ oldPen1 = SelectObject(hdc, grayPen);
		if (TRUE == ctx->IsMinimized) {
			r.bottom = 32;
			COLORREF oldColor = SetBkColor(hdc, RGB(0x33, 0x33, 0x33));
			FillRect(hdc, &r, grayHatch);
			SetBkColor(hdc, oldColor);
		}
		else {
			FillRect(hdc, &r, grayBrush);
		}
		MoveToEx(hdc, WVFRM_OFFSET, r.bottom - 1, nullptr);
		LineTo(hdc, r.right - r.left + WVFRM_OFFSET, r.bottom - 1);
		SelectObject(hdc, oldPen1);
		DeleteObject(grayBrush);
		DeleteObject(grayPen);
		DeleteObject(grayHatch);

		if (FALSE == ctx->IsMinimized) {
			HPEN GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
			OldPen = SelectObject(hdc, GreenPen);

			MoveToEx(hdc, WVFRM_OFFSET, 64, nullptr);

			// get scroll bar pos
			float pos = (float)mctx->hscroll_pos;
			UINT32 StartFrame = (UINT32)((pos / 65535.0f) * (float)mctx->max_frames);
			UINT32 rpos = 0;
			UINT32 RightLimit = (UINT32)r.right;
			for (UINT32 FrameCount = StartFrame; FrameCount < mctx->max_frames; FrameCount += mctx->zoom_mult) {
				rpos = (FrameCount - StartFrame) / mctx->zoom_mult + WVFRM_OFFSET;
				if (rpos > RightLimit) break;
				LineTo(hdc, rpos, (ctx->monobuffershort[FrameCount] / 512) + 64);
			}
			SelectObject(hdc, OldPen);
			DeleteObject(GreenPen);
		}

		memset(WindowName, 0, 16 * sizeof(wchar_t));
		GetWindowText(hWnd, WindowName, 16);

		SetTextColor(hdc, 0x00000000);
		r1.left = r1.top = 0;
		r1.right = WVFRM_OFFSET;
		r1.bottom = 32;
		DrawText(hdc, WindowName, wcslen(WindowName), &r1, DT_LEFT | DT_TOP);

		EndPaint(hWnd, &ps);

	}

	LRESULT CALLBACK track_wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		TrackContext * ctx = nullptr;
		MainWindowContext* mctx = nullptr;
		WORD mx = 0, my = 0;
		RECT wr;
		UINT32 itemHeight = 0;
		switch (message)
		{
		case WM_LBUTTONDBLCLK:
			ctx = get_track_context(hWnd);
			mx = LOWORD(lParam);
			my = HIWORD(lParam);
			if (mx > 0 && mx < WVFRM_OFFSET && my > 0 && my < 32) {
				if (ctx->IsMinimized == FALSE) {
					ctx->IsMinimized = TRUE;
					//ShowWindow(ctx->buttons[0], SW_HIDE);
					//ShowWindow(ctx->buttons[1], SW_HIDE);
					//ShowWindow(ctx->buttons[2], SW_HIDE);
					//GetWindowRect(hWnd, &wr);
					//SetWindowPos(hWnd, nullptr, 0, 0, wr.right - wr.left, 32, SWP_NOZORDER | SWP_NOMOVE);
					//InvalidateRect(hWnd, nullptr, FALSE);
					//while (ctx->NextTrackWindow != nullptr) {
						//ctx = get_track_context(ctx->NextTrackWindow);
						//GetWindowRect(ctx->TrackWindow, &wr);
						//SetWindowPos(ctx->TrackWindow, nullptr, 0, (ctx->TrackIndex * 128) - 96 + 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
						//InvalidateRect(ctx->TrackWindow, nullptr, FALSE);
					//};
				}
				else {
					ctx->IsMinimized = FALSE;
					//ShowWindow(ctx->buttons[0], SW_SHOW);
					//ShowWindow(ctx->buttons[1], SW_SHOW);
					//ShowWindow(ctx->buttons[2], SW_SHOW);
					//GetWindowRect(hWnd, &wr);
					//SetWindowPos(hWnd, nullptr, 0, 0, wr.right - wr.left, 128, SWP_NOZORDER | SWP_NOMOVE);
					//InvalidateRect(hWnd, nullptr, FALSE);
					//while (ctx->NextTrackWindow != nullptr) {
						//ctx = get_track_context(ctx->NextTrackWindow);
						//GetWindowRect(ctx->TrackWindow, &wr);
						//SetWindowPos(ctx->TrackWindow, nullptr, 0, (ctx->TrackIndex * 128) + 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
						//InvalidateRect(ctx->TrackWindow, nullptr, FALSE);
					//};
				}
				mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
				reposition_all_tracks(mctx);
			}
			break;
		case WM_COMMAND:
			ctx = get_track_context(hWnd);
			switch (LOWORD(wParam)) {
			case BTN_ARM_REC:
				ctx->state ^= TRACK_STATE_ARMED;
				SetWindowText((HWND)lParam, ((ctx->state & TRACK_STATE_ARMED) ? L"REC" : L"Rec"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			case BTN_MUTE_TRACK:
				ctx->state ^= TRACK_STATE_MUTE;
				SetWindowText((HWND)lParam, ((ctx->state & TRACK_STATE_MUTE) ? L"MUTE" : L"Mute"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			case BTN_TRACK_PROP:
				DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_TRACKPROPS), hWnd, TrackPropsDialogProc, (LPARAM)ctx);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_NCDESTROY:
			// free context
			ctx = get_track_context(hWnd);
			if (ctx->monobuffershort) free(ctx->monobuffershort);
			if (ctx) free(ctx);
			break;
		case WM_PAINT:
			DrawTrackWindow(hWnd);
			break;
		case WM_SIZE:
			ctx = get_track_context(hWnd);
			itemHeight = 128;
			if (ctx != nullptr) {
				if (TRUE == ctx->IsMinimized) itemHeight = 32;
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
		memset(&wcctl, 0, sizeof(WNDCLASS));

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
		memset(ctx, 0, sizeof(TrackContext));
		mctx = (MainWindowContext*)GetWindowLongPtr(parent, GWLP_USERDATA);
		// sixty seconds worth of samples
		ctx->state = 0;
		ctx->InputChannelIndex = channel;
		ctx->leftpan = 1.0f;
		ctx->rightpan = 1.0f;
		ctx->volume = 1.0f;
		ctx->IsMinimized = FALSE;
		ctx->monobuffershort = (short*)malloc(SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
		memset(ctx->monobuffershort, 0, SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
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
