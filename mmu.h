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

#ifndef __MMU_H__
#define __MMU_H__

#include "types.h"

u32 UtlbAddr[64];
u32 UtlbData[64 + 64];
u32 ItlbAddr[4];
u32 ItlbData[4 + 4];

u32 FASTCALL ReadItlbAddr32(u32 mem);
void FASTCALL WriteItlbAddr32(u32 mem, u32 value);

u32 FASTCALL ReadItlbData32(u32 mem);
void FASTCALL WriteItlbData32(u32 mem, u32 value);

u32 FASTCALL ReadUtlbAddr32(u32 mem);
void FASTCALL WriteUtlbAddr32(u32 mem, u32 value);

u32 FASTCALL ReadUtlbData32(u32 mem);
void FASTCALL WriteUtlbData32(u32 mem, u32 value);

u32  sqMemTranslate(u32 addr);

#endif