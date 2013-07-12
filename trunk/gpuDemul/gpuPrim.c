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

#include <math.h>
#include "gpuprim.h"
#include "gpurenderer.h"
#include "gpudemul.h"
#include "gpuregs.h"

//#define TEXTURE_UV_DUMP
//#define VERTEX_DUMP
//#define VOLUME_DUMP
//#define SPRITE_DUMP
#define ez 1
#define sz 1000

extern void (*sendPrimEnd)(u32 *mem);

static u32 vVertexCount;

float maxz = -10000000.0f, maxz1 = -10000000.0f;
float minz = 10000000.0f, minz1 = 10000000.0f;

void NewStripList() {
	vCount = 0;
	vVertexCount = 0;
}

void CopleteTriangle(u32 end_of_strip) {
	if (vVertexCount > 2) {
		pIndex[vIndex + vCount++] = vVertex - 2;
		pIndex[vIndex + vCount++] = vVertex - 1;
	} else
		vVertexCount++;
	pIndex[vIndex + vCount++] = vVertex;
	if (end_of_strip & (1 << 28))
		vVertexCount = 0;
}

void ConvertVertex(VERTEX *vtx, u32 *dst) {
	vtx->x = *(float*)&dst[0];
	vtx->y = *(float*)&dst[1];
	vtx->z = (float)(log(1.0f + (1 / (*(float*)&dst[2]))) / (log(2.0f) * 32));
	vtx->w = *(float*)&dst[2];

	if ((vtx->z) > maxz1) maxz1 = vtx->z;
	if ((vtx->z) < minz1) minz1 = vtx->z;

#ifdef VERTEX_DUMP
	DEMUL_printf("x=%f,y=%f,z=%f\n", vtx->x, vtx->y, *(float*)&dst[2]);
#endif
}

void ConvertUV16(VERTEX *vtx, u32 *dst) {
	u32 tu, tv;
	tu = dst[0] & 0xffff0000;
	tv = dst[0] << 16;

	vtx->tu = (*(float*)&tu);
	vtx->tv = (*(float*)&tv);
	vtx->tw = 1;
	vtx->tz = 1;

#ifdef TEXTURE_UV_DUMP
	DEMUL_printf("16: tu=%f,tv=%f [%08X,%08X]\n", vtx->tu, vtx->tv, dst[0], dst[1]);
#endif
}

void ConvertUV32(VERTEX *vtx, u32 *dst) {
	vtx->tu = (*(float*)&dst[0]);
	vtx->tv = (*(float*)&dst[1]);
	vtx->tw = 1;
	vtx->tz = 1;

#ifdef TEXTURE_UV_DUMP
	DEMUL_printf("32: tu=%f,tv=%f [%08X,%08X]\n", vtx->tu, vtx->tv, dst[0], dst[1]);
#endif
}

u32 ConvertColor(u32 dst) {
	return ((dst & 0x00ff0000) >> 16) | ((dst & 0x000000ff) << 16) | ((dst & 0xff00ff00) << 0);
}

u32 ConvertRGBA(u32 *dst) {
	float alpha, red, green, blue;

	alpha = *(float*)&dst[0];
	red = *(float*)&dst[1];
	green = *(float*)&dst[2];
	blue = *(float*)&dst[3];

	alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
	red = red   < 0.0f ? 0.0f : red   > 1.0f ? 1.0f : red;
	green = green < 0.0f ? 0.0f : green > 1.0f ? 1.0f : green;
	blue = blue  < 0.0f ? 0.0f : blue  > 1.0f ? 1.0f : blue;

	return ((u32)(alpha * 255.0f) << 24) |
		   ((u32)(blue * 255.0f) << 16) |
		   ((u32)(green * 255.0f) << 8) |
		   ((u32)(red * 255.0f) << 0);
}

