#pragma once

#define MAX_LOADSTRING 100
#define STATUS_BAR_ID 9801
#define SAMPLES_PER_SEC 48000
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE BITS_PER_SAMPLE / 8
#define SAMPLE_SIZE BITS_PER_SAMPLE / 8
#define NUM_CHANNELS 2
#define BLOCK_ALIGN (NUM_CHANNELS * BITS_PER_SAMPLE) / 8
#define FRAME_SIZE BLOCK_ALIGN
#define AVG_BYTES_PER_SEC BLOCK_ALIGN * SAMPLES_PER_SEC

typedef struct {
	short* monobuffershort;
	short channelIndex;
	UINT32 state;
	HWND TrackWindow;
} TrackContext;

typedef struct {
	short channel[2];
} FRAME2CHSHORT;

typedef short SAMPLESHORT;

namespace MusicStudioCX
{

	void initialize_track_window();
	TrackContext* create_track_window_a(HWND parent, LPCWSTR TrackName, short channel);
	TrackContext* get_track_context(HWND cwnd);
	void generate_sine(float frequency, UINT32 seconds, short* buffer, float max_amplitude = 1.0f);

}