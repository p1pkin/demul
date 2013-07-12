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

#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"

u8 pCram[0x00004000];

u32 SQ0[8 + 8];

void FASTCALL WriteSQ32(u32 mem, u32 value);
void FASTCALL WriteSQ64(u32 mem, u64 *value);

u8 FASTCALL ReadCacheRam8(u32 mem);
u16 FASTCALL ReadCacheRam16(u32 mem);
u32 FASTCALL ReadCacheRam32(u32 mem);
void FASTCALL ReadCacheRam64(u32 mem, u64 *out);

void FASTCALL WriteCacheRam8(u32 mem, u8 value);
void FASTCALL WriteCacheRam16(u32 mem, u16 value);
void FASTCALL WriteCacheRam32(u32 mem, u32 value);
void FASTCALL WriteCacheRam64(u32 mem, u64 *value);

#endif