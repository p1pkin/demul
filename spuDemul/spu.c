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
#include <stdio.h>
#include <math.h>
#include <mmintrin.h>

#include "spu.h"
#include "spuregs.h"
#include "spudevice.h"

#define SHIFT    12
#define EG_SHIFT  8
#define LFO_SHIFT 8

#define LIMIT(val, min, max) if (val < min) val = min; else if (val > max) val = max
#define ICLIP16(x) (x < -32768) ? -32768 : ((x > 32767) ? 32767 : x)

#define FIX(v)  ((u32)((float)(1 << SHIFT) * (v)))
#define LFIX(v) ((u32)((float)(1 << LFO_SHIFT) * (v)))
#define DB(v)    LFIX(pow(10.0, v / 20.0))
#define CENTS(v) LFIX(pow(2.0, v / 1200.0))


s32 frameBufferL[SAMPLES_PER_TICK];
s32 frameBufferR[SAMPLES_PER_TICK];

/*
void copyFrameBufferToBuffer(u32 samples)
{
#ifdef _DEBUG_SPU
    char szTempName[MAX_PATH];
    FILE *f;
    s16 s;
    u32 i;
#endif
//	u32 j;
    u32 nextBufferPos = bufferPos + samples;
    if (nextBufferPos >= MAX_BUFFER_SAMPLES) {
        nextBufferPos -= MAX_BUFFER_SAMPLES;
        samples -= nextBufferPos;
        memcpy((void*)&bufferL[bufferPos], frameBufferL, samples * sizeof(s32));
        memcpy((void*)&bufferR[bufferPos], frameBufferR, samples * sizeof(s32));

#ifdef _DEBUG_SPU
    GetTempFileName(".", "spuMIX", 0, szTempName);
    f = fopen(szTempName,"wb");

    for(i = 0; i < MAX_BUFFER_SAMPLES; i++)
    {
        s = bufferL[i];
        fwrite(&s, 2, 1, f);
        s = bufferR[i];
        fwrite(&s, 2, 1, f);
    }
    fclose(f);
#endif
        memcpy(bufferL, &frameBufferL[samples], nextBufferPos * sizeof(s32));
        memcpy(bufferR, &frameBufferR[samples], nextBufferPos * sizeof(s32));
    } else {
        memcpy((void*)&bufferL[bufferPos], frameBufferL, samples * sizeof(s32));
        for(j=0;j<samples;j++)
            bufferR[bufferPos+j]=bufferPos;
        memcpy((void*)&bufferR[bufferPos], frameBufferR, samples * sizeof(s32));
    }
    bufferPos = nextBufferPos;
}

void WriteBufferToSoundBuffer(s16 *buffer, u32 samples)
{
    u32 i;

    for(i = 0; i < samples; i++)
    {
        s32 smpl = bufferL[readBufferPos];
        s32 smpr = bufferL[readBufferPos];
        smpl = ICLIP16(smpl);
        smpr = ICLIP16(smpr);
        *buffer++ = smpl;
        *buffer++ = smpr;
        readBufferPos++;
        if (readBufferPos >= MAX_BUFFER_SAMPLES)
            readBufferPos = 0;
    }
}
*/

void WriteFrameBufferToMainBuffer(s16 *buffer) {
	u32 i = SAMPLES_PER_TICK;
	s32 *_frameBufferL = frameBufferL;
	s32 *_frameBufferR = frameBufferR;

	while (i > 0) {
		s32 smpl = *_frameBufferL++;///>> 2;
		s32 smpr = *_frameBufferR++;///>> 2;
		smpl = ICLIP16(smpl);
		smpr = ICLIP16(smpr);
		*buffer++ = smpl;
		*buffer++ = smpr;
//		*_frameBufferL++;// = 0;
//		*_frameBufferR++;// = 0;
		i--;
	}
}

s32 LPANTABLE[0x10000], RPANTABLE[0x10000];

s32 PLFO_TRI[256], PLFO_SQR[256], PLFO_SAW[256], PLFO_NOI[256];
s32 ALFO_TRI[256], ALFO_SQR[256], ALFO_SAW[256], ALFO_NOI[256];

s32 PSCALES[8][256], ASCALES[8][256];
s32 ARTABLE[64], DRTABLE[64];

