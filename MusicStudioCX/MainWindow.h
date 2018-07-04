#pragma once

#define MAIN_WINDOW_HEADER_HEIGHT 96
#define MAIN_WINDOW_TIMEBAR_OFFSET 64
#define WVFRM_OFFSET 56


namespace MainWindow
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
		RtaImpl::LPRTA_DEVICE_INFO CaptureDevInfo = nullptr;
		RtaImpl::LPRTA_DEVICE_INFO RenderDevInfo = nullptr;
		ASIO_DEVICE_INFO* AsioDevInfo = nullptr;
		AUDIO_DEVICE_TYPE adt;
		wchar_t WavFileName[1024];
		std::wstring ProjectDir;
		BOOL snap_selection;
	} MainWindowContext;

	void initialize_main_window(HINSTANCE hInst);
	HWND create_main_window();
	void reposition_all_tracks();
	void RegisterHotKeys(HWND hwnd);
	void SetTrackSelectionMsg();
	//void RedrawAllTracksTrack(HWND twnd);
}