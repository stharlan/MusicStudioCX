#pragma once
class RtaRenderAudioHandler : public IRtwqAsyncCallback
{
public:
	RtaRenderAudioHandler();
	~RtaRenderAudioHandler();

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue);
	STDMETHODIMP Invoke(IRtwqAsyncResult* pAsyncResult);

	HANDLE GetBufferEventHandle() { return this->BufferEvent; }
	HRESULT CreateAsyncResult();
	HRESULT PutWaitingWorkItem();
	void ConfigureClientInformation(
		IAudioRenderClient *parmAudioRenderClient,
		RtaImpl::LPRTA_DEVICE_INFO parmRenderDeviceInfo,
		MusicStudioCommon::RTA_DATA_HANDLER parmDataHandlerCallback)
	{
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

	IAudioRenderClient *pAudioRenderClient;

	BYTE* pRenBuffer;
	UINT32 FrameCount;
	DWORD flags;

	RtaImpl::LPRTA_DEVICE_INFO lpRenderDeviceInfo;
	MusicStudioCommon::RTA_DATA_HANDLER pHandler;
	MusicStudioCommon::HANDLER_CONTEXT hdlrCtx;
};

