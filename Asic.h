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

#ifndef __ASIC_H__
#define __ASIC_H__

#include "types.h"

#define ASIC8(x)    ASIC8_UNMASKED((x) & 0xffff)
#define ASIC16(x)   ASIC16_UNMASKED((x) & 0xffff)
#define ASIC32(x)   ASIC32_UNMASKED((x) & 0xffff)

#define ASIC8_UNMASKED(x)   (*(u8*)&pAsic[x])
#define ASIC16_UNMASKED(x)  (*(u16*)&pAsic[x])
#define ASIC32_UNMASKED(x)  (*(u32*)&pAsic[x])

u8 pAsic[0x00010000];

void MapleDma();
void GdromDma();
void PvrDma();
void AicaDma(u32 ch);

u8 FASTCALL ReadAsic8(u32 mem);
u16 FASTCALL ReadAsic16(u32 mem);
u32 FASTCALL ReadAsic32(u32 mem);

void FASTCALL WriteAsic8(u32 mem, u8 value);
void FASTCALL WriteAsic16(u32 mem, u16 value);
void FASTCALL WriteAsic32(u32 mem, u32 value);

#endif