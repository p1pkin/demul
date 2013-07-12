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

#ifndef __SPU_H__
#define __SPU_H__

#include "types.h"


//#define _DEBUG_SPU
//#define _DEBUG_SPU2

#ifdef _DEBUG_SPU
#include <stdio.h>
#endif

#ifdef _DEBUG_SPU2
#include <stdio.h>
#endif


#pragma pack(push, 1)
typedef struct {
	u32 saHi : 7;						//0x00
	u32 pcms : 2;
	u32 lpctl : 1;
	u32 ssctl : 1;
	u32 unused0 : 3;
	u32 keyOnB : 1;
	u32 keyOnEx : 1;
	u32 unused1 : 16;

	u32 saLow : 16;						//0x04
	u32 unused2 : 16;

	u32 lsa : 16;						//0x08
	u32 unused3 : 16;

	u32 lea : 16;						//0x0c
	u32 unused4 : 16;

	u32 ar : 5;							//0x10
	u32 eghold : 1;
	u32 d1r : 5;
	u32 d2r : 5;
	u32 unused6 : 16;

	u32 rr : 5;							//0x14
	u32 dl : 5;
	u32 krs : 4;
	u32 lpslink : 1;
	u32 unused7 : 17;

	u32 fns : 10;						//0x18
	u32 unused8 : 1;
	u32 oct : 4;
	u32 unused9 : 17;

	u32 alfos : 3;						//0x1c
	u32 alfows : 2;
	u32 plfos : 3;
	u32 plfows : 2;
	u32 lfof : 5;
	u32 re : 1;
	u32 unused10 : 16;

	u32 unused11 : 3;					//0x20
	u32 isel : 4;
	u32 tl : 8;
	u32 unused12 : 17;

	u32 dipan : 5;						//0x24
	u32 unused13 : 3;
	u32 disdl : 4;
	u32 imxl : 4;
	u32 unused14 : 16;

	u32 q : 5;							//0x28
	u32 unused15 : 27;

	u32 flv0 : 12;						//0x2c
	u32 unused16 : 20;

	u32 flv1 : 12;						//0x30
	u32 unused17 : 20;

	u32 flv2 : 12;						//0x34
	u32 unused18 : 20;

	u32 flv3 : 12;						//0x38
	u32 unused19 : 20;

	u32 flv4 : 12;						//0x3c
	u32 unused20 : 20;

	u32 fd1r : 5;						//0x40
	u32 unused21 : 3;
	u32 far : 5;
	u32 unused22 : 19;

	u32 frr : 5;						//0x44
	u32 unused23 : 3;
	u32 fd2r : 5;
	u32 unused24 : 19;

	u32 pading[14];					//0x48 - 0x7c unknown
} ChannelData;
#pragma pack(pop)

typedef enum { ATTACK, DECAY1, DECAY2, RELEASE } EGState;

typedef struct {
	s32 volume;
	EGState state;

	s32 ar;		//Attack
	s32 d1r;	//Decay1
	s32 d2r;	//Decay2
	s32 rr;		//Release
	s32 dl;		//Decay level

	u8 eghold;
} EG;

typedef struct {
	u8 *mem;

	bool playing;
	u16 position;
	s32 adpcmStep;
	s32 adpcmSignal;
	s32 loopAdpcmStep;
	s32 loopAdpcmSignal;
	u32 loopCount;
	u32 step;
	u32 bufferPosition;
	float frequency;
	EG eg;
	ChannelData *data;
#ifdef _DEBUG_SPU2
	FILE *file;
#endif
} ChannelInfo;

//AICA channels count
#define MAX_CHANNELS 64

#define TICKS_PER_SEC 60
#define EVENTS_COUNT (TICKS_PER_SEC + 1)
#define CLOSE_EVENT (TICKS_PER_SEC)
#define SAMPLES_PER_TICK ((int)(44100 / TICKS_PER_SEC))
#define BYTES_PER_TICK (SAMPLES_PER_TICK * 2 * 2)
#define BUFFER_SIZE  (44100 * 2 * 2)

extern ChannelInfo channels[MAX_CHANNELS];
extern s32 frameBufferL[SAMPLES_PER_TICK];
extern s32 frameBufferR[SAMPLES_PER_TICK];


void fillPCM16(ChannelInfo *channelInfo);
void fillPCM8(ChannelInfo *channelInfo);
void fillADPCM(ChannelInfo *channelInfo);
void WriteFrameBufferToMainBuffer(s16 *buffer);
void CalcStep(ChannelInfo *channelInfo);
void StartChannel(ChannelInfo *channelInfo);
void StopChannel(ChannelInfo *channelInfo);
void CalculateTable();
s32 GetRR(ChannelInfo *channelInfo, s32 base);

#endif