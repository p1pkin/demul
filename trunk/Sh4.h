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

#ifndef __SH4_H__
#define __SH4_H__

#include "types.h"

#define hwInt(code) {												\
	switch (code >> 8)												\
	{																\
	case 0: *(u32*)&pAsic[0x6900] |= (1 << (code & 0xff));	break;	\
	case 1: *(u32*)&pAsic[0x6904] |= (1 << (code & 0xff));	break;	\
	case 2: *(u32*)&pAsic[0x6908] |= (1 << (code & 0xff));	break;	\
	default:												break;	\
	}																\
}

typedef struct {
	u32 r[16 + 8];

	union {
		float flt[32];
		double dbl[16];
		float vec[8][4];
		u32 flti[32];
		u64 dbli[16];
	};

	union {
		struct {
			u32 t : 1;
			u32 s : 1;
			u32 reserved0 : 2;
			u32 imask : 4;
			u32 q : 1;
			u32 m : 1;
			u32 reserved1 : 5;
			u32 fd : 1;
			u32 reserved2 : 12;
			u32 bl : 1;
			u32 rb : 1;
			u32 md : 1;
			u32 reserved3 : 1;
		};
		u32 all;
	} sr;

	union {
		struct {
			u32 rm : 2;
			u32 flag : 5;
			u32 enable : 5;
			u32 cause : 6;
			u32 dn : 1;
			u32 pr : 1;
			u32 sz : 1;
			u32 fr : 1;
		};
		u32 all;
	} fpscr;

	u32 fpul;

	u32 ssr;
	u32 gbr;
	u32 mach;
	u32 macl;
	u32 pr;
	u32 spc;
	u32 sgr;
	u32 dbr;
	u32 vbr;

	u32 pc;

	u32 cycle;
} SH4;

SH4 sh4;

#endif
