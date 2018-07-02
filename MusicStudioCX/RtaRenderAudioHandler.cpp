#include "stdafx.h"
#include "RtaRenderAudioHandler.h"

extern HANDLE g_RtwqStop;
extern DWORD g_RtwqId;

// initialize the audio handler
// create an event for rtwq
RtaRenderAudioHandler::RtaRenderAudioHandler()
{
	this->m_cRef = 0;
	this->BufferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->Priority = 1;
	this->workItemKey = NULL;
	this->pAsyncResult = NULL;
	this->pAudioRenderClient = NULL;
	this->pRenBuffer = NULL;
	this->FrameCount = 0;
	this->flags = 0;
	this->lpRenderDeviceInfo = NULL;
	this->pHandler = NULL;
	ZeroMemory(&this->hdlrCtx, sizeof(MusicStudioCommon::HANDLER_CONTEXT));
}

// cleanup the audio handler
RtaRenderAudioHandler::~RtaRenderAudioHandler()
{
	if (this->pAudioRenderClient != NULL) pAudioRenderClient->Release();
	if (this->BufferEvent != NULL) CloseHandle(this->BufferEvent);
	if (this->pAsyncResult != NULL) this->pAsyncResult->Release();
	this->m_cRef = 0;
}

// qi for COM
HRESULT RtaRenderAudioHandler::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;
	if (riid == IID_IUnknown || riid == __uuidof(IRtwqAsyncCallback))
	{
		// Increment the reference count and return the pointer.
		*ppvObj = (LPVOID)this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

// addref for COM
ULONG RtaRenderAudioHandler::AddRef()
{
	InterlockedIncrement(&this->m_cRef);
	return this->m_cRef;
}

// release for COM
ULONG RtaRenderAudioHandler::Release()
{
	// Decrement the object's internal counter.
	ULONG ulRefCount = InterlockedDecrement(&this->m_cRef);
	if (0 == this->m_cRef)
	{
		delete this;
	}
	return ulRefCount;
}

// From the IRtwqAsyncCallback docs:
// Provides configuration information to the dispatching thread for a callback.
// pdwFlags: 
//		Receives a flag indicating the behavior of the callback object's 
//		IRtwqAsyncCallback::Invoke method. The following values are defined. 
//		The default value is zero.
// pdwQueue:
//		Receives the identifier of the work queue on which the callback is dispatched. 
STDMETHODIMP RtaRenderAudioHandler::GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
{
	HRESULT hr = S_OK;
	*pdwFlags = 0;
	*pdwQueue = g_RtwqId;
	return hr;
}

// From the IRtwqAsyncCallback docs:
// Called when an asynchronous operation is completed.
// pAsyncResult:
//		Pointer to the IRtwqAsyncResult interface. Pass this pointer to the asynchronous End... method to complete the asynchronous call.
STDMETHODIMP RtaRenderAudioHandler::Invoke(IRtwqAsyncResult* pAsyncResult)
{
	BOOL handlerResult = FALSE;

	this->FrameCount = lpRenderDeviceInfo->BufferSizeFrames;

	// run data through handler
	//if (this->pHandler != NULL) {
		//pHandler(this->lpRenderDeviceInfo->FrameBuffer, lpRenderDeviceInfo->BufferSizeFrames, &handlerResult);
	//}
	//else {
		// if no handler, just zero the buffer
		//memset(this->lpRenderDeviceInfo->FrameBuffer, 0, this->FrameCount * lpRenderDeviceInfo->SizeOfFrame);
	//}

	// From the docs:
	// gets the the amount of valid, unread data that the endpoint buffer currently contains
	// basically, the amount of data that has not been rendered yet
	UINT32 PaddingInFrames = 0;
	ThrowIfFailed(this->lpRenderDeviceInfo->pAudioClient->GetCurrentPadding(&PaddingInFrames));

	// the amount of available frames to copy new data into
	// is the buffer size minus the padding
	UINT32 avail = this->lpRenderDeviceInfo->RealBufferSizeFrames - PaddingInFrames;

	if (this->FrameCount <= avail) {

		ZeroMemory(this->lpRenderDeviceInfo->FrameBufferByte, this->FrameCount * this->lpRenderDeviceInfo->SizeOfFrame);

		hdlrCtx.DataToRenderBuffer = this->lpRenderDeviceInfo->FrameBufferByte;
		hdlrCtx.frameCount = this->FrameCount;
		hdlrCtx.audioDeviceType = MusicStudioCommon::AudioDeviceType::AUDIO_DEVICE_WASAPI;
		pHandler(&hdlrCtx, &handlerResult);

		// get buffer from the render client
		ThrowIfFailed(pAudioRenderClient->GetBuffer(this->FrameCount, &this->pRenBuffer));

		// copy capture device buffer into render buffer
		memcpy(this->pRenBuffer, this->lpRenderDeviceInfo->FrameBufferByte,
			this->FrameCount * this->lpRenderDeviceInfo->SizeOfFrame);

		// release the render buffer
		ThrowIfFailed(this->pAudioRenderClient->ReleaseBuffer(this->FrameCount, 0));

		//else if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
		// requested buffer is too large
		// not enough available space
		//}
	}

	// break if done
	if (TRUE == handlerResult) {
		// stop
		goto err;
	}
	else {
		// queue up another item
		this->PutWaitingWorkItem();
	}
	return S_OK;

err:
	// if error, set the stop event
	// will trigger if handler returns TRUE (it is done)
	SetEvent(g_RtwqStop);
	return S_OK;
}

HRESULT RtaRenderAudioHandler::CreateAsyncResult()
{
	return RtwqCreateAsyncResult(NULL, this, NULL, &(this->pAsyncResult));
}

HRESULT RtaRenderAudioHandler::PutWaitingWorkItem()
{
	return RtwqPutWaitingWorkItem(
		this->BufferEvent,
		this->Priority,
		this->pAsyncResult,
		&this->workItemKey);
}
