
#include "stdafx.h"

namespace CXASIO {

	void asio_free_device_list(ASIO_DEVICE_INFO* lpDevInfo)
	{
		if (lpDevInfo == nullptr) return;
		ASIO_DEVICE_INFO* pThis = lpDevInfo;
		ASIO_DEVICE_INFO* pNext = nullptr;
		while (pThis != nullptr) {
			pNext = (ASIO_DEVICE_INFO*)pThis->lpvNext;
			free(pThis);
			pThis = pNext;
		}
	}

	UINT32 asio_list_supporting_devices(ASIO_DEVICE_INFO** lppDevInfo)
	{

		WCHAR l_wcDriverName[ASIO_CHAR_BUFFER_LENGTH];
		WCHAR l_wcClsid[ASIO_CHAR_BUFFER_LENGTH];
		HKEY hkeyASIO = NULL;
		DWORD AsioDriverNameLength = ASIO_CHAR_BUFFER_LENGTH;
		int subkeyIndex = 0;
		ASIO_DEVICE_INFO *pLast = nullptr;
		UINT32 NumberOfDevices = 0;

#ifdef _DEBUG
		printf("\nEnumAsio\n");
		printf("(c) 2018 Stuart Harlan\n");
		printf("\n");
#endif

		long regerr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", 0, KEY_READ, &hkeyASIO);
		if (ERROR_SUCCESS != regerr)
		{
#ifdef _DEBUG
			printf("ERR: Failed to open asio key\n");
#endif
			goto done;
		}

		ZeroMemory(l_wcDriverName, ASIO_CHAR_BUFFER_LENGTH * sizeof(WCHAR));
		regerr = RegEnumKeyEx(hkeyASIO, subkeyIndex, l_wcDriverName, &AsioDriverNameLength, NULL, NULL, NULL, NULL);
		if (regerr != ERROR_SUCCESS && regerr != ERROR_NO_MORE_ITEMS)
		{
#ifdef _DEBUG
			printf("ERR: Failed to enum reg keys\n");
#endif
			goto done;
		}

		while (ERROR_NO_MORE_ITEMS != regerr)
		{
			ZeroMemory(l_wcClsid, ASIO_CHAR_BUFFER_LENGTH * sizeof(WCHAR));
			DWORD dwDriverNameSize = ASIO_CHAR_BUFFER_LENGTH;
			regerr = RegGetValue(hkeyASIO, l_wcDriverName, L"CLSID", RRF_RT_REG_SZ, NULL, l_wcClsid, &dwDriverNameSize);
			if (ERROR_SUCCESS == regerr)
			{
#ifdef _DEBUG
				wprintf(L"%s = %s\n", l_wcDriverName, l_wcClsid);
#endif

				ASIO_DEVICE_INFO* devInfo = (ASIO_DEVICE_INFO*)malloc(sizeof(ASIO_DEVICE_INFO));
				if (devInfo) {
					ZeroMemory(devInfo, sizeof(ASIO_DEVICE_INFO));
					wcscpy_s(devInfo->wcDriverName, ASIO_CHAR_BUFFER_LENGTH, l_wcDriverName);
					wcscpy_s(devInfo->wcClsid, ASIO_CHAR_BUFFER_LENGTH, l_wcClsid);
					devInfo->AsioDevInfoId = NumberOfDevices;
					if (*lppDevInfo == nullptr) {
						*lppDevInfo = devInfo;
						pLast = devInfo;
					}
					else {
						pLast->lpvNext = devInfo;
						pLast = devInfo;
					}
					NumberOfDevices++;
				}

			}
			else {
#ifdef _DEBUG
				printf("ERR: %i - can't get driver clsid\n", regerr);
#endif
				goto done;
			}

			AsioDriverNameLength = ASIO_CHAR_BUFFER_LENGTH;
			ZeroMemory(l_wcDriverName, ASIO_CHAR_BUFFER_LENGTH * sizeof(wchar_t));
			subkeyIndex++;
			regerr = RegEnumKeyEx(hkeyASIO, subkeyIndex, l_wcDriverName, &AsioDriverNameLength, NULL, NULL, NULL, NULL);
			if (regerr != ERROR_SUCCESS && regerr != ERROR_NO_MORE_ITEMS)
			{
#ifdef _DEBUG
				printf("ERR: Failed to enum reg keys\n");
#endif
				goto done;
			}
		}

	done:

		if (hkeyASIO != NULL) RegCloseKey(hkeyASIO);
		hkeyASIO = NULL;

		return NumberOfDevices;
	}


