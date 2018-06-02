
#include "stdafx.h"

namespace MusicStudioCX
{

	LRESULT CALLBACK track_wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		TrackContext * ctx = nullptr;
		MainWindowContext* mctx = nullptr;
		switch (message)
		{
		case WM_NCCREATE:
			// allocate context
			ctx = (TrackContext*)malloc(sizeof(TrackContext));
			mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
			// sixty seconds worth of samples
			ctx->buffer = (BYTE*)malloc(SAMPLES_PER_SEC * FRAME_SIZE * mctx->rec_time_seconds);
			memset(ctx->buffer, 0, SAMPLES_PER_SEC * FRAME_SIZE * mctx->rec_time_seconds);

			// fill it with a 440 hz sine wave
			for (UINT32 f = 0; f < SAMPLES_PER_SEC * mctx->rec_time_seconds; f++) {
				(((FRAME*)ctx->buffer) + f)->left = (short)(sinf((float)f / ((float)SAMPLES_PER_SEC / 440.0f) * 2.0f * 3.14159f) * 32767.0f);
				(((FRAME*)ctx->buffer) + f)->right = (((FRAME*)ctx->buffer) + f)->left; 
			}

			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
			return TRUE;
		case WM_NCDESTROY:
			// free context
			ctx = get_track_context(hWnd);
			if (ctx->buffer) free(ctx->buffer);
			if (ctx) free(ctx);
			break;
		case WM_PAINT:
		{
			mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
			RECT r;
			GetClientRect(hWnd, &r);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ctx = get_track_context(hWnd);
			HPEN GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
			HGDIOBJ obj = SelectObject(hdc, GreenPen);
			FRAME* pFrame = (FRAME*)ctx->buffer;
			MoveToEx(hdc, 0, 64, nullptr);

			// get scroll bar pos
			float pos = (float)mctx->scroll_pos;
			UINT32 StartFrame = (UINT32)((pos / 65535.0f) * (float)mctx->max_frames);
			UINT32 rpos = 0;
			for (UINT32 FrameCount = StartFrame; FrameCount < mctx->max_frames; FrameCount += mctx->zoom_mult) {
				rpos = (FrameCount - StartFrame) / mctx->zoom_mult;
				if (rpos > (UINT32)r.right) break;
				LineTo(hdc, rpos, (pFrame[FrameCount].left / 512) + 64);
			}
			SelectObject(hdc, obj);
			DeleteObject(GreenPen);
			EndPaint(hWnd, &ps);
		}
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
		return hwnd;
	}

	TrackContext* get_track_context(HWND cwnd) {
		return (TrackContext*)GetWindowLongPtr(cwnd, GWLP_USERDATA);
	}

}
