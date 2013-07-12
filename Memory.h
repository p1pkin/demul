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

#ifndef __MEMORY_H__
#define __MEMORY_H__

#define align_memory(p, bytes) (((int)(p) + (bytes)) & ~((bytes) - 1))

#define recInvalideBlock(mem, size) {						\
	int i;													\
	if (((mem) & 0x0c000000) == 0x0c000000) {				\
		int *p = (int*)&precram[((mem) & 0x00ffffff) >> 1];	\
		for (i = 0; i < (int)(size); i += 2) *p++ = 0;		\
	}														\
}

#include "types.h"

u8  *prom;
u8  *pram;
u8  *pflash;
u8  *pgpu;
u8  *pspu;

u8  *prec;
int precrom[0x00200000 >> 1];
int precram[0x01000000 >> 1];

int memAccess[0x00010000];

int  memOpen();
void memClose();

u32 FASTCALL memRead8(u32 mem);
u32 FASTCALL memRead16(u32 mem);
u32 FASTCALL memRead32(u32 mem);
void FASTCALL memRead64(u32 mem, u64 *out);

void FASTCALL memWrite8(u32 mem, u8 value);
void FASTCALL memWrite16(u32 mem, u16 value);
void FASTCALL memWrite32(u32 mem, u32 value);
void FASTCALL memWrite64(u32 mem, u64 *value);

bool  loadBios();
void memQuietMode(bool enable);

#endif
