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

#include "gaps.h"

u32 FASTCALL ReadGaps32(u32 mem) {
	switch (mem & 0xffff) {
	case 0x0400: return 0x53504147;	//GAPS
	case 0x0800: return 0x53504147;	//GAPS
	case 0x1400: return 0x53504147;	//GAPS
	case 0x1800: return 0x53504147;	//GAPS
	default:     return 0;
	}
}

void FASTCALL WriteGaps32(u32 mem, u32 value) {
}