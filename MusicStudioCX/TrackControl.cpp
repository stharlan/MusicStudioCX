
#include "stdafx.h"

#define WVFRM_OFFSET 56

#define BTN_ARM_REC 20000
#define BTN_MUTE_TRACK 20001

#define TRACK_IS_MUTE (UINT32)0x01
#define TRACK_IS_ARMED (UINT32)0x02

namespace MusicStudioCX
{

	void DrawTrackWindow(HWND hWnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		TrackContext * ctx = get_track_context(hWnd);
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
		RECT r, r1;
		HGDIOBJ OldPen = nullptr;

		HBRUSH GreenBrush = CreateSolidBrush(RGB(0, 255, 0));
		HBRUSH RedBrush = CreateSolidBrush(RGB(255, 0, 0));

		r1.left = WVFRM_OFFSET - 8;
		r1.right = WVFRM_OFFSET;
		r1.top = 0;
		r1.bottom = 32;
		FillRect(hdc, &r1, ((ctx->state & TRACK_IS_ARMED) ? GreenBrush : RedBrush));

		r1.top += 32;
		r1.bottom += 32;
		FillRect(hdc, &r1, ((ctx->state & TRACK_IS_MUTE) ? GreenBrush : RedBrush));

		GetClientRect(hWnd, &r);
		r.left += WVFRM_OFFSET;

		FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
		HPEN GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
		OldPen = SelectObject(hdc, GreenPen);

		MoveToEx(hdc, WVFRM_OFFSET, 64, nullptr);

		// get scroll bar pos
		float pos = (float)mctx->scroll_pos;
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
		DeleteObject(GreenBrush);
		DeleteObject(RedBrush);
		EndPaint(hWnd, &ps);

	}

	LRESULT CALLBACK track_wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		TrackContext * ctx = nullptr;
		MainWindowContext* mctx = nullptr;
		switch (message)
		{
		case WM_COMMAND:
			ctx = get_track_context(hWnd);
			switch (LOWORD(wParam)) {
			case BTN_ARM_REC:
				ctx->state ^= TRACK_IS_ARMED;
				SetWindowText((HWND)lParam, ((ctx->state & TRACK_IS_ARMED) ? L"REC" : L"Rec"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			case BTN_MUTE_TRACK:
				ctx->state ^= TRACK_IS_MUTE;
				SetWindowText((HWND)lParam, ((ctx->state & TRACK_IS_MUTE) ? L"MUTE" : L"Mute"));
				InvalidateRect(hWnd, nullptr, FALSE);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_NCCREATE:
			// allocate context
			ctx = (TrackContext*)malloc(sizeof(TrackContext));
			memset(ctx, 0, sizeof(TrackContext));
			mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
			// sixty seconds worth of samples
			ctx->state = 0;
			ctx->channelIndex = 0;
			ctx->monobuffershort = (short*)malloc(SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);
			memset(ctx->monobuffershort, 0, SAMPLES_PER_SEC * sizeof(short) * mctx->rec_time_seconds);

			// fill it with a 440 hz sine wave
			for (UINT32 sample = 0; sample < SAMPLES_PER_SEC * mctx->rec_time_seconds; sample++) {
				//(((FRAME*)ctx->buffer) + f)->left = (short)(sinf((float)f / ((float)SAMPLES_PER_SEC / 440.0f) * 2.0f * 3.14159f) * 32767.0f);
				//(((FRAME*)ctx->buffer) + f)->right = (((FRAME*)ctx->buffer) + f)->left; 
				ctx->monobuffershort[sample] = (short)(sinf((float)sample / ((float)SAMPLES_PER_SEC / 440.0f) * 2.0f * 3.14159f) * 32767.0f);
			}

			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
			return TRUE;
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
			SetWindowPos(hWnd, nullptr, 0, 0, LOWORD(lParam), 128, SWP_NOMOVE|SWP_NOZORDER);
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
		wcctl.style = CS_HREDRAW | CS_VREDRAW;
		wcctl.cbWndExtra = sizeof(TrackContext*);  // <-- Here we are

		RegisterClass(&wcctl);
	}

	HWND create_track_window(HWND parent, LPCWSTR TrackName)
	{
		MainWindowContext* mctx = (MainWindowContext*)GetWindowLongPtr(parent, GWLP_USERDATA);
		RECT r = { 0 };

		UINT32 idx = 0;
		while (mctx->tracks[idx] != nullptr) idx++;

		GetWindowRect(parent, &r);
		HWND hwnd = CreateWindow(L"CXTrackWindowClass", L"TrackName",
			WS_CHILD | WS_VISIBLE,
			0, (idx * 128) + 32, r.right - r.left, 128,
			parent, nullptr, GetModuleHandle(nullptr), nullptr);

		// create some controls here
		CXCommon::CreateButton(hwnd, 0, 0, 48, 32, L"Rec", BTN_ARM_REC);
		CXCommon::CreateButton(hwnd, 0, 32, 48, 32, L"Mute", BTN_MUTE_TRACK);

		return hwnd;
	}

	TrackContext* get_track_context(HWND cwnd) {
		return (TrackContext*)GetWindowLongPtr(cwnd, GWLP_USERDATA);
	}

}
