
#define ASIO_CHAR_BUFFER_LENGTH 1024

typedef struct {
	WCHAR wcDriverName[ASIO_CHAR_BUFFER_LENGTH];
	WCHAR wcClsid[ASIO_CHAR_BUFFER_LENGTH];
	UINT32 AsioDevInfoId;
	IASIO* pAsio;
	void* lpvNext;

	//long minSize;
	//long maxSize;
	//long PrefferedBufferSizeSamples;
	//long granularity;
	//long NumInCh;
	//long NumOutCh;
	//ASIOChannelInfo* InputChannelInfo;
	//ASIOChannelInfo* OutputChannelInfo;
	//UINT32 SizeOfBigBuffer;
	//BYTE* BigBuffer;
	//ASIOBufferInfo* BufferInfo;
	//HANDLE hQuitEvent;
	//RTA_DATA_HANDLER pHandler;
} ASIO_DEVICE_INFO;

namespace CXASIO {
	UINT32 asio_list_supporting_devices(ASIO_DEVICE_INFO** lppDevInfo);
	void asio_free_device_list(ASIO_DEVICE_INFO* lpDevInfo);
	int asio_start(ASIO_DEVICE_INFO *pDevice, MusicStudioCommon::RTA_DATA_HANDLER pHandler, BOOL IsRecording);
	//BOOL asio_initialize_device(ASIO_DEVICE_INFO* lpDevInfo);
	//void asio_render_frames(ASIO_DEVICE_INFO* lpRenderDeviceInfo, RTA_DATA_HANDLER pHandler);
	//void asio_capture_frames_rtwq();
	//rta_render_frames_rtwq
}
