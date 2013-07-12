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

#include <windows.h>
#include <gl\gl.h>
#include <gl\glext.h>
#include <stdio.h>
#include <string.h>
#include "gpudemul.h"
#include "gpuprim.h"
#include "gputexture.h"
#include "gpuregs.h"
#include "config.h"
#include "crc32.h"

#define dump_t

#ifdef dump_t
void dump_texture(char *name, void *addr, u32 size) {
	char buf[256];

	static int number = 0;

	FILE *f;

	sprintf(buf, "%5d", number);
	strcpy(buf + strlen(buf), name);

	number++;

	if ((f = fopen(buf, "wb")) != NULL) {
		fwrite(addr, size, 1, f);
		fclose(f);
	}
}
#endif

u32 uppow2(u32 n) {
	u32 x;
	for (x = 31; x >= 0; x--)
		if (n & (1 << x))
			if ((1 << x) != n) return(x + 1);
			else break;
	return x;
}

int ClearTextures() {
	u32 i;
	for (i = 0; i < 0x100000; i++) {
		TxtrCache *txtr = TxtrCacheData[i];
		while (txtr != NULL) {
			if (txtr->next != NULL) {
				txtr = txtr->next;
				free(txtr->prev);
			} else {
				free(txtr);
				txtr = NULL;
				TxtrCacheData[i] = NULL;
			}
		}
	}
	return 1;
}

u32 textureSize;
u32 calculateFullCRC() {
	u32 texaddr = polygon.tcw.texaddr << 3;
	u32 powu = polygon.tsp.texu + 3;
	u32 checksum = 0;
	textureSize = 8 << (polygon.tsp.texu + polygon.tsp.texv + 3);
	switch (polygon.tcw.pixelfmt) {
	case 0:					// 16bpp RGB 1555
	case 1:					// 16bpp RGB 0565
	case 2:					// 16bpp RGB 4444
	case 3:					// 16bpp YUV
	{
		textureSize <<= 1;
		if (polygon.tcw.vqcomp) {
			texaddr += 2048;
			if (polygon.tcw.mipmapped) texaddr += mipmapcodeoffsvq[powu];
		} else
		if (polygon.tcw.mipmapped) texaddr += mipmapcodeoffs[powu];
		break;
	}
	case 4: break;			// 16bpp BUMP
	case 5:					// 4bpp
	{
		textureSize >>= 1;
		if (polygon.tcw.mipmapped) texaddr += mipmapcodeoffsclut[powu];
		checksum = CalcChecksum32cont(0, (u8*)&REGS[0x9000 + ((polygon.tcw.special) << 6)], 0x40);
		break;
	}
	case 6:					// 8bpp
	{
		if (polygon.tcw.mipmapped) texaddr += mipmapcodeoffs[powu];
		checksum = CalcChecksum32cont(0, (u8*)&REGS[0x9000 + ((polygon.tcw.special) << 6)], 0x400);
		break;
	}
	}
	return CalcChecksum32cont(checksum, (u8*)&VRAM[texaddr], textureSize);	//(textureSize < 131071)?textureSize:(textureSize>>2));
}

extern u32 isFramebufferRendered;
extern u32 TextureAddress;
extern int backbufname;
u32 backbufCRC;
u32 textureCRC;
int GetTexture() {
	u32 cacheindex = 0;
	u32 texaddr = polygon.tcw.texaddr;
	u32 texaddrfull = ((texaddr << 3) & 0x00ffffff);
	TxtrCache *txtr = TxtrCacheData[texaddr];
	textureCRC = calculateFullCRC();

	if (texaddrfull == TextureAddress) {
		if (isFramebufferRendered) {
			backbufCRC = textureCRC;
			isFramebufferRendered = 0;
		}

		if (backbufCRC == textureCRC)
			return backbufname;
	}

	if (txtr != NULL) {
		u32 width = 8 << polygon.tsp.texu;
		u32 height = 8 << polygon.tsp.texv;

		while (txtr != NULL) {
			if ((txtr->textureCRC == textureCRC)
				&& (txtr->tcw.all == polygon.tcw.all)
				)
				return txtr->name;
			else {
				if (txtr->next != NULL) {
					txtr = txtr->next;
					cacheindex++;
				} else {
					if (cacheindex == MAX_TEXTURE_CACHE_INDEX) {
						TxtrCache *tmp = txtr->prev;
						tmp->next = NULL;
						glDeleteTextures(1, &txtr->name);
						free(txtr);
						txtr = NULL;
						if (cacheindex == 0) TxtrCacheData[texaddr] = NULL;
					}
					return 0;
				}
			}
		}
	}
	return 0;
}


