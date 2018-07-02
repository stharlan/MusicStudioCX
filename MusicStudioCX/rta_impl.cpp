
#include "stdafx.h"

//#define CHKERR(HR,E) if (FAILED(HR)) { last_error = E; goto done; }
#define CHKERR(HR,E) ThrowIfFailed(HR)

HANDLE g_RtwqStop = NULL;
DWORD g_RtwqId = 0;

namespace RtaImpl {

	const char* ERROR_1 = "Failed to create instance of device enum";
	const char* ERROR_2 = "Failed to enum active capture endpoints";
	const char* ERROR_3 = "Failed to get device count";
	const char* ERROR_4 = "Failed to get device from id";
	const char* ERROR_5 = "Failed to activate audio client";
	const char* ERROR_6 = "Failed to get device period";
	const char* ERROR_7 = "Failed to get buffer size";
	const char* ERROR_8 = "Failed to initialize";
	const char* ERROR_9 = "Failed to create buffer event";
	const char* ERROR_10 = "Failed to set event handle";
	const char* ERROR_11 = "Failed to get capture service";
	const char* ERROR_12 = "Failed to set thread characteristics";
	const char* ERROR_13 = "Failed to start capture";
	const char* ERROR_14 = "Failed to get buffer";
	const char* ERROR_15 = "Failed to release buffer";
	const char* ERROR_16 = "Failed to stop audio capture";
	const char* ERROR_17 = "Failed to revert thread char";
	const char* ERROR_18 = "Failed to get render service";
	const char* ERROR_19 = "Failed to start render";
	const char* ERROR_20 = "Failed to stop audio render";
	const char* ERROR_21 = "Failed to start rtwq";
	const char* ERROR_22 = "Failed to lock shared work queue";
	const char* ERROR_23 = "Failed to create audio handler";
	const char* ERROR_24 = "Failed to QI for async callback";
	const char* ERROR_25 = "Failed to create async result";
	const char* ERROR_26 = "Failed to put waiting work item";
	const char* ERROR_27 = "Failed to set audio client props";
	const char* ERROR_28 = "Failed to get native format";
	const char* ERROR_29 = "Failed to get shared mode engine period";

	const char* last_error = NULL;

	const char* rta_get_last_error()
	{
		return last_error;
	}

	//static HANDLE HeapHandle = INVALID_HANDLE_VALUE;

	//LPVOID rta_alloc(SIZE_T size) {
	//	LPVOID result = NULL;
	//	if (HeapHandle == INVALID_HANDLE_VALUE) {
	//		HeapHandle = GetProcessHeap();
	//	}
	//	if (HeapHandle != INVALID_HANDLE_VALUE) {
	//		result = malloc(size);
	//		if (result) {
	//			ZeroMemory(result, size);
	//		}
	//	}
	//	return result;
	//}

	//void rta_free(LPVOID pvoid) {
	//	LPVOID result = NULL;
	//	if (HeapHandle == INVALID_HANDLE_VALUE) {
	//		HeapHandle = GetProcessHeap();
	//	}
	//	if (HeapHandle != INVALID_HANDLE_VALUE) {
	//		free(pvoid);
	//	}
	//}

