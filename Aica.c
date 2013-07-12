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

#include <stdio.h>

#include "console.h"
#include "aica.h"
#include "sh4.h"
#include "arm.h"
#include "counters.h"
#include "plugins.h"

u8 FASTCALL ReadAica8(u32 mem) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		return spuRead8(mem);
	}

	switch (memMasked) {
	default:    return AICA8_UNMASKED(memMasked);
	}
}

u16 FASTCALL ReadAica16(u32 mem) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		return spuRead16(mem);
	}

	switch (memMasked) {
	default:    return AICA16_UNMASKED(memMasked);
	}
}

u32 FASTCALL ReadAica32(u32 mem) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		return spuRead32(mem);
	}

	switch (memMasked) {
	case 0x2c00:    return AICA32_UNMASKED(0x2c00) | CABLE;
	default:        return AICA32_UNMASKED(memMasked);
	}
}

void FASTCALL WriteAica8(u32 mem, u8 value) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		spuWrite8(mem, value);
	}

	switch (memMasked) {
	default: AICA8_UNMASKED(memMasked) = value; break;
	}
}

void FASTCALL WriteAica16(u32 mem, u16 value) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		spuWrite16(mem, value);
	}

	switch (mem & 0xffff) {
	default: AICA16_UNMASKED(memMasked) = value;    break;
	}
}

void FASTCALL WriteAica32(u32 mem, u32 value) {
	u32 memMasked = mem & 0xffff;

	if ((memMasked < 0x2890) ||
		(memMasked > 0x2d04)) {
		spuWrite32(mem, value);
	}

	switch (memMasked) {
	case 0x28a4:    AICA32_UNMASKED(0x28a0) &= ~value;
		AICA32_UNMASKED(0x28a4) = value;
		break;

	case 0x28bc:    AICA32_UNMASKED(0x28b8) &= ~value;
		AICA32_UNMASKED(0x28bc) = value;
		break;

	case 0x2c00:    AICA32_UNMASKED(0x2c00) = value;
		if (value & 1)
			armReset();
		break;

	default:        AICA32_UNMASKED(memMasked) = value;
		break;
	}
}