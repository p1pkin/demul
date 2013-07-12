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

#ifndef __REGS_H__
#define __REGS_H__

#include "types.h"

//#define REGS_DEBUG

#define REGS8(x)    (*(u8*)&REGS[(x)])
#define REGS16(x)   (*(u16*)&REGS[(x)])
#define REGS32(x)   (*(u32*)&REGS[(x)])

u8 REGS[0x10000];
u32 renderingison;

u32 FASTCALL gpuRead32(u32 mem);
void FASTCALL gpuWrite32(u32 mem, u32 value);

#endif
