#pragma once

namespace MusicStudioCX
{

	typedef struct {
		UINT32 RecTimeSeconds;
		UINT32 ZoomMult;
	} MainWindowContext;

	void initialize_main_window();
	HWND create_main_window();

}