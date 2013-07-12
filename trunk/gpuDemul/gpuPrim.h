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

#ifndef __PRIM_H__
#define __PRIM_H__

#include "types.h"

typedef union {
	struct {
		u32 uvmode     : 1;
		u32 gouraud    : 1;
		u32 offset     : 1;
		u32 texture    : 1;
		u32 colortype  : 2;
		u32 volume     : 1;
		u32 shadow     : 1;
		u32 reserved0  : 8;

		u32 userclip   : 2;
		u32 striplen   : 2;
		u32 reserved1  : 3;
		u32 groupen    : 1;

		u32 listtype   : 3;
		u32 reserved2  : 1;
		u32 endofstrip : 1;
		u32 paratype   : 3;
	};

	u32 all;
} PCW;

typedef union {
	struct {
		u32 reserved    : 20;
		u32 dcalcctrl   : 1;
		u32 cachebypass : 1;
		u32 uvmode      : 1;
		u32 gouraud     : 1;
		u32 offset      : 1;
		u32 texture     : 1;
		u32 zwritedis   : 1;
		u32 cullmode    : 2;
		u32 depthmode   : 3;
	};

	u32 all;
} ISP;

typedef union {
	struct {
		u32 texv        : 3;
		u32 texu        : 3;
		u32 shadinstr   : 2;
		u32 mipmapd     : 4;
		u32 supsample   : 1;
		u32 filtermode  : 2;
		u32 clampuv     : 2;
		u32 flipuv      : 2;
		u32 ignoretexa  : 1;
		u32 usealpha    : 1;
		u32 colorclamp  : 1;
		u32 fogctrl     : 2;
		u32 dstselect   : 1;
		u32 srcselect   : 1;
		u32 dstinstr    : 3;
		u32 srcinstr    : 3;
	};

	u32 all;
} TSP;

typedef union {
	struct {
		u32 texaddr   : 21;
		u32 special   : 6;
		u32 pixelfmt  : 3;
		u32 vqcomp    : 1;
		u32 mipmapped : 1;
	};

	u32 all;
} TCW;

typedef struct {
	PCW pcw;
	ISP isp;
	TSP tsp;
	TCW tcw;

	u32 BaseColor;
	u32 OffsetColor;

	float FaceAlpha;
	float FaceRed;
	float FaceGreen;
	float FaceBlue;

	float OffsetAlpha;
	float OffsetRed;
	float OffsetGreen;
	float OffsetBlue;
} POLYGON;

POLYGON polygon;

typedef struct {
	u32 vIndex;
	u32 vCount;
	float midZ;
	POLYGON polygon;
} OBJECT;

typedef struct {
	float x, y, z, w;
	float tu, tv, tw, tz;
	u32 color;
	u32 extcolor;
} VERTEX;

VERTEX pVertex[0x00040000];
u32 pIndex[0x00080000];
OBJECT objList[0x00008000];
VERTEX pSprite[4];
VERTEX pBackground[4];
u32 vCount;
u32 vIndex;
u32 vVertex;
u32 objCount;

extern float maxz, maxz1;
extern float minz, minz1;

void NewStripList();

void MOD_VOL(u32 *mem);				// Modifier Volume
void MOD_VOL_EXT(u32 *mem);

void TRIANGLE_T16_PC_VOL(u32 *mem);	// Textured, Packed Color, 16bit UV, with Two Volumes
void TRIANGLE_T16_PC_VOL_EXT(u32 *mem);
void TRIANGLE_T32_PC_VOL(u32 *mem);	// Textured, Packed Color, with Two Volumes
void TRIANGLE_T32_PC_VOL_EXT(u32 *mem);
void TRIANGLE_T16_PC(u32 *mem);		// Textured, Packed Color, 16bit UV
void TRIANGLE_T32_PC(u32 *mem);		// Textured, Packed Color
void TRIANGLE_NT_PC_VOL(u32 *mem);	// Non-Textured, Packed Color, with Two Volumes
void TRIANGLE_NT_PC(u32 *mem);		// Non-Textured, Packed Color
void TRIANGLE_T16_FC(u32 *mem);		// Textured, Floating Color, 16bit UV
void TRIANGLE_T16_FC_EXT(u32 *mem);
void TRIANGLE_T32_FC(u32 *mem);		// Textured, Floating Color
void TRIANGLE_T32_FC_EXT(u32 *mem);
void TRIANGLE_NT_FC(u32 *mem);		// Non-Textured, Floating Color
void TRIANGLE_T16_IC_VOL(u32 *mem);	// Textured, Intensity, 16bit UV, with Two Volumes
void TRIANGLE_T16_IC_VOL_EXT(u32 *mem);
void TRIANGLE_T32_IC_VOL(u32 *mem);	// Textured, Intensity, with Two Volumes
void TRIANGLE_T32_IC_VOL_EXT(u32 *mem);
void TRIANGLE_T16_IC(u32 *mem);		// Textured, Intensity, 16bit UV
void TRIANGLE_T32_IC(u32 *mem);		// Textured, Intensity
void TRIANGLE_NT_IC_VOL(u32 *mem);	// Non-Textured, Intensity, with Two Volumes
void TRIANGLE_NT_IC(u32 *mem);		// Non-Textured, Intensity

void SPRITE_T16(u32 *mem);			// Textured, sprite
void SPRITE_T16_EXT(u32 *mem);

void SPRITE_NT(u32 *mem);			// Non-Textured, sprite
void SPRITE_NT_EXT(u32 *mem);

void BACKGROUND(u32 *mem);

#endif