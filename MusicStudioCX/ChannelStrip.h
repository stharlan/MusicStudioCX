#pragma once

namespace ChannelStrip
{
	void ToggleChannelStrip(HWND hParent, HINSTANCE hInst);
	void RequestUpdateMeters(short LeftMeterValue, short RightMeterValue);
}