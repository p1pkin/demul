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

#include "types.h"
#include "asic.h"
#include "gdrom.h"
#include "sh4.h"
#include "plugins.h"
#include "demul.h"

u8 FASTCALL ReadAsic8(u32 mem) {
	u32 memMasked = mem & 0x007fffff;
	if ((memMasked >= 0x005f7018) &&
		(memMasked <= 0x005f709c))
		return ReadGdrom8(mem);
	return ASIC8(mem);
}

u16 FASTCALL ReadAsic16(u32 mem) {
	u32 memMasked = mem & 0x007fffff;
	if ((memMasked >= 0x005f7018) &&
		(memMasked <= 0x005f709c))
		return ReadGdrom16(mem);
	return ASIC16(mem);
}

u32 g2fifo = 0;

u32 FASTCALL ReadAsic32(u32 mem) {
	u32 memMasked = mem & 0x007fffff;
	if (memMasked >= 0x005f8000) {
		switch (memMasked) {
		case 0x005f8000: return 0x17fd11db;
		case 0x005f8004: return 0x11;
		case 0x005f810c: return ASIC32_UNMASKED(0x810c);
		default:         return gpuRead32(mem);
		}
	}

	switch (memMasked) {
	case 0x005f6880: return 0x8;
	case 0x005f688c: return(g2fifo ^= 1);
	case 0x005f689c: return 0xb;
	default:         return ASIC32(mem);
	}
}

void FASTCALL WriteAsic8(u32 mem, u8 value) {
	u32 memMasked = mem & 0x007fffff;
	if (((memMasked) >= 0x005f7018) &&
		((memMasked) <= 0x005f709c)) {
		WriteGdrom8(mem, value);
		return;
	}

	ASIC8(mem) = value;
}

void FASTCALL WriteAsic16(u32 mem, u16 value) {
	u32 memMasked = mem & 0x007fffff;
	if (((memMasked) >= 0x005f7018) &&
		((memMasked) <= 0x005f709c)) {
		WriteGdrom16(mem, value);
		return;
	}

	ASIC16(mem) = value;
}

void FASTCALL WriteAsic32(u32 mem, u32 value) {
	u32 memMasked = mem & 0x007fffff;
	if ((memMasked) >= 0x005f8000)
		gpuWrite32(mem, value);

	switch (memMasked) {
	case 0x005f6900:	ASIC32(0x6900) &= ~value;   return;
	case 0x005f6904:	ASIC32(0x6904) &= ~value;   return;
	case 0x005f6908:	ASIC32(0x6908) &= ~value;   return;
	case 0x005f6808:	if (value & 1) PvrDma(); return;
	case 0x005f6c18:	if (value & 1) MapleDma(); return;
	case 0x005f7414:
	case 0x005f7418:	ASIC32(mem) = value; GdromDma(); return;
	case 0x005f7814:
	case 0x005f7818:	ASIC32(mem) = value; AicaDma(0); return;
	case 0x005f7834:
	case 0x005f7838:	ASIC32(mem) = value; AicaDma(1); return;
	case 0x005f7854:
	case 0x005f7858:	ASIC32(mem) = value; AicaDma(2); return;
	case 0x005f7874:
	case 0x005f7878:	ASIC32(mem) = value; AicaDma(3); return;
	default:			ASIC32(mem) = value; return;
	}
}
