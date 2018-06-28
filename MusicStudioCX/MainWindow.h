#pragma once

#define MAIN_WINDOW_HEADER_HEIGHT 64
#define WVFRM_OFFSET 56

namespace MusicStudioCX
{

	enum AUDIO_DEVICE_TYPE {
		AUDIO_DEVICE_WASAPI,
		AUDIO_DEVICE_ASIO
	};

	typedef struct {
		UINT32 rec_time_seconds;
		UINT32 zoom_mult;
		UINT32 frame_offset;
		UINT32 max_frames;
		UINT32 hscroll_pos;
		UINT32 vscroll_pos;
		UINT32 sel_begin_frame;
		UINT32 sel_end_frame;
		BOOL auto_position_timebar;
		TrackContext* TrackContextList[NUM_TRACKS];
		LPRTA_DEVICE_INFO CaptureDevInfo = nullptr;
		LPRTA_DEVICE_INFO RenderDevInfo = nullptr;
		ASIO_DEVICE_INFO* AsioDevInfo = nullptr;
		AUDIO_DEVICE_TYPE adt;
		wchar_t WavFileName[1024];
		std::wstring ProjectDir;
	} MainWindowContext;

	void initialize_main_window();
	HWND create_main_window();
	void reposition_all_tracks(MainWindowContext* mctx);
	void RegisterHotKeys(HWND hwnd);
}