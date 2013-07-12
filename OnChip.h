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

#ifndef __ONCHIP_H__
#define __ONCHIP_H__

#include "types.h"

#define TOCR		(*(u8*)&pOnChip[0xd800])
#define TSTR		(*(u8*)&pOnChip[0xd804])
#define TCOR0		(*(u32*)&pOnChip[0xd808])
#define TCNT0		(*(u32*)&pOnChip[0xd80c])
#define TCR0		(*(u16*)&pOnChip[0xd810])
#define TCOR1		(*(u32*)&pOnChip[0xd814])
#define TCNT1		(*(u32*)&pOnChip[0xd818])
#define TCR1		(*(u16*)&pOnChip[0xd81c])
#define TCOR2		(*(u32*)&pOnChip[0xd820])
#define TCNT2		(*(u32*)&pOnChip[0xd824])
#define TCR2		(*(u16*)&pOnChip[0xd828])
#define TCPR2		(*(u32*)&pOnChip[0xd82c])
#define TUNF		(1 << 8)

#define ONCHIP8(x)  (*(u8*)&pOnChip[(x) & 0xffff])
#define ONCHIP16(x) (*(u16*)&pOnChip[(x) & 0xffff])
#define ONCHIP32(x) (*(u32*)&pOnChip[(x) & 0xffff])

u8 pOnChip[0x00010000];

u8 FASTCALL ReadOnChip8(u32 mem);
u16 FASTCALL ReadOnChip16(u32 mem);
u32 FASTCALL ReadOnChip32(u32 mem);

void FASTCALL WriteOnChip8(u32 mem, u8 value);
void FASTCALL WriteOnChip16(u32 mem, u16 value);
void FASTCALL WriteOnChip32(u32 mem, u32 value);

#endif