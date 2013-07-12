/*
 * Demul
 * Copyright (C) 2006 Demul Team
 *
 * Demul is not free software: you can't redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Demul isn't distributed in the hope that it will be useful,
 * and WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <string.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <math.h>
#include <dsound.h>
#include <assert.h>

#include "spudevice.h"
#include "spu.h"
#include "spuRegs.h"

u8   *ARAM = NULL;

LPDIRECTSOUND lpDS;
LPDIRECTSOUNDBUFFER lpDSBP = NULL;
LPDIRECTSOUNDBUFFER lpDSBS = NULL;
LPDIRECTSOUNDNOTIFY lpDSNotify = NULL;
DSBPOSITIONNOTIFY notify[TICKS_PER_SEC];

u32 mainBufferPosition = -1;

volatile HANDLE soundThreadHandle = NULL;
HANDLE soundEvents[EVENTS_COUNT];

INLINE static bool CHECK_ERROR(HRESULT result, char *message) {
	static char errorString[300];
	if (result != S_OK) {
		sprintf(errorString, "Error! (HRESULT = %d) %s\n", result, message);
		MessageBox(GetActiveWindow(), errorString, SPU_MODULE_NAME, MB_ICONERROR);
		return false;
	}
	return true;
}


void CDECL SoundThreadProc(void *p) {
	DWORD dwWait;
	SetThreadPriority(soundThreadHandle, THREAD_PRIORITY_TIME_CRITICAL);

	while (1) {
		dwWait = WaitForMultipleObjects(EVENTS_COUNT, soundEvents, false, 50);
		if (dwWait == WAIT_FAILED)
			break;
		if (dwWait == CLOSE_EVENT)
			break;
/*		if (dwWait == WAIT_TIMEOUT)
            continue;*/
		if (dwWait < TICKS_PER_SEC)
			mainBufferPosition = dwWait;
		mainBufferPosition++;
		if (mainBufferPosition >= TICKS_PER_SEC)
			mainBufferPosition = 0;
		DeviceSync();
	}

	soundThreadHandle = NULL;
	_endthread();
}

void DeviceSync() {
	u32 i;
	ChannelInfo *channelInfo;
	u8 *ptr1;
	u8 *ptr2;
	u32 len1, len2;
//	u32 playPos;
//	u32 writePos;
	u32 offset;


	memset(frameBufferL, 0, SAMPLES_PER_TICK * sizeof(s32));
	memset(frameBufferR, 0, SAMPLES_PER_TICK * sizeof(s32));

	for (i = 0; i < MAX_CHANNELS; i++) {
		channelInfo = &channels[i];
		//fill
		if (channelInfo->playing) {
			switch (channelInfo->data->pcms) {
			case CH_FORMAT_PCM16:
				fillPCM16(channelInfo);
				break;
			case CH_FORMAT_PCM8:
				fillPCM8(channelInfo);
				break;
			case CH_FORMAT_ADPCM1:	//??
			case CH_FORMAT_ADPCM2:
				fillADPCM(channelInfo);
				break;
			}
		}
	}


/*	if (mainBufferPosition == -1)
    {
        if (!CHECK_ERROR(IDirectSoundBuffer_GetCurrentPosition(lpDSBS, &playPos, &writePos), "GetCurrentPosition"))
            return;

        mainBufferPosition = writePos / BYTES_PER_TICK;
        mainBufferPosition ++;
    }
*/
	offset = mainBufferPosition * BYTES_PER_TICK;
	//IDirectSoundBuffer_SetCurrentPosition(lpDSBS, offset);
	//mainBufferPosition ++;

/*	if (!CHECK_ERROR(IDirectSoundBuffer_GetCurrentPosition(lpDSBS, &playPos, &writePos), "GetCurrentPosition"))
        return;
    offset = playPos / BYTES_PER_TICK;
    offset *= BYTES_PER_TICK;
    offset -= BYTES_PER_TICK;
    if (offset >= BUFFER_SIZE)
        offset -= BUFFER_SIZE;
*/

	assert(offset < BUFFER_SIZE);
	if (IDirectSoundBuffer_Lock(lpDSBS, offset, BYTES_PER_TICK, &ptr1, &len1, &ptr2, &len2, 0) != DS_OK) {
		mainBufferPosition = -1;
		return;
	}

	WriteFrameBufferToMainBuffer((s16*)ptr1);


	IDirectSoundBuffer_Unlock(lpDSBS, ptr1, len1, ptr2, len2);
}

