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

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "types.h"
#include "twiddle.h"
#include "gpuprim.h"

#define TWIDDLE(x) twiddlex[x]

static u32 mipmapcodeoffsvq[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000016, 0x00000056, 0x00000156, 0x00000556,
	0x00001556, 0x00005556, 0x00015556, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static u32 mipmapcodeoffs[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000018, 0x00000058, 0x00000158, 0x00000558, 0x00001558,
	0x00005558, 0x00015558, 0x00055558, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static u32 mipmapcodeoffsclut[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000002C, 0x000000AC, 0x000002AC, 0x00000AAC,
	0x00002AAC, 0x0000AAAC, 0x0002AAAC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

#define yuv2rs(y, u, v) (bound(((76283 * y) + (104595 * v) - 14608688) >> 16))
#define yuv2gs(y, u, v) (bound(((76283 * y) - (25624 * u) - (53281 * v) + 8879312) >> 16))
#define yuv2bs(y, u, v) (bound(((76283 * y) + (132252 * u) - 18148784) >> 16))

u32 INLINE bound(s32 x) {
	return (x < 0) ? 0 : (x > 255) ? 255 : x;
}

u8 *VRAM;
u8 *VRAMVALID;

typedef struct __TxtrCache {
	int name;
	u32 width;
	u32 height;
	u32 texaddr;
	TCW tcw;
	u32 size;
	u32 textureCRC;
	struct __TxtrCache *next;
	struct __TxtrCache *prev;
} TxtrCache;

TxtrCache *TxtrCacheData[0x00100000];

#define MAX_TEXTURE_CACHE_INDEX 15

void CreateTextureARGB1555(TxtrCache *texture);
void CreateTextureARGB0565(TxtrCache *texture);
void CreateTextureARGB4444(TxtrCache *texture);
void CreateTextureYUV422(TxtrCache *texture);
void CreateTextureCLUT4(TxtrCache *texture);
void CreateTextureCLUT8(TxtrCache *texture);

int  ClearTextures();
int  CreateTexture();
void DeleteTextures();
void DeleteTexture();

void dump_texture(char *name, void *addr, u32 size);

#endif