static float const LFOFreq[32] =
{
	0.17f, 0.19f, 0.23f, 0.27f,
	0.34f, 0.39f, 0.45f, 0.55f,
	0.68f, 0.78f, 0.92f, 1.10f,
	1.39f, 1.60f, 1.87f, 2.27f,
	2.87f, 3.31f, 3.92f, 4.79f,
	6.15f, 7.18f, 8.60f, 10.8f,
	14.4f, 17.2f, 21.5f, 28.7f,
	43.1f, 57.4f, 86.1f, 172.3f
};

static float const ASCALE[8] =
{
	0.0f, 0.4f, 0.8f, 1.5f,
	3.0f, 6.0f, 12.0f, 24.0f
};

static float const PSCALE[8] =
{
	0.0f, 7.0f, 13.5f, 27.0f,
	55.0f, 112.0f, 230.0f, 494.0f
};

static const double ARTimes[64] =
{
	100000, 100000, 8100.0, 6900.0,
	6000.0, 4800.0, 4000.0, 3400.0,
	3000.0, 2400.0, 2000.0, 1700.0,
	1500.0, 1200.0, 1000.0, 860.0,
	760.0, 600.0, 500.0, 430.0,
	380.0, 300.0, 250.0, 220.0,
	190.0, 50.0, 130.0, 110.0,
	95.0, 76.0, 63.0, 55.0,
	47.0, 38.0, 31.0, 27.0,
	24.0, 19.0, 15.0, 13.0,
	12.0, 9.4, 7.9, 6.8,
	6.0, 4.7, 3.8, 3.4,
	3.0, 2.4, 2.0, 1.8,
	1.6, 1.3, 1.1, 0.93,
	0.85, 0.65, 0.53, 0.44,
	0.40, 0.35, 0.0, 0.0
};

static const double DRTimes[64] =
{
	100000, 100000, 118200.0, 101300.0,
	88600.0, 70900.0, 59100.0, 50700.0,
	44300.0, 35500.0, 29600.0, 25300.0,
	22200.0, 17700.0, 14800.0, 12700.0,
	11100.0, 8900.0, 7400.0, 6300.0,
	5500.0, 4400.0, 3700.0, 3200.0,
	2800.0, 2200.0, 1800.0, 1600.0,
	1400.0, 1100.0, 920.0, 790.0,
	690.0, 550.0, 460.0, 390.0,
	340.0, 270.0, 230.0, 200.0,
	170.0, 140.0, 110.0, 98.0,
	85.0, 68.0, 57.0, 49.0,
	43.0, 34.0, 28.0, 25.0,
	22.0, 18.0, 14.0, 12.0,
	11.0, 8.5, 7.1, 6.1,
	5.4, 4.3, 3.6, 3.1
};

static float const SDLT[8] =
{
	-1000000.0, -36.0, -30.0, -24.0,
	-18.0, -12.0, -6.0, 0.0
};

