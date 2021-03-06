// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <ShlObj.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include <exception>
#include <fstream>
#include <string>

#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <RTWorkQ.h>
#include <propkey.h>

#define WM_UPDATE_CHSTRIP WM_USER+1

#include <iasiodrv.h>
#include "CxCommon.h"
#include "asio_impl.h"
#include "rta_impl.h"
#include "RtaAudioHandler.h"
#include "RtaRenderAudioHandler.h"
#include "TrackControl.h"
#include "MainWindow.h"
#include "wave_process.h"
#include "ChannelStrip.h"
#include "MusicStudioCX.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <iostream>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch DirectX API errors
		throw std::exception();
	}
}

