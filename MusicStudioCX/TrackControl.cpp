
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
			ctx->max_frames = SAMPLES_PER_SEC * mctx->RecTimeSeconds;
			ctx->frame_offset = 0;
			ctx->buffer = (BYTE*)malloc(SAMPLES_PER_SEC * FRAME_SIZE * mctx->RecTimeSeconds);
			memset(ctx->buffer, 0, SAMPLES_PER_SEC * FRAME_SIZE * mctx->RecTimeSeconds);

			// fill it with a 440 hz sine wave
			for (int f = 0; f < SAMPLES_PER_SEC * mctx->RecTimeSeconds; f++) {
				(((FRAME*)ctx->buffer) + f)->left = (short)(sinf((float)f / ((float)SAMPLES_PER_SEC / 440.0f) * 2.0f * 3.14159f) * 32767.0f);
				(((FRAME*)ctx->buffer) + f)->right = (((FRAME*)ctx->buffer) + f)->left; 
			}

			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
			return TRUE;
		case WM_NCDESTROY:
			// free context
			ctx = GetTrackContext(hWnd);
			if (ctx->buffer) free(ctx->buffer);
			if (ctx) free(ctx);
			break;
		case WM_PAINT:
		{
			mctx = (MainWindowContext*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ctx = GetTrackContext(hWnd);
			HPEN GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
			HGDIOBJ obj = SelectObject(hdc, GreenPen);
			FRAME* pFrame = (FRAME*)ctx->buffer;
			MoveToEx(hdc, 0, 64, nullptr);
			for (UINT32 FrameCount = 0; FrameCount < ctx->max_frames; FrameCount += mctx->ZoomMult) {
				LineTo(hdc, FrameCount / mctx->ZoomMult, (pFrame[FrameCount].left / 512) + 64);
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
		RECT r = { 0 };
		GetWindowRect(parent, &r);
		HWND hwnd = CreateWindow(L"CXTrackWindowClass", L"TrackName",
			WS_CHILD | WS_VISIBLE,
			0, 32, r.right - r.left, 128,
			parent, nullptr, GetModuleHandle(nullptr), nullptr);
		return hwnd;
	}

	TrackContext* GetTrackContext(HWND cwnd) {
		return (TrackContext*)GetWindowLongPtr(cwnd, GWLP_USERDATA);
	}

}