void CalculateTable() {
	int i, j;

	for (i = 0; i < 256; i++) {
		int a, p;

		a = 255 - i;
		if (i < 128) p = i;
		else p = 255 - i;

		ALFO_SAW[i] = a;
		PLFO_SAW[i] = p;

		if (i < 128) {
			a = 255; p = 127;
		} else { a = 0; p = -128; }

		ALFO_SQR[i] = a;
		PLFO_SQR[i] = p;

		if (i < 128) a = 255 - (i * 2);
		else a = (i * 2) - 256;

		if (i < 64) p = i * 2;
		else if (i < 128) p = 255 - i * 2;
		else if (i < 192) p = 256 - i * 2;
		else p = i * 2 - 511;

		ALFO_TRI[i] = a;
		PLFO_TRI[i] = p;

		a = rand() & 0xff;
		p = 128 - a;

		ALFO_NOI[i] = a;
		PLFO_NOI[i] = p;
	}

	for (i = 0; i < 8; i++) {
		float limit = PSCALE[i];

		for (j = -128; j < 128; j++) {
			PSCALES[i][j + 128] = CENTS(((limit * (float)i) / 128.0));
		}

		limit = -ASCALE[i];

		for (j = 0; j < 256; j++) {
			ASCALES[i][j] = DB(((limit * (float)i) / 256.0));
		}
	}

	ARTABLE[0] = DRTABLE[0] = 0;
	ARTABLE[1] = DRTABLE[1] = 0;

	for (i = 2; i < 64; ++i) {
		double t, step, scale;

		t = ARTimes[i];

		if (t != 0.0) {
			step = (1023.0 * 1000.0) / ((float)44100.0f * t);
			scale = (double)(1 << EG_SHIFT);

			ARTABLE[i] = (int)(step * scale);
		} else {
			ARTABLE[i] = 1024 << EG_SHIFT;
		}

		t = DRTimes[i];

		step = (1023.0 * 1000.0) / ((float)44100.0f * t);
		scale = (double)(1 << EG_SHIFT);

		DRTABLE[i] = (int)(step * scale);
	}

	for (i = 0; i < 0x10000; i++) {
		int iTL = (i >> 0) & 0xff;
		int iPAN = (i >> 8) & 0x1f;
		int iSDL = (i >> 13) & 0x07;

		float fTL = 1.0f;
		float fDB = 0.0f;
		float fSDL = 1.0f;
		float fPAN = 1.0f;
		float fLPAN, fRPAN;

		if (iTL & 0x01) fDB -= 0.4f;
		if (iTL & 0x02) fDB -= 0.8f;
		if (iTL & 0x04) fDB -= 1.5f;
		if (iTL & 0x08) fDB -= 3.0f;
		if (iTL & 0x10) fDB -= 6.0f;
		if (iTL & 0x20) fDB -= 12.0f;
		if (iTL & 0x40) fDB -= 24.0f;
		if (iTL & 0x80) fDB -= 48.0f;

		fTL = (float)pow(10.0, fDB / 20.0);

		fDB = 0.0f;

		if (iPAN & 0x1) fDB -= 3;
		if (iPAN & 0x2) fDB -= 6;
		if (iPAN & 0x4) fDB -= 12;
		if (iPAN & 0x8) fDB -= 24;

		if (iPAN == 0xf) fPAN = 0.0;
		else fPAN = (float)pow(10.0, fDB / 20.0);

		if (iPAN < 0x10) {
			fLPAN = fPAN;
			fRPAN = 1.0;
		} else {
			fRPAN = fPAN;
			fLPAN = 1.0;
		}

		if (iSDL)
			fSDL = (float)pow(10.0, (SDLT[iSDL]) / 20.0);
		else
			fSDL = 0.0;

		LPANTABLE[i] = FIX(4.0f * fLPAN * fTL * fSDL);
		RPANTABLE[i] = FIX(4.0f * fRPAN * fTL * fSDL);
	}
}


s32 GetAR(ChannelInfo *channelInfo, s32 base) {
	s32 rate = base + (channelInfo->data->ar << 1);

	if (rate > 63) rate = 63;
	if (rate < 0) rate = 0;

	return ARTABLE[rate];
}

s32 GetD1R(ChannelInfo *channelInfo, s32 base) {
	s32 rate = base + (channelInfo->data->d1r << 1);

	if (rate > 63) rate = 63;
	if (rate < 0) rate = 0;

	return DRTABLE[rate];
}

s32 GetD2R(ChannelInfo *channelInfo, s32 base) {
	s32 rate = base + (channelInfo->data->d2r << 1);

	if (rate > 63) rate = 63;
	if (rate < 0) rate = 0;

	return DRTABLE[rate];
}

s32 GetRR(ChannelInfo *channelInfo, s32 base) {
	s32 rate = base + (channelInfo->data->rr << 1);

	if (rate > 63) rate = 63;
	if (rate < 0) rate = 0;

	return ARTABLE[rate];
}

void CalculateEG(ChannelInfo *channelInfo) {
	u32 oct = channelInfo->data->oct;
	u32 fns = channelInfo->data->fns;
	u32 krs = channelInfo->data->krs;
	s32 rate;

	if (oct & 8) oct = oct - 16;

	if (krs != 0xf)
		rate = 2 * (oct + krs) + ((fns >> 9) & 1);
	else
		rate = ((fns >> 9) & 1);

	channelInfo->eg.volume = 0;
	channelInfo->eg.state = ATTACK;
	channelInfo->eg.ar = GetAR(channelInfo, rate);
	channelInfo->eg.d1r = GetD1R(channelInfo, rate);
	channelInfo->eg.d2r = GetD2R(channelInfo, rate);
	channelInfo->eg.rr = GetRR(channelInfo, rate);
	channelInfo->eg.dl = 0x1f - channelInfo->data->dl;
	channelInfo->eg.eghold = channelInfo->data->eghold;
}

