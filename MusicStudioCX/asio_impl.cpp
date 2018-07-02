
#include "stdafx.h"

#define MAX24BIT 8388607
#define MIN24BIT -8388607
#define INT16_TO_INT24(x) (INT32)((x * 128) << 8)
#define INT24_TO_INT16(x) (INT16)((x >> 8) / 128)

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

	HANDLE g_hQuit = INVALID_HANDLE_VALUE;

	UINT32 GetSampleByteSize(ASIOSampleType t) {
		switch (t) {
		case ASIOSTInt16LSB:
			return 2;
		case ASIOSTInt32LSB:
			return 4;
		}
		return 0;
	}

	long g_minSize; // samples
	long g_maxSize; // samples
	long g_PreferredBufferSizeSamples; // samples
	long g_granularity;
	ASIOChannelInfo* g_InputChannelInfo = NULL;
	ASIOChannelInfo* g_OutputChannelInfo = NULL;
	ASIOBufferInfo* g_BufferInfo = NULL;
	long g_numInputChannels;
	long g_numOutputChannels;
	LARGE_INTEGER g_pfreq;
	LARGE_INTEGER g_pLast = { 0 };
	MusicStudioCommon::RTA_DATA_HANDLER g_pHandler = nullptr;
	MusicStudioCommon::HANDLER_CONTEXT g_hctx;
	FRAME2CHSHORT* g_TransferRenderBuffer2Ch;
	short* g_TransferCaptureBufferNCh;

	void OnBufferSwitchCapture(long doubleBufferIndex, ASIOBool directProcess)
	{
		LARGE_INTEGER ctr;
		LONGLONG totalelps = 0;
		BOOL Cancel = FALSE;
		ASIOBufferInfo* pInputBuffer = g_BufferInfo;
		ASIOBufferInfo* pOutputBuffer = g_BufferInfo + g_numInputChannels;
		INT32* i32outputch0 = (INT32*)pOutputBuffer[0].buffers[doubleBufferIndex];
		INT32* i32outputch1 = (INT32*)pOutputBuffer[1].buffers[doubleBufferIndex];
		MusicStudioCommon::HANDLER_CONTEXT hctx;
		short temp = 0;
		short sval = 0;

		QueryPerformanceCounter(&ctr);
		totalelps = ctr.QuadPart - g_pLast.QuadPart;

#ifdef _DEBUG
		printf("elapsed %8lli counts; %4lli ms; frames %i; ", totalelps,
			(ctr.QuadPart - g_pLast.QuadPart) * 1000 / g_pfreq.QuadPart,
			g_PreferredBufferSizeSamples);
#endif
		g_pLast = ctr;

		// translate captured data to transfer buffer
		temp = 0;
		for (long channel = 0; channel < g_numInputChannels; channel++) {
			INT32* i32inputchN = (INT32*)pInputBuffer[channel].buffers[doubleBufferIndex];
			// need to do some bit twiddling here 
			// need to convert from short to 24-bit integer
			//i32outputch0[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[0]);
			//i32outputch1[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[1]);
			for (long frame = 0; frame < g_PreferredBufferSizeSamples; frame++) {
				// samples * channels
				sval = INT24_TO_INT16(i32inputchN[frame]);
				g_TransferCaptureBufferNCh[(frame * g_numInputChannels) + channel] = sval;
				if (channel == 0 && sval > temp) temp = sval;
			}
		}

		ZeroMemory(g_TransferRenderBuffer2Ch, g_PreferredBufferSizeSamples * sizeof(FRAME2CHSHORT));
		hctx.audioDeviceType = MusicStudioCommon::AudioDeviceType::AUDIO_DEVICE_ASIO;
		hctx.CapturedDataBuffer = (BYTE*)g_TransferCaptureBufferNCh;
		hctx.DataToRenderBuffer = (BYTE*)g_TransferRenderBuffer2Ch;
		hctx.frameCount = g_PreferredBufferSizeSamples;
		hctx.LastFrameCounts[0] = hctx.LastFrameCounts[1] = hctx.LastFrameCounts[2] = 0;
		g_pHandler(&hctx, &Cancel);

		// convert short buffer to asio
		for (long frame = 0; frame < g_PreferredBufferSizeSamples; frame++) {
			// need to do some bit twiddling here 
			// need to convert from short to 24-bit integer
			i32outputch0[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[0]);
			i32outputch1[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[1]);
		}

		if (TRUE == Cancel) SetEvent(g_hQuit);

		// print elapsed stats
		LARGE_INTEGER elps;
		QueryPerformanceCounter(&elps);
		LONGLONG procelps = elps.QuadPart - ctr.QuadPart;
#ifdef _DEBUG
		printf("elps %8lli; over %i; maxch1 %i\n", procelps, (procelps > totalelps ? 1 : 0), temp);
#endif

	}

	void OnBufferSwitchRender(long doubleBufferIndex, ASIOBool directProcess)
	{

		LARGE_INTEGER ctr;
		LONGLONG totalelps = 0;
		BOOL Cancel = FALSE;
		//ASIOBufferInfo* pInputBuffer = g_BufferInfo;
		//ASIOBufferInfo* pOutputBuffer = g_BufferInfo + g_numInputChannels;
		ASIOBufferInfo* pOutputBuffer = g_BufferInfo;
		INT32* i32outputch0 = (INT32*)pOutputBuffer[0].buffers[doubleBufferIndex];
		INT32* i32outputch1 = (INT32*)pOutputBuffer[1].buffers[doubleBufferIndex];
		MusicStudioCommon::HANDLER_CONTEXT hctx;

		QueryPerformanceCounter(&ctr);
		totalelps = ctr.QuadPart - g_pLast.QuadPart;

#ifdef _DEBUG
		printf("elapsed %8lli counts; %4lli ms; frames %i; ", totalelps,
			(ctr.QuadPart - g_pLast.QuadPart) * 1000 / g_pfreq.QuadPart,
			g_PreferredBufferSizeSamples);
#endif
		g_pLast = ctr;

		ZeroMemory(g_TransferRenderBuffer2Ch, g_PreferredBufferSizeSamples * sizeof(FRAME2CHSHORT));
		hctx.audioDeviceType = MusicStudioCommon::AudioDeviceType::AUDIO_DEVICE_ASIO;
		hctx.CapturedDataBuffer = nullptr;
		hctx.DataToRenderBuffer = (BYTE*)g_TransferRenderBuffer2Ch;
		hctx.frameCount = g_PreferredBufferSizeSamples;
		hctx.LastFrameCounts[0] = hctx.LastFrameCounts[1] = hctx.LastFrameCounts[2] = 0;
		g_pHandler(&hctx, &Cancel);

		// convert short buffer to asio
		for (long frame = 0; frame < g_PreferredBufferSizeSamples; frame++) {
			// need to do some bit twiddling here 
			// need to convert from short to 24-bit integer
			i32outputch0[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[0]);
			i32outputch1[frame] = INT16_TO_INT24(g_TransferRenderBuffer2Ch[frame].channel[1]);
		}

		if (TRUE == Cancel) SetEvent(g_hQuit);

		// print elapsed stats
		LARGE_INTEGER elps;
		QueryPerformanceCounter(&elps);
		LONGLONG procelps = elps.QuadPart - ctr.QuadPart;
#ifdef _DEBUG
		printf("elps %8lli; over %i\n", procelps, (procelps > totalelps ? 1 : 0));
#endif

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
			if (thisChanInfo->type != ASIOSTInt32LSB)
			{
				throw;
			}
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
		g_hQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
		pAsio->start();
		WaitForSingleObject(g_hQuit, INFINITE);
		pAsio->stop();
		CloseHandle(g_hQuit);
		pAsio->disposeBuffers();
		pAsio->Release();
		return 0;
	}

	int asio_start(ASIO_DEVICE_INFO *pDevice, MusicStudioCommon::RTA_DATA_HANDLER pHandler, BOOL IsRecording)
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
		UINT32 TotalChannels = 0;

		g_pHandler = pHandler;

		hr = CLSIDFromString(pDevice->wcClsid, &driverClsid);
		if (FAILED(hr)) {
#ifdef _DEBUG
			printf("Failed to parse clsid\n");
#endif
			return 0;
		}
		
		QueryPerformanceFrequency(&g_pfreq);

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

		pDevice->pAsio->setSampleRate(sampleRate);
		pDevice->pAsio->getSampleRate(&sampleRate);
