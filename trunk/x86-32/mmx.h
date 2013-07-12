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

#ifndef __MMX_H__
#define __MMX_H__

#include "x86.h"

typedef enum {
	MM0 = 0,
	MM1 = 1,
	MM2 = 2,
	MM3 = 3,
	MM4 = 4,
	MM5 = 5,
	MM6 = 6,
	MM7 = 7
} x86MMXRegType;

void MOV64MtoMMX(x86MMXRegType reg, u32 mem);

void MOV64MMXtoMMX(x86MMXRegType dst, x86MMXRegType src);

void MOV64MMXtoM(u32 mem, x86MMXRegType reg);

void EMMS();

#endif