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

#ifndef __SPUREGS_H__
#define __SPUREGS_H__

#define REGS8(x)    REGS8_UNMASKED((x) & 0xffff)
#define REGS16(x)   REGS16_UNMASKED((x) & 0xffff)
#define REGS32(x)   REGS32_UNMASKED((x) & 0xffff)

#define REGS8_UNMASKED(x)   (*(u8*)&REGS[x])
#define REGS16_UNMASKED(x)  (*(u16*)&REGS[x])
#define REGS32_UNMASKED(x)  (*(u32*)&REGS[x])


#include "types.h"


//channel recordsize
#define CHANNEL_RECORD_SIZE 0x00000080

#define CH_FORMAT_PCM16 0
#define CH_FORMAT_PCM8 1
#define CH_FORMAT_ADPCM1 2
#define CH_FORMAT_ADPCM2 3



extern u8 REGS[0x10000];


u8 FASTCALL spuRead8(u32 mem);
u16 FASTCALL spuRead16(u32 mem);
u32 FASTCALL spuRead32(u32 mem);

void FASTCALL spuWrite8(u32 mem, u8 value);
void FASTCALL spuWrite16(u32 mem, u16 value);
void FASTCALL spuWrite32(u32 mem, u32 value);

#endif