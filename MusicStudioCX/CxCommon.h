#pragma once

enum AudioDeviceType {
	AUDIO_DEVICE_WASAPI,
	AUDIO_DEVICE_ASIO
};

typedef struct {
	BYTE* CapturedDataBuffer;
	BYTE* DataToRenderBuffer;
	UINT32 frameCount;
	UINT32 LastFrameCounts[3];
	AudioDeviceType audioDeviceType;
} HANDLER_CONTEXT;

typedef void(*RTA_DATA_HANDLER)(HANDLER_CONTEXT* lpHandlerContext, BOOL* lpCancel);

namespace CXCommon {

	HWND CreateButton(HWND hWnd, int x, int y, int w, int h, const wchar_t* text, DWORD btnId, DWORD AddlStyles = 0);

}