	UINT32 rta_list_supporting_devices_2(RTA_DEVICE_INFO** lppDeviceInfo,
		WAVEFORMATEX *RequestedFormat,
		DWORD StateMask, EDataFlow DataFlow, AUDCLNT_SHAREMODE ShareMode,
		BOOL TestForChannels)
	{

		IMMDeviceEnumerator *pMMDeviceEnumerator = NULL;
		IMMDeviceCollection *pMMDeviceCollection = NULL;
		UINT32 count = 0;
		UINT pcDevices = 0;

		if (lppDeviceInfo == NULL) return count;
		*lppDeviceInfo = NULL;
		LPRTA_DEVICE_INFO current = NULL;

		HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), (LPUNKNOWN)NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)&pMMDeviceEnumerator);
		CHKERR(result, ERROR_1);

		result = pMMDeviceEnumerator->EnumAudioEndpoints(DataFlow, StateMask, &pMMDeviceCollection);
		CHKERR(result, ERROR_2);

		result = pMMDeviceCollection->GetCount(&pcDevices);
		CHKERR(result, ERROR_3);

		for (UINT pcDeviceId = 0; pcDeviceId < pcDevices; pcDeviceId++) {

			// get the current device by id
			IMMDevice *pMMDevice = NULL;
			result = pMMDeviceCollection->Item(pcDeviceId, &pMMDevice);

			if (SUCCEEDED(result)) {

				// activate it
				IAudioClient3 *pAudioClient3 = NULL;
				result = pMMDevice->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, (PROPVARIANT*)0, (void**)&pAudioClient3);

				if (SUCCEEDED(result)) {

					BOOL GotChannels = FALSE;

					if (TRUE == TestForChannels) {
						WORD MaxChannels = 0;
						for (WORD NumChannels = 1; NumChannels < 17; NumChannels++) {
							RequestedFormat->nChannels = NumChannels;
							if (SUCCEEDED(pAudioClient3->IsFormatSupported(ShareMode, RequestedFormat, nullptr))) {
								GotChannels = TRUE;
								MaxChannels = NumChannels;
							}
						}
						RequestedFormat->nChannels = MaxChannels;
					}
					else {
						if (SUCCEEDED(pAudioClient3->IsFormatSupported(ShareMode, RequestedFormat, nullptr)))
							GotChannels = TRUE;
					}

					if (TRUE == GotChannels)
					{

						// get device id; do not free when done
						LPWSTR lpwstrDeviceId = NULL;
						result = pMMDevice->GetId(&lpwstrDeviceId);

						if (SUCCEEDED(result)) {

							// open prop store
							IPropertyStore *pPropertyStore = NULL;
							result = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);

							if (SUCCEEDED(result)) {

								// get name
								PROPVARIANT varName;
								PropVariantInit(&varName);
								result = pPropertyStore->GetValue(
									PKEY_Device_FriendlyName, &varName);

								if (SUCCEEDED(result)) {

									PROPVARIANT varIsRaw;
									PropVariantInit(&varIsRaw);
									result = pPropertyStore->GetValue(
										PKEY_Devices_AudioDevice_RawProcessingSupported, &varIsRaw);

									if (SUCCEEDED(result)) {

										// create data structure
										LPRTA_DEVICE_INFO lpdi = (LPRTA_DEVICE_INFO)malloc(sizeof(RTA_DEVICE_INFO));
										ZeroMemory(lpdi, sizeof(RTA_DEVICE_INFO));
										lpdi->RtaDevInfoId = pcDeviceId;
										lpdi->DeviceId = lpwstrDeviceId;
										lpdi->DeviceName = _wcsdup(varName.pwszVal);
										memcpy(&lpdi->WaveFormat, RequestedFormat, sizeof(WAVEFORMATEX));
										//lpdi->ShareMode = ShareMode;
										//lpdi->lpWaveFormatEx = lpWaveFormatEx;
										lpdi->IsRawSupported = varIsRaw.boolVal;

										//printf("==========\n");
										//wprintf(L"%s\n", varName.pwszVal);
										//printf("channels %i\n", defFmt->nChannels);
										//printf("samples per sec %i\n", defFmt->nSamplesPerSec);
										//printf("bits per samples %i\n", defFmt->wBitsPerSample);
										//printf("RAW is %ssupported.\n",
										//	(varIsRaw.boolVal == VARIANT_FALSE ? "NOT " : ""));
										//printf("==========\n");

										//lpdi->dwSamplesPerSec = nativeFormat->nSamplesPerSec;
										//lpdi->wBitsPerSample = nativeFormat->wBitsPerSample;
										//lpdi->wChannels = nativeFormat->nChannels;

										if (*lppDeviceInfo == NULL) *lppDeviceInfo = lpdi;
										if (current != NULL) current->pNext = lpdi;
										current = lpdi;
										count++;

										PropVariantClear(&varIsRaw);

									}

									PropVariantClear(&varName);

								}

								pPropertyStore->Release();
								pPropertyStore = NULL;
							}
						}
					}
					pAudioClient3->Release();
					pAudioClient3 = NULL;
				}
				pMMDevice->Release();
				pMMDevice = NULL;
			}
		}

		if (pMMDeviceEnumerator != NULL) pMMDeviceEnumerator->Release();
		if (pMMDeviceCollection != NULL) pMMDeviceCollection->Release();
		return count;
	}

	void rta_free_device_list(LPRTA_DEVICE_INFO lpDeviceInfo)
	{
		if (lpDeviceInfo == NULL) return;
		LPRTA_DEVICE_INFO pThis = lpDeviceInfo;
		LPRTA_DEVICE_INFO pNext = NULL;
		while (pThis != NULL) {
			pNext = (LPRTA_DEVICE_INFO)(pThis->pNext);
			if (pThis->DeviceName != NULL) free(pThis->DeviceName);
			if (pThis->DeviceId != NULL) CoTaskMemFree(pThis->DeviceId);
			if (pThis->FrameBufferByte != NULL) free(pThis->FrameBufferByte);
			if (pThis->pAudioClient != NULL) pThis->pAudioClient->Release();
			if (pThis->pMMDevice != NULL) pThis->pMMDevice->Release();
			free(pThis);
			pThis = pNext;
		}
	}

	BOOL rta_initialize_device_2(LPRTA_DEVICE_INFO lpDeviceInfo, DWORD StreamFlags)
	{

		last_error = NULL;
		UINT32 PeriodToUse = 0;

		if (lpDeviceInfo == NULL) return FALSE;

#ifdef _DEBUG
		printf("Init'ing device...\n");
#endif

		BOOL retval = FALSE;

		IMMDeviceEnumerator *pMMDeviceEnumerator = NULL;
		REFERENCE_TIME requested = 0;

		if (lpDeviceInfo == NULL) return FALSE;

		HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), (LPUNKNOWN)NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)&pMMDeviceEnumerator);
		CHKERR(result, ERROR_1);

		// get device
		result = pMMDeviceEnumerator->GetDevice(lpDeviceInfo->DeviceId, &(lpDeviceInfo->pMMDevice));
		CHKERR(result, ERROR_4);

		result = lpDeviceInfo->pMMDevice->Activate(__uuidof(IAudioClient),
			CLSCTX_ALL, 0, (void**)&(lpDeviceInfo->pAudioClient));
		CHKERR(result, ERROR_5);

		// get reference to audio client 3
		IAudioClient3 *pAudioClient3 = NULL;
		lpDeviceInfo->pAudioClient->QueryInterface(__uuidof(IAudioClient3), (void**)&pAudioClient3);
		CHKERR(result, ERROR_5);

		if (VARIANT_TRUE == lpDeviceInfo->IsRawSupported) {
			AudioClientProperties audioProps = { 0 };
			audioProps.cbSize = sizeof(AudioClientProperties);
			audioProps.eCategory = AudioCategory_Media;
			audioProps.Options |= AUDCLNT_STREAMOPTIONS_RAW;
			audioProps.Options |= AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
			result = pAudioClient3->SetClientProperties(&audioProps);
			CHKERR(result, ERROR_27);
#ifdef _DEBUG
			printf("NOTE: Successfully set raw properties.\n");
#endif
		}