void CalcStep(ChannelInfo *channelInfo) {
	u32 oct = channelInfo->data->oct;
	u32 fns = channelInfo->data->fns;
	u32 Fn;
	u32 Fo = 44100;

/*	switch(channelInfo->data->pcms)
    {
    case CH_FORMAT_ADPCM1:
    case CH_FORMAT_ADPCM2:
        if (oct == 0)
            oct = 0xf;
        else
            oct--;
        break;
    }*/
	if (oct & 8)
		Fo >>= (16 - oct);
	else
		Fo <<= oct;

	Fn = (Fo * (((fns << (SHIFT - 10)) | (1 << SHIFT))));
	channelInfo->step = Fn / (44100);
}

void StartChannel(ChannelInfo *channelInfo) {
#ifdef _DEBUG_SPU2
	char szTempName[MAX_PATH];
	GetTempFileName(".", "spuCH", 0, szTempName);
	channelInfo->file = fopen(szTempName, "wb");
#endif
	channelInfo->playing = 1;
	channelInfo->mem = ARAM + ((channelInfo->data->saHi << 16) | (channelInfo->data->saLow));
	channelInfo->position = channelInfo->data->lsa;
	CalcStep(channelInfo);
	CalculateEG(channelInfo);
/*
    calculate_lfo(channel, channelInfo);*/
}

void StopChannel(ChannelInfo *channelInfo) {
#ifdef _DEBUG_SPU2
	if (channelInfo->file != NULL)
		fclose(channelInfo->file);
	channelInfo->file = NULL;
#endif
	channelInfo->playing = 0;
	channelInfo->data->keyOnB = 0;
}


u32 UpdateEG(ChannelInfo *channelInfo) {
	switch (channelInfo->eg.state) {
	case ATTACK:
		channelInfo->eg.volume += channelInfo->eg.ar;

		if (channelInfo->eg.volume >= (0x3ff << EG_SHIFT)) {
			channelInfo->eg.state = DECAY1;

			if (channelInfo->eg.d1r >= (1024 << EG_SHIFT)) {
				channelInfo->eg.state = DECAY2;
			}

			channelInfo->eg.volume = 0x3ff << EG_SHIFT;
		}

		if (channelInfo->eg.eghold) return 0x3ff << (SHIFT - 10);
		break;

	case DECAY1:
		channelInfo->eg.volume -= channelInfo->eg.d1r;

		if (channelInfo->eg.volume >> (EG_SHIFT + 5) >= channelInfo->eg.dl) {
			channelInfo->eg.state = DECAY2;
		}
		break;

	case DECAY2:
		if (channelInfo->eg.d2r == 0) {
			return (channelInfo->eg.volume >> EG_SHIFT) << (SHIFT - 10);
		}
		channelInfo->eg.volume -= channelInfo->eg.d2r;

		if (channelInfo->eg.volume <= 0) {
			channelInfo->eg.volume = 0;
		}
		break;

	case RELEASE:
		channelInfo->eg.volume -= channelInfo->eg.rr;

		if (channelInfo->eg.volume <= 0) {
			StopChannel(channelInfo);
			channelInfo->eg.volume = 0;
			channelInfo->eg.state = ATTACK;
		}
		break;

	default:
		return 1 << SHIFT;
	}

	return (channelInfo->eg.volume >> EG_SHIFT) << (SHIFT - 10);
}