u32 ConvertRGBAFace(u32 dst) {
	float alpha, red, green, blue;

	alpha = *(float*)&dst * polygon.FaceAlpha;
	red = *(float*)&dst * polygon.FaceRed;
	green = *(float*)&dst * polygon.FaceGreen;
	blue = *(float*)&dst * polygon.FaceBlue;

	alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
	red = red   < 0.0f ? 0.0f : red   > 1.0f ? 1.0f : red;
	green = green < 0.0f ? 0.0f : green > 1.0f ? 1.0f : green;
	blue = blue  < 0.0f ? 0.0f : blue  > 1.0f ? 1.0f : blue;

	return ((u32)(alpha * 255.0f) << 24) |
		   ((u32)(blue * 255.0f) << 16) |
		   ((u32)(green * 255.0f) << 8) |
		   ((u32)(red * 255.0f) << 0);
}

u32 ConvertRGBAOffset(u32 dst) {
	float alpha, red, green, blue;

	alpha = *(float*)&dst * polygon.OffsetAlpha;
	red = *(float*)&dst * polygon.OffsetRed;
	green = *(float*)&dst * polygon.OffsetGreen;
	blue = *(float*)&dst * polygon.OffsetBlue;

	alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
	red = red   < 0.0f ? 0.0f : red   > 1.0f ? 1.0f : red;
	green = green < 0.0f ? 0.0f : green > 1.0f ? 1.0f : green;
	blue = blue  < 0.0f ? 0.0f : blue  > 1.0f ? 1.0f : blue;

	return ((u32)(alpha * 255.0f) << 24) |
		   ((u32)(blue * 255.0f) << 16) |
		   ((u32)(green * 255.0f) << 8) |
		   ((u32)(red * 255.0f) << 0);
}

void MOD_VOL(u32 *mem) {
#ifdef VOLUME_DUMP
	VERTEX v;
	v.x = *(float*)&mem[1];
	v.y = *(float*)&mem[2];
	v.z = (float)(log(1.0f + (1 / (*(float*)&mem[3]))) / (log(2.0f) * 32));
	DEMUL_printf(">modifier: x=%f y=%f,z=%f\n", v.x, v.y, v.z);
#endif
	sendPrimEnd = MOD_VOL_EXT;
}

void MOD_VOL_EXT(u32 *mem) {
	sendPrimEnd = NULL;
}

void TRIANGLE_T16_PC_VOL(u32 *mem) {//need fix
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV16(v, &mem[4]);
	v->color = ConvertColor(mem[6]);
	v->extcolor = ConvertColor(mem[7]);
	sendPrimEnd = TRIANGLE_T16_PC_VOL_EXT;
}

void TRIANGLE_T16_PC_VOL_EXT(u32 *mem) {
	sendPrimEnd = NULL;
}

void TRIANGLE_T32_PC_VOL(u32 *mem) {//need fix
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV32(v, &mem[4]);
	v->color = ConvertColor(mem[6]);
	v->extcolor = ConvertColor(mem[7]);
	sendPrimEnd = TRIANGLE_T32_PC_VOL_EXT;
}

void TRIANGLE_T32_PC_VOL_EXT(u32 *mem) {
	sendPrimEnd = NULL;
}

void TRIANGLE_T16_PC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV16(v, &mem[4]);
	v->color = ConvertColor(mem[6]);
	v->extcolor = ConvertColor(mem[7]);
}

void TRIANGLE_T32_PC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV32(v, &mem[4]);
	v->color = ConvertColor(mem[6]);
	v->extcolor = ConvertColor(mem[7]);
}

void TRIANGLE_NT_PC_VOL(u32 *mem) {	//need fix
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	v->color = ConvertColor(mem[4]);
	v->extcolor = ConvertColor(mem[5]);
}

void TRIANGLE_NT_PC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	v->color = ConvertColor(mem[6]);
	v->extcolor = 0;
}

void TRIANGLE_T16_FC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex];
	ConvertVertex(v, &mem[1]);
	ConvertUV16(v, &mem[4]);
	sendPrimEnd = TRIANGLE_T16_FC_EXT;
}

