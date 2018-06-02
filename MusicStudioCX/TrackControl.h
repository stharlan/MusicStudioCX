#pragma once

namespace MusicStudioCX
{

	typedef struct {
		BYTE* buffer;
	} TrackContext;

	void initialize_track_window();
	HWND create_track_window(HWND parent, LPCWSTR TrackName);

}