#ifdef _DEBUG
		printf("            Sample Rate: %.0f\n", sampleRate);
#endif

		pDevice->pAsio->getBufferSize(&g_minSize, &g_maxSize, &g_PreferredBufferSizeSamples, &g_granularity);
#ifdef _DEBUG
		printf("        Buffer Min Size: %i\n", g_minSize);
		printf("        Buffer Max Size: %i\n", g_maxSize);
		printf("  Buffer Preferred Size: %i\n", g_PreferredBufferSizeSamples);
		printf("Buffer Size Granularity: %i\n", g_granularity);
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

		pDevice->pAsio->getChannels(&g_numInputChannels, &g_numOutputChannels);
#ifdef _DEBUG
		printf("     Num Input Channels: %i\n", g_numInputChannels);
		printf("    Num Output Channels: %i\n", g_numOutputChannels);
#endif

		g_InputChannelInfo = (ASIOChannelInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			g_numInputChannels * sizeof(ASIOChannelInfo));
		g_OutputChannelInfo = (ASIOChannelInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			g_numOutputChannels * sizeof(ASIOChannelInfo));
		if (g_InputChannelInfo == NULL || g_OutputChannelInfo == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate channel info\n");
#endif
			goto done;
		}

#ifdef _DEBUG
		printf("\nInput Channels\n");
		printf("==============\n");
#endif
		DumpChannelInfo(g_numInputChannels, pDevice->pAsio, ASIOTrue, g_InputChannelInfo);

#ifdef _DEBUG
		printf("\nOutput Channels\n");
		printf("==============\n");