void TRIANGLE_T16_FC_EXT(u32 *mem) {
	VERTEX *v = &pVertex[vVertex++];
	v->color = ConvertRGBA(&mem[0]);
	v->extcolor = ConvertRGBA(&mem[4]);
	sendPrimEnd = NULL;
}

void TRIANGLE_T32_FC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex];
	ConvertVertex(v, &mem[1]);
	ConvertUV32(v, &mem[4]);
	sendPrimEnd = TRIANGLE_T32_FC_EXT;
}

void TRIANGLE_T32_FC_EXT(u32 *mem) {
	VERTEX *v = &pVertex[vVertex++];
	v->color = ConvertRGBA(&mem[0]);
	v->extcolor = ConvertRGBA(&mem[4]);
	sendPrimEnd = NULL;
}

void TRIANGLE_NT_FC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	v->color = ConvertRGBA(&mem[4]);
	v->extcolor = 0;
}

void TRIANGLE_T16_IC_VOL(u32 *mem) {//need fix
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV16(v, &mem[4]);
	v->color = ConvertRGBAFace(mem[6]);
	v->extcolor = ConvertRGBAOffset(mem[7]);
	sendPrimEnd = TRIANGLE_T16_IC_VOL_EXT;
}

void TRIANGLE_T16_IC_VOL_EXT(u32 *mem) {
	sendPrimEnd = NULL;
}

void TRIANGLE_T32_IC_VOL(u32 *mem) {//need fix
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV32(v, &mem[4]);
	v->color = ConvertRGBAFace(mem[6]);
	v->extcolor = ConvertRGBAOffset(mem[7]);
	sendPrimEnd = TRIANGLE_T32_IC_VOL_EXT;
}

void TRIANGLE_T32_IC_VOL_EXT(u32 *mem) {
	sendPrimEnd = NULL;
}

void TRIANGLE_T16_IC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV16(v, &mem[4]);
	v->color = ConvertRGBAFace(mem[6]);
	v->extcolor = ConvertRGBAOffset(mem[7]);
}

void TRIANGLE_T32_IC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	ConvertUV32(v, &mem[4]);
	v->color = ConvertRGBAFace(mem[6]);
	v->extcolor = ConvertRGBAOffset(mem[7]);
}

void TRIANGLE_NT_IC_VOL(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	v->color = ConvertRGBAFace(mem[4]);
	v->extcolor = ConvertRGBAOffset(mem[7]);
}

void TRIANGLE_NT_IC(u32 *mem) {
	VERTEX *v;
	CopleteTriangle(mem[0]);
	v = &pVertex[vVertex++];
	ConvertVertex(v, &mem[1]);
	v->color = ConvertRGBAFace(mem[6]);
	v->extcolor = 0;
}

void SPRITE_T16(u32 *mem) {
	ConvertVertex(&pSprite[0], &mem[1]);
	ConvertVertex(&pSprite[1], &mem[4]);
	pSprite[3].x = *(float*)&mem[7];
	sendPrimEnd = SPRITE_T16_EXT;
}