int CreateTexture() {
	int name;
	u32 texaddr;
	TxtrCache *txtr;

	if (polygon.tcw.pixelfmt == 4) return 0;
	if (name = GetTexture()) return name;

	txtr = (TxtrCache*)malloc(sizeof(TxtrCache));
	texaddr = polygon.tcw.texaddr;
	if (TxtrCacheData[texaddr] == NULL) {
		TxtrCacheData[texaddr] = txtr;
		txtr->next = NULL;
		txtr->prev = txtr;
	} else {
		TxtrCache *tmp = TxtrCacheData[texaddr];
		tmp->prev = txtr;
		txtr->next = tmp;
		TxtrCacheData[texaddr] = txtr;
	}

	txtr->width = 8 << polygon.tsp.texu;
	txtr->height = 8 << polygon.tsp.texv;
	txtr->texaddr = polygon.tcw.texaddr << 3;

//	DEMUL_printf(">create texture: %08x, size = %08x\n",txtr->texaddr,textureSize);

	txtr->tcw.all = polygon.tcw.all;
	txtr->textureCRC = textureCRC;

	glGenTextures(1, &txtr->name);
	glBindTexture(GL_TEXTURE_2D, txtr->name);

	switch (polygon.tcw.pixelfmt) {
	case 0:
		txtr->size = textureSize;
//		DEMUL_printf("ARGB1555 %s %s %s\n",(txtr->tcw.special & 0x20)?"Non Twiddle":"Twiddle", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture");
		CreateTextureARGB1555(txtr);
		break;

	case 1:
		txtr->size = textureSize;
//		DEMUL_printf("ARGB0565 %s %s %s\n",(txtr->tcw.special & 0x20)?"Non Twiddle":"Twiddle", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture");
		CreateTextureARGB0565(txtr);
		break;

	case 2:
		txtr->size = textureSize;
//		DEMUL_printf("ARGB4444 %s %s %s\n",(txtr->tcw.special & 0x20)?"Non Twiddle":"Twiddle", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture");
		CreateTextureARGB4444(txtr);
		break;

	case 3:
		txtr->size = textureSize << 1;
//		DEMUL_printf("YUV422 %s %s %s %s(%d)(%dx%d)\n",(txtr->tcw.special & 0x20)?"Non Twiddle":"Twiddle", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture",(txtr->tcw.special & 0x10)?"Stride":"Non Stride",(REGS32(0x80e4)&0x1F) * 32,txtr->width,txtr->height);
		CreateTextureYUV422(txtr);
		break;

	case 5:
		if ((REGS8(0x8108) & 3) == 3)
			txtr->size = textureSize << 3;
		else
			txtr->size = textureSize << 2;
//		DEMUL_printf("CLUT4 %s %s\n", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture");
		CreateTextureCLUT4(txtr);
		break;

	case 6:
		if ((REGS8(0x8108) & 3) == 3)
			txtr->size = textureSize << 2;
		else
			txtr->size = textureSize << 1;
//		DEMUL_printf("CLUT8 %s %s\n", txtr->tcw.mipmapped?"Mipmapped":"No Mipmapped", (txtr->tcw.vqcomp)?"VQ texture":"Unpacked texture");
		CreateTextureCLUT8(txtr);
		break;

	default:
		return 0;
	}

	TxtrCacheData[texaddr] = txtr;
	return txtr->name;
}