#endif
		DumpChannelInfo(g_numInputChannels, pDevice->pAsio, ASIOFalse, g_OutputChannelInfo);

#ifdef _DEBUG
		printf("\nCalculating buffer size...");
#endif

		if (TRUE == IsRecording)
		{
			for (int i = 0; i < g_numInputChannels; i++) {
				SizeOfBigBuffer += GetSampleByteSize(g_InputChannelInfo[i].type) * g_PreferredBufferSizeSamples;
			}
		}

		for (int i = 0; i < g_numOutputChannels; i++) {
			UINT32 s = GetSampleByteSize(g_OutputChannelInfo[i].type) * g_PreferredBufferSizeSamples;
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

		// transfer buffer
		// this is a 2 channel short buffer
		TotalChannels = 2;
		g_TransferRenderBuffer2Ch = (FRAME2CHSHORT*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			g_PreferredBufferSizeSamples * sizeof(FRAME2CHSHORT));
		ZeroMemory(g_TransferRenderBuffer2Ch, g_PreferredBufferSizeSamples * sizeof(FRAME2CHSHORT));

		if (TRUE == IsRecording) {
			// this is a N channel short buffer
			TotalChannels += g_numInputChannels;
			g_TransferCaptureBufferNCh = (short*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				g_PreferredBufferSizeSamples * sizeof(short) * g_numInputChannels);
			ZeroMemory(g_TransferCaptureBufferNCh, g_PreferredBufferSizeSamples * sizeof(short) * g_numInputChannels);
		}

		g_BufferInfo = (ASIOBufferInfo*)HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, TotalChannels * sizeof(ASIOBufferInfo));
		if (g_BufferInfo == NULL) {
#ifdef _DEBUG
			printf("ERR: Failed to allocate buffer info\n");
#endif
			goto done;
		}

		bufferPointer = BigBuffer;

		if (TRUE == IsRecording) {
			for (int i = 0; i < g_numInputChannels; i++) {
				// get channel info to get sample byte size
				UINT32 AdvanceBy = GetSampleByteSize(g_InputChannelInfo[i].type) * g_PreferredBufferSizeSamples;

				// configure channel
				g_BufferInfo[ChannelCounter].channelNum = i;
				g_BufferInfo[ChannelCounter].isInput = ASIOTrue;
				g_BufferInfo[ChannelCounter].buffers[0] = bufferPointer;
				bufferPointer += AdvanceBy;
				g_BufferInfo[ChannelCounter].buffers[1] = bufferPointer;
				bufferPointer += AdvanceBy;

				ChannelCounter++;
			}
		}
		for (int i = 0; i < g_numOutputChannels; i++) {
			// get channel info to get sample byte size
			UINT32 AdvanceBy = GetSampleByteSize(g_OutputChannelInfo[i].type) * g_PreferredBufferSizeSamples;

			// configure channel
			g_BufferInfo[ChannelCounter].channelNum = i;
			g_BufferInfo[ChannelCounter].isInput = ASIOFalse;
			g_BufferInfo[ChannelCounter].buffers[0] = bufferPointer;
			bufferPointer += AdvanceBy;
			g_BufferInfo[ChannelCounter].buffers[1] = bufferPointer;
			bufferPointer += AdvanceBy;

			ChannelCounter++;
		}

#ifdef _DEBUG
		printf("done\n");
#endif

		ASIOCallbacks callbacks;
		callbacks.asioMessage = OnAsioMessage;
		if (TRUE == IsRecording) {
			callbacks.bufferSwitch = OnBufferSwitchCapture;
		}
		else {
			callbacks.bufferSwitch = OnBufferSwitchRender;
		}
		callbacks.bufferSwitchTimeInfo = OnBufferSwitchTimeInfo;
		callbacks.sampleRateDidChange = OnSampleRateDidChange;

#ifdef _DEBUG
		printf("Creating buffers...");
#endif
		err = pDevice->pAsio->createBuffers(g_BufferInfo, TotalChannels, 
			g_PreferredBufferSizeSamples, &callbacks);
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
		if (g_TransferCaptureBufferNCh) {
			HeapFree(GetProcessHeap(), 0, g_TransferCaptureBufferNCh);
			g_TransferCaptureBufferNCh = nullptr;
		}
		if (g_TransferRenderBuffer2Ch) {
			HeapFree(GetProcessHeap(), 0, g_TransferRenderBuffer2Ch);
			g_TransferRenderBuffer2Ch = nullptr;
		}
		if (BigBuffer != NULL) {
			HeapFree(GetProcessHeap(), 0, BigBuffer);
			BigBuffer = nullptr;
		}
		if (g_InputChannelInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, g_InputChannelInfo);
			g_InputChannelInfo = nullptr;
		}
		if (g_OutputChannelInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, g_OutputChannelInfo);
			g_OutputChannelInfo = nullptr;
		}
		if (g_BufferInfo != NULL) {
			HeapFree(GetProcessHeap(), 0, g_BufferInfo);
			g_BufferInfo = nullptr;
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