void SPRITE_T16_EXT(u32 *mem) {
	u32 tu, tv;
//    float delta,a,b,c;
	pSprite[3].y = *(float*)&mem[0];
	pSprite[3].z = (float)(log(1.0f + 1 / (*(float*)&mem[1])) / (log(2.0f) * 32));
	if (pSprite[3].z > maxz1) maxz1 = pSprite[3].z;
	if (pSprite[3].z < minz1) minz1 = pSprite[3].z;

	pSprite[2].x = *(float*)&mem[2];
	pSprite[2].y = *(float*)&mem[3];
	pSprite[2].z = pSprite[3].z;

//	delta=pSprite[0].x*(pSprite[1].y-pSprite[3].y)+
//		  pSprite[1].x*(pSprite[0].y-pSprite[3].y)+
//		  pSprite[3].x*(pSprite[0].y-pSprite[1].y);

//	a=pSprite[0].z*(pSprite[1].y-pSprite[3].y+pSprite[3].x-pSprite[1].x+pSprite[1].x*pSprite[3].y-pSprite[3].x*pSprite[1].y)/delta;
//	b=pSprite[1].z*(pSprite[3].y-pSprite[0].y+pSprite[0].x-pSprite[3].x+pSprite[3].x*pSprite[0].y-pSprite[0].x*pSprite[3].y)/delta;
//	c=pSprite[3].z*(pSprite[0].y-pSprite[1].y+pSprite[1].x-pSprite[0].x+pSprite[0].x*pSprite[1].y-pSprite[1].x*pSprite[0].y)/delta;

//	pSprite[2].z = a*pSprite[2].x+b*pSprite[2].y+c;

	tu = mem[5] & 0xffff0000;
	tv = mem[5] << 16;

	pSprite[0].tu = *(float*)&tu;
	pSprite[2].tu = *(float*)&tu;
	pSprite[0].tv = *(float*)&tv;

	tu = mem[6] & 0xffff0000;
	tv = mem[6] << 16;

	pSprite[1].tu = *(float*)&tu;
	pSprite[1].tv = *(float*)&tv;

	tu = mem[7] & 0xffff0000;
	tv = mem[7] << 16;

	pSprite[3].tu = *(float*)&tu;
	pSprite[3].tv = *(float*)&tv;
	pSprite[2].tv = *(float*)&tv;

	pSprite[0].color = polygon.BaseColor;
	pSprite[1].color = polygon.BaseColor;
	pSprite[2].color = polygon.BaseColor;
	pSprite[3].color = polygon.BaseColor;

	pSprite[0].extcolor = polygon.OffsetColor;
	pSprite[1].extcolor = polygon.OffsetColor;
	pSprite[2].extcolor = polygon.OffsetColor;
	pSprite[3].extcolor = polygon.OffsetColor;

#ifdef SPRITE_DUMP
	DEMUL_printf("AX=%f,AY=%f,AZ=%f\n", pSprite[0].x, pSprite[0].y, pSprite[0].z);
	DEMUL_printf("BX=%f,BY=%f,BZ=%f\n", pSprite[1].x, pSprite[1].y, pSprite[1].z);
	DEMUL_printf("CX=%f,CY=%f,CZ=%f\n", pSprite[3].x, pSprite[3].y, pSprite[3].z);
	DEMUL_printf("DX=%f,DY=%f,DZ=%f\n", pSprite[2].x, pSprite[2].y, pSprite[2].z);
#endif

	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[0];
	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[1];
	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[2];
	CopleteTriangle(-1);
	pVertex[vVertex++] = pSprite[3];

	sendPrimEnd = NULL;
}

void SPRITE_NT(u32 *mem) {
	ConvertVertex(&pSprite[0], &mem[1]);
	ConvertVertex(&pSprite[1], &mem[4]);
	pSprite[3].x = *(float*)&mem[7];
	sendPrimEnd = SPRITE_NT_EXT;
}