#ifdef _DEBUG
		else {
			printf("NOTE: RAW is not supported for this device.\n");
		}
#endif

		WAVEFORMATEX *FormatToUse = &lpDeviceInfo->WaveFormat;
		//if (RequestedFormat != nullptr) {
			//FormatToUse = RequestedFormat;
		//}
		//else {
			//result = pAudioClient3->GetMixFormat(&FormatToUse);
			//CHKERR(result, ERROR_28);
		//}

#ifdef _DEBUG
		printf("Native Samples Per Sec %i\n", FormatToUse->nSamplesPerSec);
		printf("Native Sample Size %i\n", FormatToUse->wBitsPerSample);
		printf("Native Channels %i\n", FormatToUse->nChannels);
		printf("Wave Format Tag %x\n", FormatToUse->wFormatTag);
		printf("Extra Data Size %i\n", FormatToUse->cbSize);
#endif
		if (FormatToUse->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {

			WAVEFORMATEXTENSIBLE* lpwfex = (WAVEFORMATEXTENSIBLE*)FormatToUse;

#ifdef _DEBUG
			printf("EXT Channel Mask %i; Channel Configuration:\n", lpwfex->dwChannelMask);
			for (DWORD chid = 1; chid < 0x40000; chid *= 2) {
				if (lpwfex->dwChannelMask & chid) {
					switch (chid) {
					case 0x01: printf("\tSpeaker Front Left\n"); break;
					case 0x02: printf("\tSpeaker Front Right\n"); break;
					default: printf("\tOther Channel: %x\n", chid); break;
					}
				}
			}

			printf("EXT Valid Bits Per Sample %i\n", lpwfex->Samples.wValidBitsPerSample);
#endif
			if (IsEqualGUID(lpwfex->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
#ifdef _DEBUG
				printf("EXT is ieee float\n");
#endif
			}
			else {
#ifdef _DEBUG
				printf("ERROR: EXT Other format. This program is written to only support IEEE Float data.\n");
#endif
				goto done;
			}
		}
		//else {
			//printf("ERROR: EXT Other format. This program is written to only support IEEE Float data.\n");
			//goto done;
		//}

		UINT32 DefPeriodInFrames, p2, MinPeriodInFrames, p4;
		result = pAudioClient3->GetSharedModeEnginePeriod(
			FormatToUse, &DefPeriodInFrames, &p2, &MinPeriodInFrames, &p4);
		CHKERR(result, ERROR_29);
#ifdef _DEBUG
		printf("Def Period in Frames %i\n", DefPeriodInFrames);
		printf("Fnd Period in Frames %i\n", p2);
		printf("Min Period in Frames %i\n", MinPeriodInFrames);
		printf("Max Period in Frames %i\n", p4);
#endif

		PeriodToUse = DefPeriodInFrames;

		result = pAudioClient3->InitializeSharedAudioStream(
			StreamFlags,
			PeriodToUse,
			FormatToUse,
			nullptr);

		if (FAILED(result))
		{
			pAudioClient3->Release();
			lpDeviceInfo->BufferSizeFrames = 0;
			last_error = ERROR_8;
			goto done;
		}
		else
		{
			pAudioClient3->Release();
#ifdef _DEBUG
			printf("NOTE: Init is successful\n");
#endif

			lpDeviceInfo->BufferSizeFrames = PeriodToUse;
			lpDeviceInfo->SizeOfFrame = FormatToUse->nChannels *
				(FormatToUse->wBitsPerSample / 8);

			// size of frame buffer
			lpDeviceInfo->FrameBufferByte = (BYTE*)malloc(lpDeviceInfo->BufferSizeFrames * lpDeviceInfo->SizeOfFrame);
			ZeroMemory(lpDeviceInfo->FrameBufferByte, lpDeviceInfo->BufferSizeFrames * lpDeviceInfo->SizeOfFrame);

			UINT32 RealBufferSizeFrames = 0;
			lpDeviceInfo->pAudioClient->GetBufferSize(&RealBufferSizeFrames);
#ifdef _DEBUG
			printf("INIT: Success; buffer size is %i [ %i ]\n",
				lpDeviceInfo->BufferSizeFrames, RealBufferSizeFrames);
			if (RealBufferSizeFrames != lpDeviceInfo->BufferSizeFrames) {
				printf("WARNING! Buffer sizes do not match!\n");
			}
#endif
			lpDeviceInfo->RealBufferSizeFrames = RealBufferSizeFrames;

#ifdef _DEBUG
			REFERENCE_TIME latency = 0;
			lpDeviceInfo->pAudioClient->GetStreamLatency(&latency);
			printf("Latency = %lli\n", latency);
#endif
		}

		// done
		retval = TRUE;

	done:
		if (pMMDeviceEnumerator != NULL) pMMDeviceEnumerator->Release();
#ifdef _DEBUG
		if (last_error != NULL)
		{
			printf("LAST ERROR: %s\n", last_error);
		}
#endif
		return retval;
	}

	void rta_render_frames_rtwq(LPRTA_DEVICE_INFO lpRenderDeviceInfo, MusicStudioCommon::RTA_DATA_HANDLER pHandler)
	{

		HRESULT hr = 0;

		if (lpRenderDeviceInfo == NULL) return;

		last_error = NULL;

		IAudioRenderClient *pAudioRenderClient = NULL;

		RtaRenderAudioHandler *pRenderAudioHandler;
		DWORD RtwqTaskId = 0;
		IRtwqAsyncCallback *pAsyncCallback = NULL;

		// instantiate a new audio handler
		// creates a buffer event
		pRenderAudioHandler = new RtaRenderAudioHandler();
		if (pRenderAudioHandler == NULL) {
			last_error = ERROR_23;
			goto done;
		}

		// set the capture audio client event handle
		// to the event handle generated in the rta audio handler
		hr = lpRenderDeviceInfo->pAudioClient->SetEventHandle(
			pRenderAudioHandler->GetBufferEventHandle());
		CHKERR(hr, ERROR_10);

		// get an audio capture client interface from the capture device
		hr = lpRenderDeviceInfo->pAudioClient->GetService(__uuidof(IAudioRenderClient),
			(void**)&pAudioRenderClient);
		CHKERR(hr, ERROR_11);

		// send the client interfaces, device info and handler routine pointer
		// to the audio handler
		pRenderAudioHandler->ConfigureClientInformation(
			pAudioRenderClient, lpRenderDeviceInfo, pHandler);

		// START rtwq

		// start the real-time work queue
		hr = RtwqStartup();
		CHKERR(hr, ERROR_21);

		// obtains and locks a shared work queue
		hr = RtwqLockSharedWorkQueue(L"Audio", 0, &RtwqTaskId, &g_RtwqId);
		CHKERR(hr, ERROR_22);

		// qi a async callback from the audio handler
		hr = pRenderAudioHandler->QueryInterface(__uuidof(IRtwqAsyncCallback), (void**)&pAsyncCallback);
		CHKERR(hr, ERROR_24);

		// From the docs:
		// Creates an asynchronous result object. Use this function 
		// if you are implementing an asynchronous method.
		hr = pRenderAudioHandler->CreateAsyncResult();
		CHKERR(hr, ERROR_25);

		// From the docs:
		// Queues a work item that waits for an event to be signaled.
		hr = pRenderAudioHandler->PutWaitingWorkItem();
		CHKERR(hr, ERROR_26);

		// Create a stop event.
		// Later, the method will block on this event.
		// When the audio handler detemines that it
		// is done, this event will be triggered
		// causing this method to complete.
		g_RtwqStop = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (g_RtwqStop == NULL) {
			last_error = ERROR_9;
			goto done;
		}

		// END rtwq

		// start capture
		hr = lpRenderDeviceInfo->pAudioClient->Start();
		CHKERR(hr, ERROR_13);

		// block and wait for handler to complete
#ifdef _DEBUG
		printf("Waiting for stop signal...\n");
#endif
		WaitForSingleObject(g_RtwqStop, INFINITE);
#ifdef _DEBUG
		printf("Got stop signal...\n");
#endif

		// stop capture and render
		lpRenderDeviceInfo->pAudioClient->Stop();

	done:
#ifdef _DEBUG
		if (last_error != NULL) printf("ERROR: %s\n", last_error);
#endif
		if (0 != g_RtwqId) {
			RtwqUnlockWorkQueue(g_RtwqId);
			RtwqShutdown();
		}
		if (pAsyncCallback != NULL) pAsyncCallback->Release();
		if (g_RtwqStop != NULL) CloseHandle(g_RtwqStop);
		if (pAudioRenderClient != NULL) pAudioRenderClient->Release();
		if (pRenderAudioHandler) pRenderAudioHandler->Release();
		if (lpRenderDeviceInfo->pAudioClient) lpRenderDeviceInfo->pAudioClient->Release();
		lpRenderDeviceInfo->pAudioClient = nullptr;

	}

	void rta_capture_frames_rtwq(LPRTA_DEVICE_INFO lpCaptureDeviceInfo,
		LPRTA_DEVICE_INFO lpRenderDeviceInfo, MusicStudioCommon::RTA_DATA_HANDLER pHandler)
	{

		HRESULT hr = 0;

		if (lpCaptureDeviceInfo == NULL) return;

		last_error = NULL;

		IAudioCaptureClient *pAudioCaptureClient = NULL;
		IAudioRenderClient *pAudioRenderClient = NULL;

		RtaAudioHandler *pAudioHandler;
		DWORD RtwqTaskId = 0;
		IRtwqAsyncCallback *pAsyncCallback = NULL;

		// instantiate a new audio handler
		// creates a buffer event
		pAudioHandler = new RtaAudioHandler();
		if (pAudioHandler == NULL) {
			last_error = ERROR_23;
			goto done;
		}

		// set the capture audio client event handle
		// to the event handle generated in the rta audio handler
		hr = lpCaptureDeviceInfo->pAudioClient->SetEventHandle(
			pAudioHandler->GetBufferEventHandle());
		CHKERR(hr, ERROR_10);

		// get an audio capture client interface from the capture device
		hr = lpCaptureDeviceInfo->pAudioClient->GetService(__uuidof(IAudioCaptureClient),
			(void**)&pAudioCaptureClient);
		CHKERR(hr, ERROR_11);

		// get an audio render client interface from the render device
		if (lpRenderDeviceInfo != NULL) {
			hr = lpRenderDeviceInfo->pAudioClient->GetService(__uuidof(IAudioRenderClient),
				(void**)&pAudioRenderClient);
			CHKERR(hr, ERROR_18);
		}

		// send the client interfaces, device info and handler routine pointer
		// to the audio handler
		pAudioHandler->ConfigureClientInformation(
			pAudioCaptureClient, lpCaptureDeviceInfo,
			pAudioRenderClient, lpRenderDeviceInfo, pHandler);

		// START rtwq

		// start the real-time work queue
		hr = RtwqStartup();
		CHKERR(hr, ERROR_21);

		// obtains and locks a shared work queue
		hr = RtwqLockSharedWorkQueue(L"Audio", 0, &RtwqTaskId, &g_RtwqId);
		CHKERR(hr, ERROR_22);

		// qi a async callback from the audio handler
		hr = pAudioHandler->QueryInterface(__uuidof(IRtwqAsyncCallback), (void**)&pAsyncCallback);
		CHKERR(hr, ERROR_24);

		// From the docs:
		// Creates an asynchronous result object. Use this function 
		// if you are implementing an asynchronous method.
		hr = pAudioHandler->CreateAsyncResult();
		CHKERR(hr, ERROR_25);

		// From the docs:
		// Queues a work item that waits for an event to be signaled.
		hr = pAudioHandler->PutWaitingWorkItem();
		CHKERR(hr, ERROR_26);

		// Create a stop event.
		// Later, the method will block on this event.
		// When the audio handler detemines that it
		// is done, this event will be triggered
		// causing this method to complete.
		g_RtwqStop = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (g_RtwqStop == NULL) {
			last_error = ERROR_9;
			goto done;
		}

		// END rtwq

		// start capture
		hr = lpCaptureDeviceInfo->pAudioClient->Start();
		CHKERR(hr, ERROR_13);

		// start render
		if (lpRenderDeviceInfo != NULL) {
			hr = lpRenderDeviceInfo->pAudioClient->Start();
			if (FAILED(hr)) {
				last_error = ERROR_19;
				lpCaptureDeviceInfo->pAudioClient->Stop();
				goto done;
			}
		}

		// block and wait for handler to complete
#ifdef _DEBUG
		printf("Waiting for stop signal...\n");
#endif
		WaitForSingleObject(g_RtwqStop, INFINITE);
#ifdef _DEBUG
		printf("Got stop signal...\n");
#endif


		// stop capture and render
		hr = lpCaptureDeviceInfo->pAudioClient->Stop();
		if (lpRenderDeviceInfo != NULL) {
			lpRenderDeviceInfo->pAudioClient->Stop();
		}

	done:
#ifdef _DEBUG
		if (last_error != NULL) printf("ERROR: %s\n", last_error);
#endif
		if (0 != g_RtwqId) {
			RtwqUnlockWorkQueue(g_RtwqId);
			RtwqShutdown();
		}
		if (pAsyncCallback != NULL) pAsyncCallback->Release();
		if (g_RtwqStop != NULL) CloseHandle(g_RtwqStop);
		if (pAudioCaptureClient != NULL) pAudioCaptureClient->Release();
		if (pAudioRenderClient != NULL) pAudioRenderClient->Release();
		if (pAudioHandler) pAudioHandler->Release();
		if (lpRenderDeviceInfo->pAudioClient) lpRenderDeviceInfo->pAudioClient->Release();
		lpRenderDeviceInfo->pAudioClient = nullptr;
		if (lpCaptureDeviceInfo->pAudioClient) lpCaptureDeviceInfo->pAudioClient->Release();
		lpCaptureDeviceInfo->pAudioClient = nullptr;

	}

}