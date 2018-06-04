#pragma once

namespace MusicStudioCX
{

	typedef struct {
		UINT32 rec_time_seconds;
		UINT32 zoom_mult;
		UINT32 frame_offset;
		UINT32 max_frames;
		UINT32 scroll_pos;
		TrackContext* TrackContextList[16];
	} MainWindowContext;

	void initialize_main_window();
	HWND create_main_window();

}