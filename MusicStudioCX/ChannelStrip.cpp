
#include "stdafx.h"

#define MIXCHLEFT 16
#define MIXCHRIGHT 17

namespace ChannelStrip
{

	HWND hwndStripDialog = nullptr;

	INT_PTR CALLBACK ChStripDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	UINT32 PAN_ID_ARRAY[16] = {
		IDC_CH1PAN, IDC_CH2PAN,IDC_CH3PAN,IDC_CH4PAN,
		IDC_CH5PAN,IDC_CH6PAN,IDC_CH7PAN,IDC_CH8PAN,
		IDC_CH9PAN,IDC_CH10PAN,IDC_CH11PAN,IDC_CH12PAN,
		IDC_CH13PAN,IDC_CH14PAN,IDC_CH15PAN,IDC_CH16PAN
	};

	UINT32 LEVEL_ID_ARRAY[18] = {
		IDC_CH1SLIDER, IDC_CH2SLIDER,IDC_CH3SLIDER,IDC_CH4SLIDER,
		IDC_CH5SLIDER,IDC_CH6SLIDER,IDC_CH7SLIDER,IDC_CH8SLIDER,
		IDC_CH9SLIDER,IDC_CH10SLIDER,IDC_CH11SLIDER,IDC_CH12SLIDER,
		IDC_CH13SLIDER,IDC_CH14SLIDER,IDC_CH15SLIDER,IDC_CH16SLIDER,
		IDC_MIXLSLIDER, IDC_MIXRSLIDER
	};

	UINT32 METER_ID_ARRAY[18] = {
		IDC_CH1LEVEL, IDC_CH2LEVEL,IDC_CH3LEVEL,IDC_CH4LEVEL,
		IDC_CH5LEVEL,IDC_CH6LEVEL,IDC_CH7LEVEL,IDC_CH8LEVEL,
		IDC_CH9LEVEL,IDC_CH10LEVEL,IDC_CH11LEVEL,IDC_CH12LEVEL,
		IDC_CH13LEVEL,IDC_CH14LEVEL,IDC_CH15LEVEL,IDC_CH16LEVEL,
		IDC_MIXLLEVEL, IDC_MIXRLEVEL
	};

	void RequestUpdateMeters(short LeftMeterValue, short RightMeterValue)
	{
		if (hwndStripDialog != nullptr) {
			SendMessage(hwndStripDialog, WM_UPDATE_CHSTRIP, 0, MAKELONG(LeftMeterValue, RightMeterValue));
		}
	}

	void UpdateMeters(short LeftMeterValue, short RightMeterValue)
	{
		if (hwndStripDialog != nullptr) {
			if (IsWindowVisible(hwndStripDialog)) {
				HWND hParent = GetParent(hwndStripDialog);
				MusicStudioCX::MainWindowContext* mctx =
					(MusicStudioCX::MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
				for (int t = 0; t < 16; t++) {
					TrackContext *tc = mctx->TrackContextList[t];
					SendMessage(GetDlgItem(hwndStripDialog, METER_ID_ARRAY[t]), PBM_SETPOS, tc->MeterVal, 0);
				}
				SendMessage(GetDlgItem(hwndStripDialog, METER_ID_ARRAY[MIXCHLEFT]), PBM_SETPOS, LeftMeterValue, 0);
				SendMessage(GetDlgItem(hwndStripDialog, METER_ID_ARRAY[MIXCHRIGHT]), PBM_SETPOS, RightMeterValue, 0);
			}
		}
	}

	void ToggleChannelStrip(HWND hParent) {

		BOOL update = FALSE;
		if (hwndStripDialog == nullptr) {
			hwndStripDialog = CreateDialog(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_CHSTRIPDLG), hParent, ChStripDialogProc);
			ShowWindow(hwndStripDialog, SW_SHOW);
			update = TRUE;
		}
		else {
			if (IsWindowVisible(hwndStripDialog)) {
				ShowWindow(hwndStripDialog, SW_HIDE);
			}
			else {
				ShowWindow(hwndStripDialog, SW_SHOW);
				update = TRUE;
			}
		}

		if (TRUE == update) {
			MusicStudioCX::MainWindowContext* mctx = 
				(MusicStudioCX::MainWindowContext*)GetWindowLongPtr(hParent, GWLP_USERDATA);
			for (int t = 0; t < 16; t++) {
				TrackContext *tc = mctx->TrackContextList[t];
				UINT32 SliderPos = 0;
				// pan value
				// left full is l=1, r=0
				// center is l=1, r=1
				// right full is l=0, r=1
				// Slider should go from 0 to 200
				if (tc->leftpan < 1.0f) {
					// pan right
					// 100 to 200
					SliderPos = (LPARAM)((1.0f - tc->leftpan) * 100.0f) + 100;
				}
				else {
					// pan left 0 to 100
					SliderPos = (LPARAM)(tc->rightpan * 100.0f);
				}
				SendMessage(GetDlgItem(hwndStripDialog, PAN_ID_ARRAY[t]), TBM_SETRANGE, FALSE, MAKELONG(0, 200));
				SendMessage(GetDlgItem(hwndStripDialog, PAN_ID_ARRAY[t]), TBM_SETTICFREQ, 20, 0);
				SendMessage(GetDlgItem(hwndStripDialog, PAN_ID_ARRAY[t]), TBM_SETPOS, TRUE, SliderPos);

				SliderPos = (LPARAM)((1.0f - tc->volume) * 100.0f);
				SendMessage(GetDlgItem(hwndStripDialog, LEVEL_ID_ARRAY[t]), TBM_SETRANGE, FALSE, MAKELONG(0, 100));
				SendMessage(GetDlgItem(hwndStripDialog, LEVEL_ID_ARRAY[t]), TBM_SETTICFREQ, 10, 0);
				SendMessage(GetDlgItem(hwndStripDialog, LEVEL_ID_ARRAY[t]), TBM_SETPOS, TRUE, SliderPos);
			}
		}
	}

	void HideChannelStrip()
	{
		if(hwndStripDialog) ShowWindow(hwndStripDialog, SW_HIDE);
	}

	INT_PTR CALLBACK ChStripDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
			for (int t = 0; t < 18; t++) {
				SendMessage(GetDlgItem(hDlg, METER_ID_ARRAY[t]), PBM_SETRANGE, 0, MAKELONG(0, 32767));
			}
			return (INT_PTR)TRUE;
		case WM_UPDATE_CHSTRIP:
			UpdateMeters(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_COMMAND:
			break;
		}
		return (INT_PTR)FALSE;
	}

}