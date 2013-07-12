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
#include <windows.h>
#include <math.h>

#include "spudevice.h"
#include "spuRegs.h"
#include "spu.h"

#define CH_REC_FORMAT_KEY_LOOP          0x01
#define CH_REC_FNS                      0x18
#define CH_REC_FNS_OCT                  0x19
#define CH_REC_AR_D1R                   0x10
#define CH_REC_D1R_D2R                  0x11
#define CH_REC_RR_DL                    0x14
#define CH_REC_DL_KRS_LS                0x15
#define CH_REC_ALFOS_ALFOWS_PLFOS       0x1c
#define CH_REC_PLFOWS_LFOF_RE           0x1d



u8 REGS[0x10000];
ChannelInfo channels[MAX_CHANNELS];


static const float tone[16] =
{
	0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
	-8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f
};
/*
INLINE static void CalcFrequency(ChannelInfo *currentChannel)
{
    currentChannel->frequency = (float)(44100 * (pow(2, tone[currentChannel->data->oct] * (1.0f + (float)currentChannel->data->fns / 1024.0f))));
    if (currentChannel->frequency < 44100)
    {
        currentChannel->frequency = currentChannel->frequency;
    }
    if (currentChannel->frequency > 44100)
    {
        currentChannel->frequency = currentChannel->frequency;
    }
}
*/
u8 FASTCALL spuRead8(u32 mem) {
	switch (mem & 0xffff) {
	case 0x2808:    return 0x00;
	case 0x2809:    return 0x1;
	case 0x2814: {
		return 1111;
	} break;
	default:        return REGS32(mem);
	}
}

u16 FASTCALL spuRead16(u32 mem) {
	switch (mem & 0xffff) {
	case 0x2808:    return 0x100;
	case 0x2814: {
		return 1111;
	} break;
	default:        return REGS16(mem);
	}
}

u32 FASTCALL spuRead32(u32 mem) {
	switch (mem & 0xffff) {
	case 0x2808:    return 0x100;
	case 0x2814:    return channels[REGS8(0x280d) & 0x3f].position;
	default:        return REGS32(mem);
	}
}

// accept trimed high bits
void writeChannel(ChannelInfo *channelInfo, u32 channelMem, u8 value) {
	switch (channelMem) {
	case CH_REC_FORMAT_KEY_LOOP:
		if (channelInfo->data->keyOnEx) {
			int i;

			for (i = 0; i < MAX_CHANNELS; i++) {
				ChannelInfo *channelInfo2 = (ChannelInfo*)&channels[i];

				if (channelInfo2->data->keyOnB && !channelInfo2->playing) {
					StartChannel(channelInfo2);
				}

				if (!channelInfo2->data->keyOnB && channelInfo2->playing) {
					StopChannel(channelInfo2);
				}
			}

			channelInfo->data->keyOnEx = 0;
		}
		break;

	case CH_REC_FNS:
	case CH_REC_FNS_OCT:
		CalcStep(channelInfo);
		break;
	case CH_REC_RR_DL:
	case CH_REC_DL_KRS_LS:
		channelInfo->eg.rr = GetRR(channelInfo, 0);
		channelInfo->eg.dl = 0x1f - channelInfo->data->dl;
		break;
	}
}
void FASTCALL spuWrite8(u32 mem, u8 value) {
	u32 channelIdx;
	u32 channelMem;
	ChannelInfo *channelInfo;
	mem &= 0xffff;
	REGS8_UNMASKED(mem) = value;
	if (mem < CHANNEL_RECORD_SIZE * MAX_CHANNELS) {
		channelIdx = mem / CHANNEL_RECORD_SIZE;
		channelMem = mem % CHANNEL_RECORD_SIZE;
		channelInfo = (ChannelInfo*)&channels[channelIdx];
		writeChannel(channelInfo, channelMem, value);
	}
}

void FASTCALL spuWrite16(u32 mem, u16 value) {
	u32 channelIdx;
	u32 channelMem;
	ChannelInfo *channelInfo;
	mem &= 0xffff;
	REGS16_UNMASKED(mem) = value;
	if (mem < CHANNEL_RECORD_SIZE * MAX_CHANNELS) {
		channelIdx = mem / CHANNEL_RECORD_SIZE;
		channelMem = mem % CHANNEL_RECORD_SIZE;
		channelInfo = (ChannelInfo*)&channels[channelIdx];
		writeChannel(channelInfo, channelMem + 0, (value >> 0) & 0xFF);
		writeChannel(channelInfo, channelMem + 1, (value >> 8) & 0xFF);
	}
}

void FASTCALL spuWrite32(u32 mem, u32 value) {
	u32 channelIdx;
	u32 channelMem;
	ChannelInfo *channelInfo;
	mem &= 0xffff;
	REGS32_UNMASKED(mem) = value;
	if (mem < CHANNEL_RECORD_SIZE * MAX_CHANNELS) {
		channelIdx = mem / CHANNEL_RECORD_SIZE;
		channelMem = mem % CHANNEL_RECORD_SIZE;
		channelInfo = (ChannelInfo*)&channels[channelIdx];
		writeChannel(channelInfo, channelMem + 0, (value >> 0) & 0xFF);
		writeChannel(channelInfo, channelMem + 1, (value >> 8) & 0xFF);
		writeChannel(channelInfo, channelMem + 2, (value >> 16) & 0xFF);
		writeChannel(channelInfo, channelMem + 3, (value >> 24) & 0xFF);
	}
}