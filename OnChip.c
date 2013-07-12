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

#include "onchip.h"
#include "counters.h"

u8 FASTCALL ReadOnChip8(u32 mem) {
	switch (mem & 0x1fffffff) {
	default: return ONCHIP8(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff));
	}
}

u16 FASTCALL ReadOnChip16(u32 mem) {
	switch (mem & 0x1fffffff) {
	case 0x1F800028:		//hack
	{
		u32 value = (-(pOnChip[0x801c] & 1) & ~0x1ff) + 0x400;
		*(u16*)&pOnChip[0x8028] += 1;

		if ((u32)(*(u16*)&pOnChip[0x8028]) > value)
			*(u16*)&pOnChip[0x8028] = 0;

		return *(u16*)&pOnChip[0x8028];
	}

	case 0x1F800030:		//hack
		if (*(u32*)&pOnChip[0x802c] == 0xa03f8) {
			return 3;
		}
		if (*(u32*)&pOnChip[0x802c] == 0xa03fb) {
			if (*(u16*)&pOnChip[0x8030] == 2) return 0;
			else return 3;
		}
		if (*(u32*)&pOnChip[0x802c] == 0xa03fc) {
			if (*(u16*)&pOnChip[0x8030] == 2) return 3;
			else return 0;
		}
		return 0x200;

	default: return ONCHIP16(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff));
	}
}

u32 FASTCALL ReadOnChip32(u32 mem) {
	switch (mem & 0x1fffffff) {
	default: return ONCHIP32(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff));
	}
}

void FASTCALL WriteOnChip8(u32 mem, u8 value) {
	ONCHIP8(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff)) = value;

/*	switch (mem & 0x1fffffff)
	{
	default: return;
	}*/
}

void FASTCALL WriteOnChip16(u32 mem, u16 value) {
	ONCHIP16(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff)) = value;

/*	switch (mem & 0x1fffffff)
	{
	default: return;
	}*/
}

void FASTCALL WriteOnChip32(u32 mem, u32 value) {
	ONCHIP32(((mem >> 8) & 0xff00) | ((mem >> 0) & 0x00ff)) = value;
/*
	switch (mem & 0x1fffffff)
	{
	default: return;
	}*/
}