bool OpenDevice() {
	int i;
	DSBUFFERDESC dsbd;
	DSBUFFERDESC dsbdesc;
	WAVEFORMATEX wf;

	CalculateTable();

	for (i = 0; i < TICKS_PER_SEC; i++) {
		notify[i].dwOffset = i * BYTES_PER_TICK;
		notify[i].hEventNotify = soundEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	soundEvents[CLOSE_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!CHECK_ERROR(DirectSoundCreate(NULL, &lpDS, NULL), "DirectSoundCreate"))
		return false;

	if (!CHECK_ERROR(IDirectSound_SetCooperativeLevel(lpDS, GetActiveWindow(), DSSCL_EXCLUSIVE), "SetCooperativeLevel"))
		return false;

	memset(&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

	if (!CHECK_ERROR(IDirectSound_CreateSoundBuffer(lpDS, &dsbd, &lpDSBP, NULL), "Create Primary Sound Buffer"))
		return false;

	memset(&wf, 0, sizeof(WAVEFORMATEX));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 2;
	wf.wBitsPerSample = 16;
	wf.nSamplesPerSec = 44100;
	wf.nBlockAlign = ((wf.wBitsPerSample * wf.nChannels) / 8);
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	if (!CHECK_ERROR(IDirectSoundBuffer_SetFormat(lpDSBP, &wf), "SetFormat Primary Sound Buffer"))
		return false;

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;
	dsbdesc.dwBufferBytes = BUFFER_SIZE;
	dsbdesc.lpwfxFormat = (WAVEFORMATEX*)&wf;

	if (!CHECK_ERROR(IDirectSound_CreateSoundBuffer(lpDS, &dsbdesc, &lpDSBS, NULL), "Create Secondary Sound Buffer"))
		return false;

	if (!CHECK_ERROR(IUnknown_QueryInterface(lpDSBS, &IID_IDirectSoundNotify, (void**)&lpDSNotify), "DirectSoundNotify"))
		return false;


	if (!CHECK_ERROR(IDirectSoundNotify_SetNotificationPositions(lpDSNotify, TICKS_PER_SEC, notify), "SetNotificationPositions"))
		return false;

	if (!CHECK_ERROR(IDirectSoundBuffer_Play(lpDSBP, 0, 0, DSBPLAY_LOOPING), "Play Primary"))
		return false;

	if (!CHECK_ERROR(IDirectSoundBuffer_Play(lpDSBS, 0, 0, DSBPLAY_LOOPING), "Play Secondary"))
		return false;


	for (i = 0; i < MAX_CHANNELS; i++) {
		channels[i].data = (((ChannelData*)REGS) + i);
	}

	soundThreadHandle = (HANDLE)_beginthread(SoundThreadProc, 0, NULL);
	return true;
}

void CloseDevice() {
	int i;

	if (soundThreadHandle != NULL) {
		SetEvent(soundEvents[CLOSE_EVENT]);
		while (soundThreadHandle != NULL) {
			Sleep(0);
		}
	}

	for (i = 0; i < EVENTS_COUNT; i++) {
		CloseHandle(soundEvents[i]);
	}

	if (lpDSBS != NULL) {
		IDirectSoundBuffer_Stop(lpDSBS);
		IDirectSoundBuffer_Release(lpDSBS);
		lpDSBS = NULL;
	}

	if (lpDSBP != NULL) {
		IDirectSoundBuffer_Stop(lpDSBP);
		IDirectSoundBuffer_Release(lpDSBP);
		lpDSBP = NULL;
	}

	if (lpDS != NULL) {
		IDirectSound_Release(lpDS);
		lpDS = NULL;
	}
}

