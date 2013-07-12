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

#include <time.h>
#include "rts.h"

#define TWENTY_YEARS ((20 * 365LU + 5) * 24 * 60 * 60)

u32 REALTIME = 0x5bfc8900;

u32 FASTCALL ReadRTS32(u32 mem) {
	switch (mem & 0xf) {
	case 0:		return (REALTIME >> 16) & 0xffff;
	case 4:		return (REALTIME >> 0) & 0xffff;
	default:	return 0;
	}
}

void FASTCALL WriteRTS32(u32 mem, u32 value) {
	switch (mem & 0xf) {
	case 0:
		REALTIME &= 0x0000ffff;
		REALTIME |= value << 16;
		return;

	case 4:
		REALTIME &= 0xffff0000;
		REALTIME |= value << 0;
		return;

	default:
		return;
	}
}