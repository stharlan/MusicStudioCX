#pragma once

class RtaAudioHandler : public IRtwqAsyncCallback
{
public:
	RtaAudioHandler();
	~RtaAudioHandler();

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue);
	STDMETHODIMP Invoke(IRtwqAsyncResult* pAsyncResult);

	HANDLE GetBufferEventHandle() { return this->BufferEvent; }
	HRESULT CreateAsyncResult();
	HRESULT PutWaitingWorkItem();
	void ConfigureClientInformation(
		IAudioCaptureClient *parmAudioCaptureClient,
		RtaImpl::LPRTA_DEVICE_INFO parmCaptureDeviceInfo,
		IAudioRenderClient *parmAudioRenderClient,
		RtaImpl::LPRTA_DEVICE_INFO parmRenderDeviceInfo,
		MusicStudioCommon::RTA_DATA_HANDLER parmDataHandlerCallback)
	{
		this->pAudioCaptureClient = parmAudioCaptureClient;
		this->pAudioCaptureClient->AddRef();
		this->lpCaptureDeviceInfo = parmCaptureDeviceInfo;
		this->pHandler = parmDataHandlerCallback;
		this->pAudioRenderClient = parmAudioRenderClient;
		if (this->pAudioRenderClient != NULL)
			this->pAudioRenderClient->AddRef();
		this->lpRenderDeviceInfo = parmRenderDeviceInfo;
	}

private:
	volatile ULONG m_cRef;
	HANDLE BufferEvent;
	LONG Priority;
	RTWQWORKITEM_KEY workItemKey;
	IRtwqAsyncResult *pAsyncResult;

	IAudioCaptureClient *pAudioCaptureClient;
	IAudioRenderClient *pAudioRenderClient;

	BYTE* pCapBuffer;
	BYTE* pRenBuffer;
	UINT32 FrameCount;
	DWORD flags;

	RtaImpl::LPRTA_DEVICE_INFO lpCaptureDeviceInfo;
	RtaImpl::LPRTA_DEVICE_INFO lpRenderDeviceInfo;
	MusicStudioCommon::RTA_DATA_HANDLER pHandler;
	MusicStudioCommon::HANDLER_CONTEXT hdlrCtx;

};


