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

#include "gpu.h"
#include "sh4.h"
#include "asic.h"
#include "plugins.h"

void FASTCALL gpuIrq(u32 code) {
	hwInt(code);
}

void FASTCALL hblank() {
	u32 count = ASIC32(0x810c) & 0x3ff;

	ASIC32(0x810c) &= ~0x2000;

	if (count == ((ASIC32(0x80cc) >> 16) & 0x3ff)) hwInt(0x0004);
	if (count == ((ASIC32(0x80cc) >> 0) & 0x3ff)) hwInt(0x0003);

	if (count == ((ASIC32(0x80f0) >> 0) & 0x3ff)) {
		ASIC32(0x810c) |= 0x2000;
		hwInt(0x0005);
	}

	count++;

	if (count > 524) {
		gpuSync();
		count = 0;
	}

	ASIC32(0x810c) &= ~0x3ff;
	ASIC32(0x810c) |= count;

//	spuSync();
}