	/* from asio test */

	// AudioboxAsioTest.cpp : Defines the entry point for the console application.
	//

	HANDLE hQuit = INVALID_HANDLE_VALUE;

	UINT32 GetSampleByteSize(ASIOSampleType t) {
		switch (t) {
		case ASIOSTInt16LSB:
			return 2;
		case ASIOSTInt32LSB:
			return 4;
		}
		return 0;
	}

	long minSize; // samples
	long maxSize; // samples
	long PrefferedBufferSizeSamples; // samples
	long granularity;
	ASIOChannelInfo* InputChannelInfo = NULL;
	ASIOChannelInfo* OutputChannelInfo = NULL;
	ASIOBufferInfo* BufferInfo = NULL;
	long numInputChannels;
	long numOutputChannels;
	LARGE_INTEGER pfreq;
	LARGE_INTEGER pLast = { 0 };
	RTA_DATA_HANDLER g_pHandler = nullptr;
	HANDLER_CONTEXT hctx;

	void OnBufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
	{

		LARGE_INTEGER ctr;
		QueryPerformanceCounter(&ctr);
		LONGLONG totalelps = ctr.QuadPart - pLast.QuadPart;
#ifdef _DEBUG
		printf("elapsed %8lli counts; %4lli ms; ", totalelps,
			(ctr.QuadPart - pLast.QuadPart) * 1000 / pfreq.QuadPart);
#endif
		pLast = ctr;

		ASIOBufferInfo* pInputBuffer = BufferInfo;
		ASIOBufferInfo* pOutputBuffer = BufferInfo + numInputChannels;

		for (int i = 0; i < numInputChannels; i++) {

			// this is for no processing - straight through
			//memcpy(
			//pOutputBuffers[i].buffers[doubleBufferIndex],
			//pInputBuffer[i].buffers[doubleBufferIndex],
			//GetSampleByteSize(InputChannelInfo[i].type) * PrefferedBufferSizeSamples);

			if (InputChannelInfo[i].type == 18) {
				// 24 bit audio signed
				INT32* i32input = (INT32*)pInputBuffer[i].buffers[doubleBufferIndex];
				INT32* i32output = (INT32*)pOutputBuffer[i].buffers[doubleBufferIndex];
				//INT32 isample = 0;
				float fsample = 0.0f;
				float fmaxSample = 8388607.0f;
				float fminSample = -8388607.0f;
				//float fthreshold = fmaxSample * 0.001f;
				//float f80pmaxSample = fmaxSample * 0.8f;
				//INT32 peakup = 0;
				//INT32 peakdn = 0;
				//for (long s = 0; s < PrefferedBufferSizeSamples; s++) {
				//isample = *(i32input + s) >> 8;
				//if (isample > peakup) peakup = isample;
				//if (isample < peakdn) peakdn = isample;
				//}
				//peakdn *= -1;
				//INT32 peak = (peakup > peakdn ? peakup : peakdn);
				//float ampFrom = ChannelContext[i].amplifier;
				//float ampTo = ChannelContext[i].amplifier;
				//if (peak > 0) {
				//if (peak < fthreshold) {
				//ampTo = 0.0f;
				//}
				//else {
				//ampTo = f80pmaxSample / (float)peak;
				//}
				//}
				//float ampStep = (ampTo - ampFrom) / (float)PrefferedBufferSizeSamples;
				int clipping = 0;
				for (long s = 0; s < PrefferedBufferSizeSamples; s++) {
					fsample = (float)(*(i32input + s) >> 8);
					//fsample = ampFrom * fsample;
					//fsample /= peak;
					//fsample = fsample - ((fsample * fsample * fsample) / 3.0f);
					//fsample *= peak;
					//ampFrom += ampStep;
					if (fsample > fmaxSample) {
						clipping = 1;
						fsample = fmaxSample;
					}
					if (fsample < fminSample) {
						clipping = 1;
						fsample = fminSample;
					}
					*(i32output + s) = ((INT32)fsample) << 8;
				}
				//ChannelContext[i].amplifier = ampTo;
				//printf(" %10i; %.1f; CLP %i; ", peak, ChannelContext[i].amplifier, clipping);
#ifdef _DEBUG
				printf("clipping %i; ", clipping);
#endif
			}
			else if (InputChannelInfo[i].type == 16) {
				// 16 bit audio signed short
				INT16* i16input = (INT16*)pInputBuffer[i].buffers[doubleBufferIndex];
				INT16* i16output = (INT16*)pOutputBuffer[i].buffers[doubleBufferIndex];
				INT16 sample = 0;
				//INT16 maxSample = (INT16)32767;
				//INT16 minSample = (INT16)-32767;
				//INT16 Amplifier = (INT16)ChannelContext[i].amplifier;
				//INT16 maxSample1 = maxSample / AmplifierMultiplier;
				//INT16 minSample1 = minSample / AmplifierMultiplier;
				//ChannelContext[i].peak = 0;
				for (long s = 0; s < PrefferedBufferSizeSamples; s++) {
					sample = *(i16input);
					//if (sample > maxSample1) sample = maxSample1;
					//if (sample < minSample1) sample = minSample1;
					//if (sample >= 0) sample += Amplifier;
					//else sample -= Amplifier;
					//if (sample > ChannelContext[i].peak) ChannelContext[i].peak = sample;
					//if (sample < 0 && ((sample * -1) > ChannelContext[i].peak)) ChannelContext[i].peak = sample * -1;
					*(i16output) = sample;
				}
				//ChannelContext[i].amplifier = ((maxSample / 2) - ChannelContext[i].peak) / 2;
				//printf(" %10i; %2i; ", ChannelContext[i].peak, Amplifier);
			}

		}

		LARGE_INTEGER elps;
		QueryPerformanceCounter(&elps);
		LONGLONG procelps = elps.QuadPart - ctr.QuadPart;
#ifdef _DEBUG
		printf("elps %8lli; over %i\n", procelps, (procelps > totalelps ? 1 : 0));
#endif

		BOOL Cancel = FALSE;
		for (UINT32 ch = 0; ch < numInputChannels; ch++) {
			hctx.capBuffer = (BYTE*)pInputBuffer[ch].buffers[doubleBufferIndex];
			hctx.ChannelIndex = ch;
			hctx.fmt = CX_AUDIO_FORMAT::FMT_24_BIT_SIGNED;
			hctx.frameCount = PrefferedBufferSizeSamples;
			hctx.LastFrameCounts[0] = hctx.LastFrameCounts[1] = hctx.LastFrameCounts[2] = 0;
			hctx.renBuffer = (BYTE*)pOutputBuffer[ch].buffers[doubleBufferIndex];
			g_pHandler(&hctx, &Cancel);
		}
		if (TRUE == Cancel) SetEvent(hQuit);
	}

