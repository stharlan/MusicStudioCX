#pragma once

typedef struct {
	LPWSTR DeviceName;
	LPWSTR DeviceId;
	BYTE* FrameBufferByte;
	IMMDevice *pMMDevice;
	IAudioClient *pAudioClient;
	LPVOID pNext;
	UINT32 BufferSizeFrames;
	UINT32 RealBufferSizeFrames;
	VARIANT_BOOL IsRawSupported;
	DWORD SizeOfFrame;
	WAVEFORMATEX WaveFormat;
	UINT32 RtaDevInfoId;
} RTA_DEVICE_INFO, *LPRTA_DEVICE_INFO;

const char* rta_get_last_error();

//LPVOID rta_alloc(SIZE_T size);

//void rta_free(LPVOID pvoid);

UINT32 rta_list_supporting_devices_2(RTA_DEVICE_INFO** lppDeviceInfo,
	WAVEFORMATEX *RequestedFormat,
	DWORD StateMask, EDataFlow DataFlow, AUDCLNT_SHAREMODE ShareMode,
	BOOL TestForChannels);

void rta_free_device_list(LPRTA_DEVICE_INFO lpDeviceInfo);

BOOL rta_initialize_device_2(LPRTA_DEVICE_INFO lpDeviceInfo, DWORD StreamFlags);

void rta_capture_frames_rtwq(LPRTA_DEVICE_INFO lpCaptureDeviceInfo,
	LPRTA_DEVICE_INFO lpRenderDeviceInfo, RTA_DATA_HANDLER pHandler);

void rta_render_frames_rtwq(LPRTA_DEVICE_INFO lpRenderDeviceInfo, RTA_DATA_HANDLER pHandler);