#pragma once

#define MAX_LOADSTRING 100
#define STATUS_BAR_ID 9801
#define SAMPLES_PER_SEC 48000
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 2
#define BLOCK_ALIGN (NUM_CHANNELS * BITS_PER_SAMPLE) / 8
#define FRAME_SIZE BLOCK_ALIGN
#define AVG_BYTES_PER_SEC BLOCK_ALIGN * SAMPLES_PER_SEC

typedef struct {
	BYTE* buffer;
} TrackContext;

typedef struct {
	short left;
	short right;
} FRAME;

namespace MusicStudioCX
{

	void initialize_track_window();
	HWND create_track_window(HWND parent, LPCWSTR TrackName);
	TrackContext* get_track_context(HWND cwnd);

}