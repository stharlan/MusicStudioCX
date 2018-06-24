#include "stdafx.h"
#include "RtaAudioHandler.h"

extern HANDLE g_RtwqStop;
extern DWORD g_RtwqId;

// initialize the audio handler
// create an event for rtwq
RtaAudioHandler::RtaAudioHandler()
{
	this->m_cRef = 0;
	this->BufferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->Priority = 1;
	this->workItemKey = NULL;
	this->pAsyncResult = NULL;
	this->pAudioCaptureClient = NULL;
	this->pAudioRenderClient = NULL;
	this->pCapBuffer = NULL;
	this->pRenBuffer = NULL;
	this->FrameCount = 0;
	this->flags = 0;
	this->lpCaptureDeviceInfo = NULL;
	this->lpRenderDeviceInfo = NULL;
	this->pHandler = NULL;
	ZeroMemory(&this->hdlrCtx, sizeof(HANDLER_CONTEXT));
}

// cleanup the audio handler
RtaAudioHandler::~RtaAudioHandler()
{
	if (this->pAudioCaptureClient != NULL) pAudioCaptureClient->Release();
	if (this->pAudioRenderClient != NULL) pAudioRenderClient->Release();
	if (this->BufferEvent != NULL) CloseHandle(this->BufferEvent);
	if (this->pAsyncResult != NULL) this->pAsyncResult->Release();
	this->m_cRef = 0;
}

// qi for COM
HRESULT RtaAudioHandler::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
ULONG RtaAudioHandler::AddRef()
{
	InterlockedIncrement(&this->m_cRef);
	return this->m_cRef;
}

// release for COM
ULONG RtaAudioHandler::Release()
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
STDMETHODIMP RtaAudioHandler::GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
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
STDMETHODIMP RtaAudioHandler::Invoke(IRtwqAsyncResult* pAsyncResult)
{
	BOOL handlerResult = FALSE;

	{

		// get capture buffer from the audio capture client
		HRESULT hr = this->pAudioCaptureClient->GetBuffer(
			&(this->pCapBuffer), &(this->FrameCount), &(this->flags), NULL, NULL);
		if (FAILED(hr)) {
			goto err;
		}

		// copy data from audio client capture buffer
		// to the capture device info frame buffer
		memcpy(this->lpCaptureDeviceInfo->FrameBufferByte, this->pCapBuffer,
			this->FrameCount * this->lpCaptureDeviceInfo->SizeOfFrame);

		// release capture buffer
		hr = this->pAudioCaptureClient->ReleaseBuffer(this->FrameCount);
		if (FAILED(hr)) {
			goto err;
		}

	}

	// run data through handler
	if (this->pHandler != NULL) {
		hdlrCtx.CapturedDataBuffer = this->lpCaptureDeviceInfo->FrameBufferByte;
		hdlrCtx.DataToRenderBuffer = this->lpRenderDeviceInfo->FrameBufferByte;
		hdlrCtx.frameCount = this->FrameCount;
		hdlrCtx.audioDeviceType = AudioDeviceType::AUDIO_DEVICE_WASAPI;
		pHandler(&hdlrCtx, &handlerResult);
		hdlrCtx.LastFrameCounts[2] = hdlrCtx.LastFrameCounts[1];
		hdlrCtx.LastFrameCounts[1] = hdlrCtx.LastFrameCounts[0];
		hdlrCtx.LastFrameCounts[0] = this->FrameCount;
	}

	// if rendering
	if (this->pAudioRenderClient != NULL) {

		// From the docs:
		// gets the the amount of valid, unread data that the endpoint buffer currently contains
		// basically, the amount of data that has not been rendered yet
		UINT32 PaddingInFrames = 0;
		this->lpRenderDeviceInfo->pAudioClient->GetCurrentPadding(&PaddingInFrames);

		// the amount of available frames to copy new data into
		// is the buffer size minus the padding
		UINT32 avail = this->lpRenderDeviceInfo->RealBufferSizeFrames - PaddingInFrames;

		if (this->FrameCount <= avail) {

			// get buffer from the render client
			HRESULT hr = pAudioRenderClient->GetBuffer(this->FrameCount, &this->pRenBuffer);

			if (SUCCEEDED(hr)) {

				// copy capture device buffer into render buffer
				memcpy(this->pRenBuffer, this->lpRenderDeviceInfo->FrameBufferByte,
					this->FrameCount * this->lpCaptureDeviceInfo->SizeOfFrame);

				// release the render buffer
				this->pAudioRenderClient->ReleaseBuffer(this->FrameCount, 0);
			}
			//else if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
				// requested buffer is too large
				// not enough available space
			//}
		}
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

HRESULT RtaAudioHandler::CreateAsyncResult()
{
	return RtwqCreateAsyncResult(NULL, this, NULL, &(this->pAsyncResult));
}

HRESULT RtaAudioHandler::PutWaitingWorkItem()
{
	return RtwqPutWaitingWorkItem(
		this->BufferEvent,
		this->Priority,
		this->pAsyncResult,
		&this->workItemKey);
}
