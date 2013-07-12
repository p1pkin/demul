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
#include "console.h"

#include "sh4.h"
#include "plugins.h"
#include "asic.h"
#include "gdrom.h"
#include "maple.h"
#include "memory.h"
#include "onchip.h"

#include "debug.h"

u8* dmaGetAddr(u32 addr) {
	u8* ptr = (u8*)memAccess[addr >> 16];

	if (ptr == NULL) {
		Printf("DMA error = 0x%08lx\n", addr);
		return NULL;
	}

	return (u8*)(ptr + (addr & 0xffe0));
}

void PvrDma() {
	u8 *src = dmaGetAddr(*(u32*)&pOnChip[0xa020]);

	if (src != NULL) {
		switch (ASIC32(0x6800) & 0xff800000) {
		case 0x10000000:
		{
			u32 size = ASIC32(0x6804);

			gpuTaTransfer((u32*)src, size);
			break;
		}

		case 0x10800000:
		{
			u8  *dst = &pgpu[ASIC32(0x8148)];

			gpuYuvTransfer(src, dst);

			hwInt(0x0006);
			break;
		}

		case 0x11000000:
		{
			u8  *dst = (u8*)&pgpu[ASIC32(0x6800) & 0x007fffff];
			u32 size = ASIC32(0x6804);

			memcpy(dst, src, size);
			break;
		}

		default:
			Printf("dma unknown address 0x%08lx\n", ASIC32(0x6800));
			break;
		}

		ASIC32(0x6800) += ASIC32(0x6804);
		ASIC32(0x6804) = 0;
		ASIC32(0x6808) = 0;
		ONCHIP32(0xa020) += ONCHIP32(0xa028) * 32;
		ONCHIP32(0xa028) = 0;
		ONCHIP32(0xa02c) &= ~1;
		ONCHIP32(0xa02c) |= 2;

		hwInt(0x0013);
	}
}

void MapleDma() {
	u32* p = (u32*)dmaGetAddr(ASIC32(0x6c04));

	if (p != NULL) {
		u32 last;

		do {
			last = ((p[0] >> 31) & 0x01);

			if ((p[0] & 0x0700) == 0) {
				u8 *addr = dmaGetAddr(p[1]);

				if (addr != NULL)
					switch (p[2] & 0xff) {
					case 1:  RequestDeviceInformation(addr, &p[2]);             break;
					case 2:  Printf("Request extended device information\n");   break;
					case 3:  ResetDevice(addr, &p[2]);                          break;
					case 4:  Printf("Shutdown device\n");                       break;
					case 9:  Getcondition(addr, &p[2]);                         break;
					case 10: GetMemoryInformation(addr, &p[2]);                 break;
					case 11: BlockRead(addr, &p[2]);                            break;
					case 12: BlockWrite(addr, &p[2]);                           break;
					case 13: GetLastErorr(addr, &p[2]);                         break;
					default: Printf("unknown maple code 0x%08lx\n", p[2] & 0xff); break;
					}

				p += (p[0] & 0xff) + 3;
			} else {
				p++;
			}
		} while (!last);

		hwInt(0x000c);
	}
}


void AicaDma(u32 ch) {
	if
	(
		(ASIC8_UNMASKED(0x7814 + (ch * 32)) & 1) &&
		(ASIC8_UNMASKED(0x7818 + (ch * 32)) & 1)
	) {
		u32 *p = &ASIC32_UNMASKED(0x7800 + (ch * 32));
		u8  *dst = dmaGetAddr(p[0]);
		u8  *src = dmaGetAddr(p[1]);
		u32 size = p[2] & 0x1fffffe0;

		if
		(
			(dst != NULL) &&
			(src != NULL)
		) {
			if ((p[2] >> 31) & 1) p[5] = 0;

			p[0] += size;
			p[1] += size;
			p[2] = 0;
			p[6] = 0;

			if (p[4] != 5) return;

			if (p[3] == 0) memcpy(dst, src, size);
			else memcpy(src, dst, size);
		}

		hwInt(0x000f);
	}
}

void GdromDma() {
	if
	(
		(ASIC8(0x7414) & 1) &&
		(ASIC8(0x7418) & 1)
	) {
		u32 dst = ASIC32(0x7404);
		u32 size = ASIC32(0x7408);

		ASIC32(0x7418) = 0;
		ASIC32(0x74f8) = size;
		recInvalideBlock(dst, size);

		memcpy((void*)dmaGetAddr(dst), (void*)&buffer[position], size);

		if ((position += size) == bufsize) {
			gdrom.busy &= 0x36;
			gdrom.busy |= 0x40;
			hwInt(0x000e);
		}

		hwInt(0x0100);
	}
}
