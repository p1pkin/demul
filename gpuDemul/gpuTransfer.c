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
#include <string.h>

#include "gpurenderer.h"
#include "gputransfer.h"
#include "gputexture.h"
#include "gpuprim.h"
#include "gpuregs.h"
#include "gpudemul.h"

void (*sendPrimStart)(u32 *mem) = NULL;
void (*sendPrimEnd)(u32 *mem) = NULL;

void sendEndList(u32 *mem) {
	switch (listtype) {
	case 0: IRQ(0x0007); break;
	case 1: IRQ(0x0008); break;
	case 2: IRQ(0x0009); break;
	case 3: IRQ(0x000a); break;
	case 4: IRQ(0x0015); break;
	default:             break;
	}

	completed = 1;
	listtype = -1;
}

void sendUserClip(u32 *mem) {
}

void sendObjectListSet(u32 *mem) {
}

void sendPolygonExt(u32 *mem) {
	polygon.FaceAlpha = *(float*)&mem[0];
	polygon.FaceRed = *(float*)&mem[1];
	polygon.FaceGreen = *(float*)&mem[2];
	polygon.FaceBlue = *(float*)&mem[3];

	polygon.OffsetAlpha = *(float*)&mem[4];
	polygon.OffsetRed = *(float*)&mem[5];
	polygon.OffsetGreen = *(float*)&mem[6];
	polygon.OffsetBlue = *(float*)&mem[7];

	sendPrimEnd = NULL;
}

void sendPolygon(u32 *mem) {
	if ((polygon.pcw.all != mem[0]) ||
		(polygon.isp.all != mem[1]) ||
		(polygon.tsp.all != mem[2]) ||
		(polygon.tcw.all != mem[3]) &&
		(objCount)) {
		flushPrim();
		NewStripList();
	}
	polygon.pcw.all = mem[0];
	polygon.isp.all = mem[1];
	polygon.tsp.all = mem[2];
	polygon.tcw.all = mem[3];

//	DEMUL_printf(">send Poly (objCount=%d, ListType=%d)\n",objCount,polygon.pcw.listtype);

	if (completed) {
		listtype = polygon.pcw.listtype;
		completed = 0;
	}

	if ((polygon.pcw.listtype == 1) || (polygon.pcw.listtype == 3)) {
		sendPrimStart = MOD_VOL;
//		DEMUL_printf(">send Modifier Volume (objCount=%d)\n",objCount);
	} else {
		switch (polygon.pcw.colortype) {
		case 0:
			if (polygon.pcw.texture)
				if (polygon.pcw.volume)
					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_PC_VOL;	// Textured, Packed Color, 16bit UV, with Two Volumes
					else sendPrimStart = TRIANGLE_T32_PC_VOL;					// Textured, Packed Color, with Two Volumes
				else
				if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_PC;	// Textured, Packed Color, 16bit UV
				else sendPrimStart = TRIANGLE_T32_PC;						// Textured, Packed Color
			else
			if (polygon.pcw.volume) sendPrimStart = TRIANGLE_NT_PC_VOL;		// Non-Textured, Packed Color, with Two Volumes
			else sendPrimStart = TRIANGLE_NT_PC;							// Non-Textured, Packed Color
			break;

		case 1:
			if (polygon.pcw.texture)
				if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_FC;	// Textured, Floating Color, 16bit UV
				else sendPrimStart = TRIANGLE_T32_FC;					// Textured, Floating Color
			else
				sendPrimStart = TRIANGLE_NT_FC;	// Non-Textured, Floating Color
			break;

		case 2:
			if (polygon.pcw.texture)
				if (polygon.pcw.volume) {
					sendPrimEnd = sendPolygonExt;

					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_IC_VOL;	// Textured, Intensity, 16bit UV, with Two Volumes
					else sendPrimStart = TRIANGLE_T32_IC_VOL;					// Textured, Intensity, with Two Volumes
				} else if (polygon.pcw.offset) {
					sendPrimEnd = sendPolygonExt;

					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_IC;	// Textured, Intensity, 16bit UV
					else sendPrimStart = TRIANGLE_T32_IC;					// Textured, Intensity
				} else {
					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_IC;	// Textured, Intensity, 16bit UV
					else sendPrimStart = TRIANGLE_T32_IC;					// Textured, Intensity

					polygon.FaceAlpha = *(float*)&mem[4];
					polygon.FaceRed = *(float*)&mem[5];
					polygon.FaceGreen = *(float*)&mem[6];
					polygon.FaceBlue = *(float*)&mem[7];
				}
			else
			if (polygon.pcw.volume) {
				sendPrimEnd = sendPolygonExt;

				sendPrimStart = TRIANGLE_NT_IC_VOL;		// Non-Textured, Intensity, with Two Volumes
			} else if (polygon.pcw.offset) {
				sendPrimEnd = sendPolygonExt;

				sendPrimStart = TRIANGLE_NT_IC;			// Non-Textured, Intensity
			} else {
				sendPrimStart = TRIANGLE_NT_IC;			// Non-Textured, Intensity

				polygon.FaceAlpha = *(float*)&mem[4];
				polygon.FaceRed = *(float*)&mem[5];
				polygon.FaceGreen = *(float*)&mem[6];
				polygon.FaceBlue = *(float*)&mem[7];
			}
			break;

		case 3:
			if (polygon.pcw.texture)
				if (polygon.pcw.volume) {
					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_IC_VOL;	// Textured, Intensity, 16bit UV, with Two Volumes
					else sendPrimStart = TRIANGLE_T32_IC_VOL;					// Textured, Intensity, with Two Volumes
				} else {
					if (polygon.pcw.uvmode) sendPrimStart = TRIANGLE_T16_IC;	// Textured, Intensity, 16bit UV
					else sendPrimStart = TRIANGLE_T32_IC;					// Textured, Intensity
				}
			else
			if (polygon.pcw.volume) sendPrimStart = TRIANGLE_NT_IC_VOL;			// Non-Textured, Intensity, with Two Volumes
			else sendPrimStart = TRIANGLE_NT_IC;								// Non-Textured, Intensity
			break;
		}
	}
}