void CreateTextureARGB1555(TxtrCache *texture) {
	u32 i, j;
	void *texdata = malloc(texture->size);

	if (texture->tcw.vqcomp) {
		u32 tw = texture->width;
		u32 th = texture->height;

		u8  *code = (u8*)&VRAM[texture->texaddr] + 2048;
		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < (th >> 1); i++, dst += tw) {
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 index = (*code++) << 2;

					dst[0] = src[index + 0];
					dst[1] = src[index + 2];
					dst[tw + 0] = src[index + 1];
					dst[tw + 1] = src[index + 3];
				}
			}
		} else {
			u32 min = th < tw ? th > > 1 : tw >> 1;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				code += mipmapcodeoffsvq[uppow2(tw)];

			for (i = 0; i < (th >> 1); i++, dst += tw) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					u32 index = (code[twiddley | (TWIDDLE(j & mask) << 1) + chunknum]) << 2;

					dst[0] = src[index + 0];
					dst[1] = src[index + 2];
					dst[tw + 0] = src[index + 1];
					dst[tw + 1] = src[index + 3];
				}
			}
		}
	} else {
		u32 tw = texture->width;
		u32 th = texture->height;

		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < th; i++)
				for (j = 0; j < tw; j += 4) {
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
				}
		} else {
			u32 min = th < tw ? th : tw;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				src += mipmapcodeoffs[powmin];

			for (i = 0; i < th; i++) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < tw; j++) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					*dst++ = src[twiddley | (TWIDDLE(j & mask) << 1) + chunknum];
				}
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_BGRA,
		GL_UNSIGNED_SHORT_1_5_5_5_REV,
		texdata
		);

	free(texdata);
}

void CreateTextureARGB0565(TxtrCache *texture) {
	u32 i, j;
	void *texdata = malloc(texture->size);

	if (texture->tcw.vqcomp) {
		u32 tw = texture->width;
		u32 th = texture->height;

		u8  *code = (u8*)&VRAM[texture->texaddr] + 2048;
		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < (th >> 1); i++, dst += tw) {
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 index = *code++;

					dst[0] = src[index * 4 + 0];
					dst[1] = src[index * 4 + 2];
					dst[tw + 0] = src[index * 4 + 1];
					dst[tw + 1] = src[index * 4 + 3];
				}
			}
		} else {
			u32 min = th < tw ? th > > 1 : tw >> 1;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				code += mipmapcodeoffsvq[uppow2(tw)];

			for (i = 0; i < (th >> 1); i++, dst += tw) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					u32 index = (code[twiddley | (TWIDDLE(j & mask) << 1) + chunknum]) << 2;

					dst[0] = src[index + 0];
					dst[1] = src[index + 2];
					dst[tw + 0] = src[index + 1];
					dst[tw + 1] = src[index + 3];
				}
			}
		}
	} else {
		u32 tw = texture->width;
		u32 th = texture->height;

		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < th; i++)
				for (j = 0; j < tw; j += 4) {
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
				}
		} else {
			u32 min = th < tw ? th : tw;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				src += mipmapcodeoffs[powmin];

			for (i = 0; i < th; i++) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < tw; j++) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					*dst++ = src[twiddley | (TWIDDLE(j & mask) << 1) + chunknum];
				}
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5,
		texdata
		);

	free(texdata);
}