	void OnSampleRateDidChange(ASIOSampleRate sRate)
	{
#ifdef _DEBUG		
		printf("sample rate changed\n");
#endif
	}

	long OnAsioMessage(long selector, long value, void* message, double* opt)
	{
#ifdef _DEBUG
		printf("message %i; %i; 0x%llx; 0x%llx\n", selector, value, (UINT64)message, (UINT64)opt);
#endif
		return 0;
	}

	ASIOTime* OnBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
	{
#ifdef _DEBUG
		printf("buffer switch time info\n");
#endif
		return NULL;
	}

	void DumpChannelInfo(long numChannels, IASIO *pAsio, ASIOBool isInput, ASIOChannelInfo* ChanInfo)
	{
		for (long l = 0; l < numChannels; l++) {

			ASIOChannelInfo* thisChanInfo = ChanInfo + l;

#ifdef _DEBUG
			if (ASIOTrue == isInput) printf("\nInput Channel #%i\n", l);
			if (ASIOFalse == isInput) printf("\nOutput Channel #%i\n", l);
#endif
			thisChanInfo->channel = l;
			thisChanInfo->isInput = isInput;
			pAsio->getChannelInfo(thisChanInfo);
#ifdef _DEBUG
			printf("\t    Channel Name: %s\n", thisChanInfo->name);
			printf("\t    Channel Type: %i\n", thisChanInfo->type);
			UINT32 bps = GetSampleByteSize(thisChanInfo->type);
			printf("\tBytes per Sample: %i (%i bit)\n", bps, bps * 8);
			if (thisChanInfo->isActive == ASIOTrue) {
				printf("\t      Channel is: active\n");
			}
			else {
				printf("\t      Channel is: NOT active\n");
			}
#endif
		}
	}

	DWORD WINAPI ASIOThread(LPVOID lpParameter)
	{
		IASIO *pAsio = (IASIO*)lpParameter;
		pAsio->AddRef();
		hQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
		pAsio->start();
		WaitForSingleObject(hQuit, INFINITE);
		pAsio->stop();
		CloseHandle(hQuit);
		pAsio->disposeBuffers();
		pAsio->Release();
		return 0;
	}

