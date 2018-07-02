#pragma once

#define MAX_LOADSTRING 100
#define SAMPLES_PER_SEC 48000
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE BITS_PER_SAMPLE / 8
#define SAMPLE_SIZE BITS_PER_SAMPLE / 8
#define NUM_CHANNELS 2
#define BLOCK_ALIGN (NUM_CHANNELS * BITS_PER_SAMPLE) / 8
#define FRAME_SIZE BLOCK_ALIGN
#define AVG_BYTES_PER_SEC BLOCK_ALIGN * SAMPLES_PER_SEC
#define REC_TIME_SECONDS 5 * 60
#define NUM_TRACKS 16

#define TRACK_STATE_SELECTED 0
#define TRACK_STATE_MINIMIZED 1
#define TRACK_STATE_MUTE 2
#define TRACK_STATE_ARMED 3
//number |= 1UL << x;
#define SET_BIT(s,b) (s |= 1UL << b)
//number &= ~(1UL << x);
#define CLEAR_BIT(s,b) (s &= ~(1UL << b))
//number ^= 1UL << x;
#define TOGGLE_BIT(s,b) (s ^= 1UL << b)
//bit = (number >> x) & 1U;
#define CHECK_BIT(s,b) ((s >> b) & 1UL)

typedef struct {
	short* monobuffershort;
	short InputChannelIndex;
	float leftpan;
	float rightpan;
	float volume;
	HWND PrevTrackWindow;
	HWND TrackWindow;
	HWND NextTrackWindow;
	UINT32 TrackIndex;
	HWND buttons[3];
	short MeterVal;
	ULONG wstate = 0;
} TrackContext;

typedef struct {
	short channel[2];
} FRAME2CHSHORT;

typedef short FRAME1CHSHORT;

typedef short SAMPLESHORT;

namespace TrackControl
{

	void initialize_track_window(HINSTANCE hInst);
	TrackContext* create_track_window_a(HWND parent, LPCWSTR TrackName, short channel);
	TrackContext* get_track_context(HWND cwnd);
	void generate_sine(float frequency, UINT32 seconds, short* buffer, float max_amplitude = 1.0f);
	inline void SetState(TrackContext* tctx, DWORD flag)
	{
		SET_BIT(tctx->wstate, flag); 
	}

	inline void ClearState(TrackContext* tctx, DWORD flag)
	{
		CLEAR_BIT(tctx->wstate, flag);
	}

	inline BOOL CheckState(TrackContext* tctx, DWORD flag)
	{
		return CHECK_BIT(tctx->wstate, flag) ? TRUE : FALSE;
	}

	inline BOOL ToggleState(TrackContext* tctx, DWORD flag) {
		TOGGLE_BIT(tctx->wstate, flag);
		return CHECK_BIT(tctx->wstate, flag) ? TRUE : FALSE;
	}

}