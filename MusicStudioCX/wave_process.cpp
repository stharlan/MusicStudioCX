
#include "stdafx.h"

namespace WaveProcess {

	void wp_maximize(MainWindow::MainWindowContext* mctx, TrackContext* tctx, HWND progressBarWindow,
		UINT32 subselfrom, UINT32 subselto) {

		// find the max
		int max_val = 0;
		int this_val = 0;

		UINT32 sframe = 0;
		UINT32 eframe = mctx->max_frames;

		if (subselto > subselfrom) {
			sframe = subselfrom;
			eframe = subselto;
			if (eframe > mctx->max_frames) eframe = mctx->max_frames;
		}

		for (UINT32 frame = sframe; frame < eframe; frame++) {
			this_val = abs(tctx->monobuffershort[frame]);
			if (this_val > max_val) max_val = this_val;
		}

		// multiply values
		for (UINT32 frame = sframe; frame < eframe; frame++) {
			this_val = tctx->monobuffershort[frame];
			this_val = MulDiv(this_val, SHRT_MAX, max_val);
			tctx->monobuffershort[frame] = (short)this_val;
		}

		//SendMessage(progressBarWindow, PBM_SETPOS, MulDiv(FrameIndex, 100, SamplesPerSec * RecordingTimeSeconds), 0);
	}

}