	int asio_start(ASIO_DEVICE_INFO *pDevice, RTA_DATA_HANDLER pHandler)
	{

		CLSID driverClsid;
		ASIOSampleRate sampleRate = 48000;
		HRESULT hr = 0;
		void* sysHandle = NULL;
		char* DriverName = NULL;
		BYTE* BigBuffer = NULL;
		ASIOClockSource clockSources[10];
		long numSources = 10;
		long inputLatency = 0;
		long outputLatency = 0;
		UINT32 SizeOfBigBuffer = 0;
		UINT32 ChannelCounter = 0;
		BYTE* bufferPointer = nullptr;
		ASIOError err = 0;

		g_pHandler = pHandler;

		hr = CLSIDFromString(pDevice->wcClsid, &driverClsid);
		if (FAILED(hr)) {
#ifdef _DEBUG
			printf("Failed to parse clsid\n");
#endif
			return 0;
		}
		
		QueryPerformanceFrequency(&pfreq);

		CoInitialize(nullptr);

		if (pDevice->pAsio) pDevice->pAsio->Release();
		pDevice->pAsio = nullptr;

		// load the driver, get an instance
		hr = CoCreateInstance(driverClsid, (LPUNKNOWN)NULL,
			CLSCTX_INPROC_SERVER, driverClsid, (void**)&pDevice->pAsio);
		if (FAILED(hr)) {
#ifdef _DEBUG
			printf("ERR: Failed to create driver object\n");
#endif
			goto done;
		}

		if (!pDevice->pAsio->init(sysHandle)) {
#ifdef _DEBUG
			printf("ERR: failed to init ASIO\n");
#endif
			goto done;
		}

		DriverName = (char*)malloc(1024 * sizeof(char));
		if (DriverName == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate memory for driver name\n");
#endif
			goto done;
		}
		ZeroMemory(DriverName, 1024);
		pDevice->pAsio->getDriverName(DriverName);
#ifdef _DEBUG
		printf("                 Driver: %s Version %i\n", DriverName, pDevice->pAsio->getDriverVersion());
#endif

		pDevice->pAsio->getSampleRate(&sampleRate);
#ifdef _DEBUG
		printf("            Sample Rate: %.0f\n", sampleRate);
#endif

		pDevice->pAsio->getBufferSize(&minSize, &maxSize, &PrefferedBufferSizeSamples, &granularity);
#ifdef _DEBUG
		printf("        Buffer Min Size: %i\n", minSize);
		printf("        Buffer Max Size: %i\n", maxSize);
		printf("  Buffer Preferred Size: %i\n", PrefferedBufferSizeSamples);
		printf("Buffer Size Granularity: %i\n", granularity);
#endif

		pDevice->pAsio->getClockSources(&clockSources[0], &numSources);
#ifdef _DEBUG
		printf("Number of clock sources: %i\n", numSources);
		for (int i = 0; i < numSources; i++) {
			printf("         Clock Source %i: %s\n", i, clockSources[i].name);
		}
#endif

		pDevice->pAsio->getLatencies(&inputLatency, &outputLatency);
#ifdef _DEBUG
		printf("          Input Latency: %i\n", inputLatency);
		printf("         Output Latency: %i\n", outputLatency);
#endif

		pDevice->pAsio->getChannels(&numInputChannels, &numOutputChannels);
#ifdef _DEBUG
		printf("     Num Input Channels: %i\n", numInputChannels);
		printf("    Num Output Channels: %i\n", numOutputChannels);
#endif

		InputChannelInfo = (ASIOChannelInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			numInputChannels * sizeof(ASIOChannelInfo));
		OutputChannelInfo = (ASIOChannelInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			numOutputChannels * sizeof(ASIOChannelInfo));
		if (InputChannelInfo == NULL || OutputChannelInfo == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate channel info\n");
#endif
			goto done;
		}

#ifdef _DEBUG
		printf("\nInput Channels\n");
		printf("==============\n");
#endif
		DumpChannelInfo(numInputChannels, pDevice->pAsio, ASIOTrue, InputChannelInfo);

#ifdef _DEBUG
		printf("\nOutput Channels\n");
		printf("==============\n");
#endif
		DumpChannelInfo(numInputChannels, pDevice->pAsio, ASIOFalse, OutputChannelInfo);

#ifdef _DEBUG
		printf("\nCalculating buffer size...");
#endif
		for (int i = 0; i < numInputChannels; i++) {
			SizeOfBigBuffer += GetSampleByteSize(InputChannelInfo[i].type) * PrefferedBufferSizeSamples;
		}

		for (int i = 0; i < numOutputChannels; i++) {
			UINT32 s = GetSampleByteSize(OutputChannelInfo[i].type) * PrefferedBufferSizeSamples;
			SizeOfBigBuffer += s;
		}

		// two buffers per channel so multiply by 2
		SizeOfBigBuffer *= 2;
#ifdef _DEBUG
		printf("%i bytes\n", SizeOfBigBuffer);
#endif

#ifdef _DEBUG
		printf("Allocating buffer info...");
#endif
		BigBuffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SizeOfBigBuffer);
		if (BigBuffer == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate big buffer\n");
#endif
			goto done;
		}

