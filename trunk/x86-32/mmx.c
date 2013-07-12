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

#include "mmx.h"

void MOV64MtoMMX(x86MMXRegType reg, u32 mem) {
	write16(0x6f0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOV64MMXtoMMX(x86MMXRegType dst, x86MMXRegType src) {
	write16(0x6f0f);
	ModRm(0, dst, src);
}

void MOV64MMXtoM(u32 mem, x86MMXRegType reg) {
	write16(0x7f0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void EMMS() {
	write16(0x770f);
}