void SPRITE_NT_EXT(u32 *mem) {
//    float delta,a,b,c;
	pSprite[3].y = *(float*)&mem[0];
	pSprite[3].z = (float)(log(1.0f + 1 / (*(float*)&mem[1])) / (log(2.0f) * 32));
	if (pSprite[3].z > maxz1) maxz1 = pSprite[3].z;
	if (pSprite[3].z < minz1) minz1 = pSprite[3].z;

	pSprite[2].x = *(float*)&mem[2];
	pSprite[2].y = *(float*)&mem[3];
	pSprite[2].z = pSprite[3].z;

//	delta=pSprite[0].x*(pSprite[1].y-pSprite[3].y)+
//		  pSprite[1].x*(pSprite[0].y-pSprite[3].y)+
//		  pSprite[3].x*(pSprite[0].y-pSprite[1].y);

//	a=pSprite[0].z*(pSprite[1].y-pSprite[3].y+pSprite[3].x-pSprite[1].x+pSprite[1].x*pSprite[3].y-pSprite[3].x*pSprite[1].y)/delta;
//	b=pSprite[1].z*(pSprite[3].y-pSprite[0].y+pSprite[0].x-pSprite[3].x+pSprite[3].x*pSprite[0].y-pSprite[0].x*pSprite[3].y)/delta;
//	c=pSprite[3].z*(pSprite[0].y-pSprite[1].y+pSprite[1].x-pSprite[0].x+pSprite[0].x*pSprite[1].y-pSprite[1].x*pSprite[0].y)/delta;

//	pSprite[2].z = a*pSprite[2].x+b*pSprite[2].y+c;

	pSprite[0].color = polygon.BaseColor;
	pSprite[1].color = polygon.BaseColor;
	pSprite[2].color = polygon.BaseColor;
	pSprite[3].color = polygon.BaseColor;

	pSprite[0].extcolor = polygon.OffsetColor;
	pSprite[1].extcolor = polygon.OffsetColor;
	pSprite[2].extcolor = polygon.OffsetColor;
	pSprite[3].extcolor = polygon.OffsetColor;

#ifdef SPRITE_DUMP
	DEMUL_printf("AX=%f,AY=%f,AZ=%f\n", pSprite[0].x, pSprite[0].y, pSprite[0].z);
	DEMUL_printf("BX=%f,BY=%f,BZ=%f\n", pSprite[1].x, pSprite[1].y, pSprite[1].z);
	DEMUL_printf("CX=%f,CY=%f,CZ=%f\n", pSprite[3].x, pSprite[3].y, pSprite[3].z);
	DEMUL_printf("DX=%f,DY=%f,DZ=%f\n", pSprite[2].x, pSprite[2].y, pSprite[2].z);
#endif

	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[0];
	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[1];
	CopleteTriangle(0);
	pVertex[vVertex++] = pSprite[2];
	CopleteTriangle(-1);
	pVertex[vVertex++] = pSprite[3];

	sendPrimEnd = NULL;
}