		BufferInfo = (ASIOBufferInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			(numInputChannels + numOutputChannels) * sizeof(ASIOBufferInfo));
		if (BufferInfo == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate buffer info\n");
#endif
			goto done;
		}

		bufferPointer = BigBuffer;
		for (int i = 0; i < numInputChannels; i++) {
			// get channel info to get sample byte size
			UINT32 AdvanceBy = GetSampleByteSize(InputChannelInfo[i].type) * PrefferedBufferSizeSamples;

			// configure channel
			BufferInfo[ChannelCounter].channelNum = i;
			BufferInfo[ChannelCounter].isInput = ASIOTrue;
			BufferInfo[ChannelCounter].buffers[0] = bufferPointer;
			bufferPointer += AdvanceBy;
			BufferInfo[ChannelCounter].buffers[1] = bufferPointer;
			bufferPointer += AdvanceBy;

			ChannelCounter++;
		}
		for (int i = 0; i < numOutputChannels; i++) {
			// get channel info to get sample byte size
			UINT32 AdvanceBy = GetSampleByteSize(OutputChannelInfo[i].type) * PrefferedBufferSizeSamples;

			// configure channel
			BufferInfo[ChannelCounter].channelNum = i;
			BufferInfo[ChannelCounter].isInput = ASIOFalse;
			BufferInfo[ChannelCounter].buffers[0] = bufferPointer;
			bufferPointer += AdvanceBy;
			BufferInfo[ChannelCounter].buffers[1] = bufferPointer;
			bufferPointer += AdvanceBy;

			ChannelCounter++;
		}

#ifdef _DEBUG
		printf("done\n");
#endif

		ASIOCallbacks callbacks;
		callbacks.asioMessage = OnAsioMessage;
		callbacks.bufferSwitch = OnBufferSwitch;
		callbacks.bufferSwitchTimeInfo = OnBufferSwitchTimeInfo;
		callbacks.sampleRateDidChange = OnSampleRateDidChange;

#ifdef _DEBUG
		printf("Creating buffers...");
#endif
		err = pDevice->pAsio->createBuffers(BufferInfo,
			numInputChannels + numOutputChannels, PrefferedBufferSizeSamples, &callbacks);
		if (err == ASE_NoMemory) {
#ifdef _DEBUG
			printf("ERROR\n");
			printf("create buffers fail; no mem\n");
#endif
			goto done;
		}
		else if (err == ASE_NotPresent) {
#ifdef _DEBUG
			printf("ERROR\n");
			printf("create buffers fail; no i/o\n");
#endif
			goto done;
		}
		else if (err == ASE_InvalidMode) {
#ifdef _DEBUG
			printf("ERROR\n");
			printf("create buffers fail; invalid mode\n");
#endif
			goto done;
		}
		else if (err == ASE_OK) {
			HANDLE hHandle;
			hHandle = CreateThread(NULL, 0, ASIOThread, (LPVOID)pDevice->pAsio, 0, NULL);
			WaitForSingleObject(hHandle, INFINITE);
			CloseHandle(hHandle);
		}

	done:

		if (DriverName != NULL) free(DriverName);
		if (BigBuffer != NULL) {
			HeapFree(GetProcessHeap(), 0, BigBuffer);
		}
		if (InputChannelInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, InputChannelInfo);
		}
		if (OutputChannelInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, OutputChannelInfo);
		}
		if (BufferInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, BufferInfo);
		}
		if (pDevice->pAsio != NULL) pDevice->pAsio->Release();
		pDevice->pAsio = nullptr;

		CoUninitialize();

#ifdef _DEBUG
		printf("done\n");
#endif

		return 0;
	}


}