void CreateTextureARGB4444(TxtrCache *texture) {
	u32 i, j;
	void *texdata = malloc(texture->size);

	if (texture->tcw.vqcomp) {
		u32 tw = texture->width;
		u32 th = texture->height;

		u8  *code = (u8*)&VRAM[texture->texaddr] + 2048;
		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < (th >> 1); i++, dst += tw) {
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 index = (*code++) << 2;

					dst[0] = src[index + 0];
					dst[1] = src[index + 2];
					dst[tw + 0] = src[index + 1];
					dst[tw + 1] = src[index + 3];
				}
			}
		} else {
			u32 min = th < tw ? th > > 1 : tw >> 1;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				code += mipmapcodeoffsvq[uppow2(tw)];

			for (i = 0; i < (th >> 1); i++, dst += tw) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < (tw >> 1); j++, dst += 2) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					u32 index = (code[twiddley | (TWIDDLE(j & mask) << 1) + chunknum]) << 2;

					dst[0] = src[index + 0];
					dst[1] = src[index + 2];
					dst[tw + 0] = src[index + 1];
					dst[tw + 1] = src[index + 3];
				}
			}
		}
	} else {
		u32 tw = texture->width;
		u32 th = texture->height;

		u16 *src = (u16*)&VRAM[texture->texaddr];
		u16 *dst = (u16*)texdata;

		if (texture->tcw.special & 0x20) {
			for (i = 0; i < th; i++)
				for (j = 0; j < tw; j++)
					*dst++ = *src++;
		} else {
			u32 min = th < tw ? th : tw;
			u32 mask = min - 1;
			u32 notmask = ~mask;
			u32 powmin = uppow2(min);
			u32 powtw = uppow2(tw) - powmin;

			if (texture->tcw.mipmapped)
				src += mipmapcodeoffs[powmin];

			for (i = 0; i < th; i++) {
				u32 twiddley = TWIDDLE(i & mask);
				u32 chunky = (i & notmask) << powtw;
				for (j = 0; j < tw; j++) {
					u32 chunknum = (chunky + (j & notmask)) << powmin;
					*dst++ = src[twiddley | (TWIDDLE(j & mask) << 1) + chunknum];
				}
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_BGRA,
		GL_UNSIGNED_SHORT_4_4_4_4_REV,
		texdata
		);

	free(texdata);
}

void CreateTextureYUV422(TxtrCache *texture) {
	u32 i, j;

	void *texdata = malloc(texture->size);

	u32 *dst = (u32*)texdata;
	u32 tw, th;

	tw = texture->width;
	th = texture->height;

	if (texture->tcw.special & 0x20) {
		u8  *src = (u8*)&VRAM[texture->texaddr];

		if ((polygon.tcw.special & 0x30) == 0x30)
			tw = (REGS32(0x80e4) & 0x1F) * 32;

		for (i = 0; i < th; dst += texture->width, i++) {
			for (j = 0; j < tw; j += 2) {
				s32 u = *(src++);
				s32 y0 = *(src++);
				s32 v = *(src++);
				s32 y1 = *(src++);
				dst[j + 0] = (yuv2rs(y0, u, v) << 0) |
							 (yuv2gs(y0, u, v) << 8) |
							 (yuv2bs(y0, u, v) << 16) |
							 (0xff000000);
				dst[j + 1] = (yuv2rs(y1, u, v) << 0) |
							 (yuv2gs(y1, u, v) << 8) |
							 (yuv2bs(y1, u, v) << 16) |
							 (0xff000000);
			}
		}
	} else {
		u16 *src = (u16*)&VRAM[texture->texaddr];
		u32 min = th < tw ? th : tw;
		u32 mask = min - 1;
		u32 notmask = ~mask;
		u32 powmin = uppow2(min);
		u32 powtw = uppow2(tw) - powmin;

		if (texture->tcw.mipmapped)
			src += mipmapcodeoffs[powmin];

		for (i = 0; i < th; i++) {
			u32 twiddley = TWIDDLE(i & mask);
			u32 chunky = (i & notmask) << powtw;
			for (j = 0; j < tw; j += 2) {
				u32 chunknum = (chunky + (j & notmask)) << powmin;
				u32 index = twiddley | (TWIDDLE(j & mask) << 1) + chunknum;

				s32 v, u = *(((u8*)&src[index]) + 0);
				s32 y1, y0 = *(((u8*)&src[index]) + 1);

				chunknum = (chunky + ((j + 1) & notmask)) << powmin;
				index = twiddley | (TWIDDLE((j + 1) & mask) << 1) + chunknum;

				v = *(((u8*)&src[index]) + 0);
				y1 = *(((u8*)&src[index]) + 1);

				*dst++ = (yuv2rs(y0, u, v) << 0) |
						 (yuv2gs(y0, u, v) << 8) |
						 (yuv2bs(y0, u, v) << 16) |
						 (0xff000000);
				*dst++ = (yuv2rs(y1, u, v) << 0) |
						 (yuv2gs(y1, u, v) << 8) |
						 (yuv2bs(y1, u, v) << 16) |
						 (0xff000000);
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		texdata
		);

	free(texdata);
}

static GLint tex_type[4] = {
	GL_UNSIGNED_SHORT_1_5_5_5_REV,
	GL_UNSIGNED_SHORT_5_6_5,
	GL_UNSIGNED_SHORT_4_4_4_4_REV,
	GL_UNSIGNED_INT_8_8_8_8_REV
};

void CreateTextureCLUT4(TxtrCache *texture) {
	u32 i, j;
	void *texdata = malloc(texture->size);

	u32 tw = texture->width;
	u32 th = texture->height;

	u32 *clut = (u32*)&REGS[0x9000 + ((texture->tcw.special) << 6)];
	u8  *src = (u8*)&VRAM[texture->texaddr];

	u32 min = th < tw ? th > > 1 : tw >> 1;
	u32 mask = min - 1;
	u32 notmask = ~mask;
	u32 powmin = uppow2(min);
	u32 powtw = uppow2(tw) - powmin;

	if (texture->tcw.mipmapped)
		src += mipmapcodeoffsclut[uppow2(tw)];

	if ((REGS8(0x8108) & 3) == 3) {
		u32 *dst = (u32*)texdata;

		for (i = 0; i < (th >> 1); i++, dst += tw) {
			u32 twiddley = TWIDDLE(i & mask);
			u32 chunky = (i & notmask) << powtw;
			for (j = 0; j < tw; j += 2, dst += 2) {
				u32 chunknum = (chunky + (j & notmask)) << powmin;
				u32 data = (u32) * (u16*)&src[(TWIDDLE(j & mask) | twiddley << 1) + chunknum];

				dst[0] = (u32)clut[(data >> 0) & 0xf];
				dst[1] = (u32)clut[(data >> 8) & 0xf];
				dst[tw + 0] = (u32)clut[(data >> 4) & 0xf];
				dst[tw + 1] = (u32)clut[(data >> 12) & 0xf];
			}
		}
	} else {
		u16 *dst = (u16*)texdata;

		for (i = 0; i < (th >> 1); i++, dst += tw) {
			u32 twiddley = TWIDDLE(i & mask);
			u32 chunky = (i & notmask) << powtw;
			for (j = 0; j < tw; j += 2, dst += 2) {
				u32 chunknum = (chunky + (j & notmask)) << powmin;
				u32 data = (u32) * (u16*)&src[(TWIDDLE(j & mask) | twiddley << 1) + chunknum];

				dst[0] = (u16)clut[(data >> 0) & 0xf];
				dst[1] = (u16)clut[(data >> 8) & 0xf];
				dst[tw + 0] = (u16)clut[(data >> 4) & 0xf];
				dst[tw + 1] = (u16)clut[(data >> 12) & 0xf];
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_BGRA,
		tex_type[REGS8(0x8108) & 3],
		texdata
		);

	free(texdata);
}

void CreateTextureCLUT8(TxtrCache *texture) {
	u32 i, j;
	void *texdata = malloc(texture->size);

	u32 tw = texture->width;
	u32 th = texture->height;

	u32 *clut = (u32*)&REGS[0x9000 + ((texture->tcw.special) << 6)];
	u8  *src = (u8*)&VRAM[texture->texaddr];

	u32 min = th < tw ? th : tw;
	u32 mask = min - 1;
	u32 notmask = ~mask;
	u32 powmin = uppow2(min);
	u32 powtw = uppow2(tw) - powmin;

	if (texture->tcw.mipmapped)
		src += mipmapcodeoffs[powmin];

	if ((REGS8(0x8108) & 3) == 3) {
		u32 *dst = (u32*)texdata;

		for (i = 0; i < th; i++) {
			u32 twiddley = TWIDDLE(i & mask);
			u32 chunky = (i & notmask) << powtw;
			for (j = 0; j < tw; j++) {
				u32 chunknum = (chunky + (j & notmask)) << powmin;
				u32 data = src[twiddley | (TWIDDLE(j & mask) << 1) + chunknum];
				*dst++ = (u32)clut[data];
			}
		}
	} else {
		u16 *dst = (u16*)texdata;

		for (i = 0; i < th; i++) {
			u32 twiddley = TWIDDLE(i & mask);
			u32 chunky = (i & notmask) << powtw;
			for (j = 0; j < tw; j++) {
				u32 chunknum = (chunky + (j & notmask)) << powmin;
				u32 data = src[twiddley | (TWIDDLE(j & mask) << 1) + chunknum];
				*dst++ = (u16)clut[data];
			}
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		texture->width,
		texture->height,
		0,
		GL_BGRA,
		tex_type[REGS8(0x8108) & 3],
		texdata
		);

	free(texdata);
}
