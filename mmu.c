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

#include "mmu.h"

u32 FASTCALL ReadItlbAddr32(u32 mem) {
	return ItlbAddr[(mem >> 8) & 0x03];
}

void FASTCALL WriteItlbAddr32(u32 mem, u32 value) {
	ItlbAddr[(mem >> 8) & 0x03] = value;
}

u32 FASTCALL ReadItlbData32(u32 mem) {
	if (mem & 0x00800000) return ItlbData[((mem >> 8) & 0x03) + 4];
	else return ItlbData[((mem >> 8) & 0x03) + 0];
}

void FASTCALL WriteItlbData32(u32 mem, u32 value) {
	if (mem & 0x00800000) ItlbData[((mem >> 8) & 0x03) + 4] = value;
	else ItlbData[((mem >> 8) & 0x03) + 0] = value;
}

u32 FASTCALL ReadUtlbAddr32(u32 mem) {
	return UtlbAddr[(mem >> 8) & 0x3f];
}

void FASTCALL WriteUtlbAddr32(u32 mem, u32 value) {
	UtlbAddr[(mem >> 8) & 0x3f] = value;
}

u32 FASTCALL ReadUtlbData32(u32 mem) {
	if (mem & 0x00800000) return UtlbData[((mem >> 8) & 0x3f) + 64];
	else return UtlbData[((mem >> 8) & 0x3f) + 0];
}

void FASTCALL WriteUtlbData32(u32 mem, u32 value) {
	if (mem & 0x00800000) UtlbData[((mem >> 8) & 0x3f) + 64] = value;
	else UtlbData[((mem >> 8) & 0x3f) + 0] = value;
}

u32  sqMemTranslate(u32 addr) {
	return (UtlbData[((addr >> 20) & 0x3f) + 1] & 0x1ff00000) | (addr & 0xfffe0);
}