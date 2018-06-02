
#include "stdafx.h"

namespace MusicStudioCX
{

	LRESULT CALLBACK track_wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		TrackContext * ctx = nullptr;
		switch (message)
		{
		case WM_NCCREATE:
			// allocate context
			ctx = (TrackContext*)malloc(sizeof(TrackContext));
			// sixty seconds worth of samples
			ctx->max_frames = SAMPLES_PER_SEC * 3;
			ctx->frame_offset = 0;
			ctx->buffer = (BYTE*)malloc(SAMPLES_PER_SEC * FRAME_SIZE * 3);
			memset(ctx->buffer, 0, SAMPLES_PER_SEC * FRAME_SIZE * 3);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
			return TRUE;
		case WM_NCDESTROY:
			// free context
			ctx = (TrackContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (ctx->buffer) free(ctx->buffer);
			if (ctx) free(ctx);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ctx = (TrackContext*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			HPEN WhitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
			HGDIOBJ obj = SelectObject(hdc, WhitePen);
			FRAME* pFrame = (FRAME*)ctx->buffer;
			MoveToEx(hdc, 0, 64, nullptr);
			for (UINT32 FrameCount = 0; FrameCount < ctx->max_frames; FrameCount+=100) {
				LineTo(hdc, FrameCount / 100, (pFrame[FrameCount].left / 1024) + 64);
			}
			SelectObject(hdc, obj);
			DeleteObject(WhitePen);
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
			0, 0, r.right - r.left, 128,
			parent, nullptr, GetModuleHandle(nullptr), nullptr);
		return hwnd;
	}

}