void fillPCM16(ChannelInfo *channelInfo) {
	u32 samples = SAMPLES_PER_TICK;
	u16 position = channelInfo->position;
	u32 sPosition = position << SHIFT;

	s32*bL = frameBufferL;
	s32*bR = frameBufferR;
	s16*base = (s16*)channelInfo->mem;
	s32 sample;

	if (!channelInfo->data->lpctl) {
		while (samples > 0) {
			sample = base[position];
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

			*bL += sample;
			*bR += sample;
			bL++;
			bR++;
			if (position >= channelInfo->data->lea)
				break;
		}
	} else {
		while (samples > 0) {
			sample = base[position];
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

//			sample = (sample * UpdateEG(channelInfo)) >> SHIFT;

			*bL += sample;
			*bR += sample;
			bL++;
			bR++;

			if (position >= channelInfo->data->lea) {
				position = channelInfo->data->lsa;
				sPosition = position << SHIFT;
			}
		}
	}

	channelInfo->position = position;
}

void fillPCM8(ChannelInfo *channelInfo) {
	u32 samples = SAMPLES_PER_TICK;
	u16 position = channelInfo->position;
	u32 sPosition = position << SHIFT;

	s32*bL = frameBufferL;
	s32*bR = frameBufferR;
	s8*base = (s8*)channelInfo->mem;
	s32 sample;

	if (!channelInfo->data->lpctl) {
		while (samples > 0) {
			sample = base[position] << 8;
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

			*bL += sample;
			*bR += sample;
			bL++;
			bR++;

			if (position >= channelInfo->data->lea)
				break;
		}
	} else {
		while (samples > 0) {
			sample = base[position] << 8;
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

			*bL += sample;
			*bR += sample;
			bL++;
			bR++;

			if (position >= channelInfo->data->lea) {
				position = channelInfo->data->lsa;
				sPosition = position << SHIFT;
			}
		}
	}

	channelInfo->position = position;
}

static int diffLookup[16] = {
	1, 3, 5, 7, 9, 11, 13, 15,
	-1, -3, -5, -7, -9, -11, -13, -15,
};

static int indexScale[8] = {
	0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266
};

void fillADPCM(ChannelInfo *channelInfo) {
	u32 samples = SAMPLES_PER_TICK;
	u16 position = channelInfo->position;
	u32 sPosition = position << SHIFT;
	s32 signal = channelInfo->adpcmSignal;
	s32 step = channelInfo->adpcmStep;
	s32 val;
	s32 sample;
	s32*bL = frameBufferL;
	s32*bR = frameBufferR;
	u8*base = channelInfo->mem;
#ifdef _DEBUG_SPU2
	s16 val16;
#endif

	if (!channelInfo->data->lpctl) {
		while (samples > 0) {
			val = base[position / 2] >> ((~position & 1) << 10);
			signal += (step * diffLookup[val & 15]) / 8;
			LIMIT(signal, -32768, 32767);

			step = (step * indexScale[val & 7]) >> 8;
			LIMIT(step, 0x7f, 0x6000);

			sample = signal;
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

			*bL += sample;
			*bR += sample;
			bL++;
			bR++;

			if (position >= channelInfo->data->lea)	// or different end address ?
				break;
		}
	} else {
		while (samples > 0) {
			val = base[position / 2] >> ((~position & 1) << 10);
			signal += (step * diffLookup[val & 15]) / 8;
			LIMIT(signal, -32768, 32767);

			step = (step * indexScale[val & 7]) >> 8;
			LIMIT(step, 0x7f, 0x6000);

			sample = signal;
			samples--;

			sPosition += channelInfo->step;
			position = sPosition >> SHIFT;

//			sample = (sample * UpdateEG(channelInfo)) >> SHIFT;

			*bL += sample;
			*bR += sample;
#ifdef _DEBUG_SPU2
			val16 = sample;
			fwrite(&val16, 1, 2, channelInfo->file);
#endif
			bL++;
			bR++;

			if (position == channelInfo->data->lsa && channelInfo->loopCount == 0) {
				channelInfo->loopAdpcmSignal = signal;
				channelInfo->loopAdpcmStep = step;
			}
			if (position >= channelInfo->data->lea) {
				if (channelInfo->data->keyOnB) {
					position = channelInfo->data->lsa;
					sPosition = position << SHIFT;
					if (channelInfo->data->pcms == CH_FORMAT_ADPCM1) {
						signal = channelInfo->loopAdpcmSignal;
						step = channelInfo->loopAdpcmStep;
					}
					channelInfo->loopCount++;
				}
			}
		}
	}

	channelInfo->position = position;
	channelInfo->adpcmSignal = signal;
	channelInfo->adpcmStep = step;
}

