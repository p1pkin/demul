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

#ifndef __AICA_H__
#define __AICA_H__

#include "types.h"

#define AICA8(x)    AICA8_UNMASKED((x) & 0xffff)
#define AICA16(x)   AICA16_UNMASKED((x) & 0xffff)
#define AICA32(x)   AICA32_UNMASKED((x) & 0xffff)

#define AICA8_UNMASKED(x)   (*(u8*)&pAica[x])
#define AICA16_UNMASKED(x)  (*(u16*)&pAica[x])
#define AICA32_UNMASKED(x)  (*(u32*)&pAica[x])

#define ARAM8(x)    ARAM8_UNMASKED((x) & 0x001fffff)
#define ARAM16(x)   ARAM16_UNMASKED((x) & 0x001fffff)
#define ARAM32(x)   ARAM32_UNMASKED((x) & 0x001fffff)

#define ARAM8_UNMASKED(x)   (*(u8*)&pspu[x])
#define ARAM16_UNMASKED(x)  (*(u16*)&pspu[x])
#define ARAM32_UNMASKED(x)  (*(u32*)&pspu[x])

#define CABLE 0x00000200

u8 pAica[0x00010000];

u8 FASTCALL ReadAica8(u32 mem);
u16 FASTCALL ReadAica16(u32 mem);
u32 FASTCALL ReadAica32(u32 mem);

void FASTCALL WriteAica8(u32 mem, u8 value);
void FASTCALL WriteAica16(u32 mem, u16 value);
void FASTCALL WriteAica32(u32 mem, u32 value);

#endif