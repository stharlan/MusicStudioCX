#pragma once

typedef struct {
	LPWSTR DeviceName;
	LPWSTR DeviceId;
	BYTE* FrameBuffer;
	IMMDevice *pMMDevice;
	IAudioClient *pAudioClient;
	LPVOID pNext;
	UINT32 BufferSizeFrames;
	UINT32 RealBufferSizeFrames;
	VARIANT_BOOL IsRawSupported;
	DWORD SizeOfFrame;
} RTA_DEVICE_INFO, *LPRTA_DEVICE_INFO;

typedef void(*RTA_DATA_HANDLER)(BYTE* buffer, UINT32 frameCount, BOOL* Cancel);

const char* rta_get_last_error();

LPVOID rta_alloc(SIZE_T size);

void rta_free(LPVOID pvoid);

UINT32 rta_list_supporting_devices_2(RTA_DEVICE_INFO** lppDeviceInfo,
	WAVEFORMATEX *RequestedFormat,
	DWORD StateMask = DEVICE_STATE_ACTIVE,
	EDataFlow DataFlow = eCapture,
	AUDCLNT_SHAREMODE ShareMode = AUDCLNT_SHAREMODE_EXCLUSIVE
);

void rta_free_device_list(LPRTA_DEVICE_INFO lpDeviceInfo);

BOOL rta_initialize_device_2(LPRTA_DEVICE_INFO lpDeviceInfo, DWORD StreamFlags, WAVEFORMATEX* RequestedFormat = nullptr);

void rta_capture_frames_rtwq(LPRTA_DEVICE_INFO lpCaptureDeviceInfo,
	LPRTA_DEVICE_INFO lpRenderDeviceInfo, RTA_DATA_HANDLER pHandler);

void rta_render_frames_rtwq(LPRTA_DEVICE_INFO lpRenderDeviceInfo, RTA_DATA_HANDLER pHandler);