void BACKGROUND(u32 *mem) {
	u32 index = 0;
	polygon.pcw.all = 0;
	polygon.isp.all = mem[index + 0];
	polygon.tsp.all = mem[index + 1];
	polygon.tcw.all = mem[index + 2];

	ConvertVertex(&pBackground[0], &mem[index + 3]);
	pBackground[0].color = ConvertColor(mem[index + 6]);
	ConvertVertex(&pBackground[1], &mem[index + 7]);
	pBackground[1].color = ConvertColor(mem[index + 10]);
	ConvertVertex(&pBackground[2], &mem[index + 11]);
	pBackground[2].color = ConvertColor(mem[index + 14]);

	pBackground[3].x = pBackground[1].x;
	pBackground[3].y = pBackground[2].y;
	pBackground[3].z = pBackground[2].z;
	pBackground[3].color = pBackground[2].color;

	pBackground[0].tu = 0.0;
	pBackground[0].tv = 0.0;

	pBackground[1].tu = 1.0;
	pBackground[1].tv = 0.0;

	pBackground[2].tu = 0.0;
	pBackground[2].tv = 1.0;

	pBackground[3].tu = 1.0;
	pBackground[3].tv = 1.0;
/*
    DEMUL_printf("polygon.isp.reserved = %x\n",polygon.isp.reserved);
    DEMUL_printf("polygon.isp.dcalcctrl = %d\n",polygon.isp.dcalcctrl);
    DEMUL_printf("polygon.isp.cachebypass = %d\n",polygon.isp.cachebypass);
    DEMUL_printf("polygon.isp.uvmode = %d\n",polygon.isp.uvmode);
    DEMUL_printf("polygon.isp.gouraud = %d\n",polygon.isp.gouraud);
    DEMUL_printf("polygon.isp.offset = %d\n",polygon.isp.offset);
    DEMUL_printf("polygon.isp.texture = %d\n",polygon.isp.texture);
    DEMUL_printf("polygon.isp.zwritedis = %d\n",polygon.isp.zwritedis);
    DEMUL_printf("polygon.isp.cullmode = %d\n",polygon.isp.cullmode);
    DEMUL_printf("polygon.isp.depthmode = %d\n",polygon.isp.depthmode);
*/

	polygon.pcw.uvmode = polygon.isp.uvmode;
	polygon.pcw.gouraud = polygon.isp.gouraud;
	polygon.pcw.offset = polygon.isp.offset;
	polygon.pcw.texture = polygon.isp.texture;

/*
    DEMUL_printf("polygon.tsp.texv = %d\n",polygon.tsp.texv);
    DEMUL_printf("polygon.tsp.texu = %d\n",polygon.tsp.texu);
    DEMUL_printf("polygon.tsp.shadinstr = %d\n",polygon.tsp.shadinstr);
    DEMUL_printf("polygon.tsp.mipmapd = %d\n",polygon.tsp.mipmapd);
    DEMUL_printf("polygon.tsp.supsample = %d\n",polygon.tsp.supsample);
    DEMUL_printf("polygon.tsp.filtermode = %d\n",polygon.tsp.filtermode);
    DEMUL_printf("polygon.tsp.clampuv = %d\n",polygon.tsp.clampuv);
    DEMUL_printf("polygon.tsp.flipuv = %d\n",polygon.tsp.flipuv);
    DEMUL_printf("polygon.tsp.ignoretexa = %d\n",polygon.tsp.ignoretexa);
    DEMUL_printf("polygon.tsp.usealpha = %d\n",polygon.tsp.usealpha);
    DEMUL_printf("polygon.tsp.colorclamp = %d\n",polygon.tsp.colorclamp);
    DEMUL_printf("polygon.tsp.fogctrl = %d\n",polygon.tsp.fogctrl);
    DEMUL_printf("polygon.tsp.dstselect = %d\n",polygon.tsp.dstselect);
    DEMUL_printf("polygon.tsp.srcselect = %d\n",polygon.tsp.srcselect);
    DEMUL_printf("polygon.tsp.dstinstr = %d\n",polygon.tsp.srcselect);
    DEMUL_printf("polygon.tsp.srcinstr = %d\n",polygon.tsp.srcinstr);

    DEMUL_printf("polygon.tcw.texaddr = %x\n",polygon.tcw.texaddr);
    DEMUL_printf("polygon.tcw.special = %x\n",polygon.tcw.special);
    DEMUL_printf("polygon.tcw.pixelfmt = %d\n",polygon.tcw.pixelfmt);
    DEMUL_printf("polygon.tcw.vqcomp = %d\n",polygon.tcw.vqcomp);
    DEMUL_printf("polygon.tcw.mipmapped = %d\n",polygon.tcw.mipmapped);
*/
//	DEMUL_printf("AX=%f,AY=%f,AZ=%f,C=%08x\n",pBackground[0].x,pBackground[0].y,pBackground[0].z,pBackground[0].color);
//	DEMUL_printf("AX=%f,AY=%f,AZ=%f,C=%08x\n",pBackground[1].x,pBackground[1].y,pBackground[1].z,pBackground[1].color);
//	DEMUL_printf("AX=%f,AY=%f,AZ=%f,C=%08x\n",pBackground[2].x,pBackground[2].y,pBackground[2].z,pBackground[2].color);

	CopleteTriangle(0);
	pVertex[vVertex++] = pBackground[0];
	CopleteTriangle(0);
	pVertex[vVertex++] = pBackground[1];
	CopleteTriangle(0);
	pVertex[vVertex++] = pBackground[2];
	CopleteTriangle(-1);
	pVertex[vVertex++] = pBackground[3];
}