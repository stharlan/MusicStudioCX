#pragma once

enum CX_AUDIO_FORMAT {
	FMT_16_BIT_SIGNED,
	FMT_24_BIT_SIGNED
};

typedef struct {
	BYTE* capBuffer;
	BYTE* renBuffer;
	UINT32 frameCount;
	UINT32 LastFrameCounts[3];
	int ChannelIndex;
	CX_AUDIO_FORMAT fmt;
} HANDLER_CONTEXT;

typedef void(*RTA_DATA_HANDLER)(HANDLER_CONTEXT* lpHandlerContext, BOOL* lpCancel);

namespace CXCommon {

	HWND CreateButton(HWND hWnd, int x, int y, int w, int h, const wchar_t* text, DWORD btnId, DWORD AddlStyles = 0);

}