void sendSprite(u32 *mem) {
	if ((polygon.pcw.all != mem[0]) ||
		(polygon.isp.all != mem[1]) ||
		(polygon.tsp.all != mem[2]) ||
		(polygon.tcw.all != mem[3]) &&
		(objCount)) {
		flushPrim();
		NewStripList();
	}

	polygon.pcw.all = mem[0];
	polygon.isp.all = mem[1];
	polygon.tsp.all = mem[2];
	polygon.tcw.all = mem[3];
//	DEMUL_printf(">send Sprite (objCount=%d, ListType=%d)\n",objCount,polygon.pcw.listtype);

	if (completed) {
		listtype = polygon.pcw.listtype;
		completed = 0;
	}

	polygon.BaseColor = ((mem[4] & 0x00ff0000) >> 16) |
						((mem[4] & 0x000000ff) << 16) |
						((mem[4] & 0xff00ff00) << 0);

	polygon.OffsetColor = ((mem[5] & 0x00ff0000) >> 16) |
						  ((mem[5] & 0x000000ff) << 16) |
						  ((mem[5] & 0xff00ff00) << 0);

	if (polygon.pcw.texture) sendPrimStart = SPRITE_T16;
	else sendPrimStart = SPRITE_NT;
}

void FASTCALL gpuTaTransfer(u32 *mem, u32 size) {
	for (; size != 0; mem += 8, size -= 32) {
		if (sendPrimEnd != NULL) {
			sendPrimEnd(mem);
		} else
			switch ((mem[0] >> 29) & 0x7) {
			case 0: sendEndList(mem); break;			//END_OF_LIST
			case 1: sendUserClip(mem); break;			//USER CLIP
			case 2: sendObjectListSet(mem); break;		//OBJECT_LIST_SET
			case 4: sendPolygon(mem); break;			//POLYGON
			case 5: sendSprite(mem); break;				//SPRITE
			case 7: sendPrimStart(mem); break;			//VERTEX
			default:                        break;
			}
	}
}

void FASTCALL gpuYuvTransfer(u8 *src, u8 *dst) {
	u32 tex_ctrl = REGS32(0x814c);
	u8 yuvu = ((tex_ctrl >> 0) & 0x3f) + 1;
	u8 yuvv = ((tex_ctrl >> 8) & 0x3f) + 1;

	u32 width = yuvu << 5;
	u32 height = width << 4;

	u32 i;

	for (i = 0; i < yuvv; i++, dst += height) {
		u32 i;
		for (i = 0; i < width; i += 32, src += 384) {
			u8  *p = (u8*)&dst[i];
			u32 l;
			for (l = 0; l < 16; l++, p += width) {
				u32 i, j, k = 0;
				u32 ll1 = ((l & 0xFFFFFFFE) << 2);
				u32 ll2 = ((ll1 & 0xFFFFFFE0) << 2) + ((l & 7) << 3);
				for (i = 0, j = 0; j < 8; i += 2, j++) {
					u32 index1 = ll1 + j;
					u32 index2 = ll2 + (((i & 0xFFFFFFF8) << 3) + (i & 7));

					p[k++] = src[index1 + 0x00];
					p[k++] = src[index2 + 0x80];
					p[k++] = src[index1 + 0x40];
					p[k++] = src[index2 + 0x81];
				}
			}
		}
	}
}
