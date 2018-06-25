
#include "stdafx.h"

namespace WaveProcess {

	void wp_maximize(MusicStudioCX::MainWindowContext* mctx, TrackContext* tctx, HWND progressBarWindow) {

		// find the max
		int max_val = 0;
		int this_val = 0;
		for (UINT32 frame = 0; frame < mctx->max_frames; frame++) {
			this_val = abs(tctx->monobuffershort[frame]);
			if (this_val > max_val) max_val = this_val;
		}

		// multiply values
		for (UINT32 frame = 0; frame < mctx->max_frames; frame++) {
			this_val = tctx->monobuffershort[frame];
			this_val = MulDiv(this_val, SHRT_MAX, max_val);
			tctx->monobuffershort[frame] = (short)this_val;
		}

		//SendMessage(progressBarWindow, PBM_SETPOS, MulDiv(FrameIndex, 100, SamplesPerSec * RecordingTimeSeconds), 0);
	}

}