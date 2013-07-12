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
#include "gdrom.h"
#include "asic.h"
#include "sh4.h"
#include "counters.h"
#include "plugins.h"

u8   ReadGdrom8(u32 mem) {
	switch (mem & 0x007fffff) {
	case 0x005f7018:	return gdrom.busy;
	case 0x005f708C:	return gdrGetStatus();
	case 0x005f7090:	return gdrom.cntlo;
	case 0x005f7094:	return gdrom.cnthi;
	case 0x005f709C:
		ASIC8(0x6904) &= ~1;
		return gdrom.busy;
	default:
		return 0;
	}
}

u16  ReadGdrom16(u32 mem) {
	u16 value = ((u16*)buffer)[index++];

	if ((count -= 2) == 0) {
		count = index * 2;
		index = 0;
		gdrom.busy &= 0x37;
		gdrom.busy |= 0x40;
		hwInt(0x0100);
	}

	return value;
}

void WriteGdrom8(u32 mem, u8 value) {
	switch (mem & 0x007fffff) {
	case 0x005f7018:    gdrom.busy = value; return;
	case 0x005f7084:    gdrom.ftrs = value; return;
	case 0x005f7088:    gdrom.irsn = value; return;
	case 0x005f708C:    gdrom.stat = value; return;
	case 0x005f7090:    gdrom.cntlo = value; return;
	case 0x005f7094:    gdrom.cnthi = value; return;

	case 0x005f709C:

		switch (value) {
		case 0x00:
			gdrom.busy &= 0x76;
			gdrom.busy |= 0x01;
			hwInt(0x0100);
			return;

		case 0xa0:
			index = 0;
			gdrom.busy &= 0xfe;
			gdrom.busy |= 0x08;
			return;

		default:
			//Printf("gdrom unknown command 0x%02x", value);
			return;
		}

	default:
		return;
	}
}

void WriteGdrom16(u32 mem, u16 value) {
	u16 *data = (u16*)&buffer;
	data[index++] = value;

	if (index == 6) {
		index = 0;

		gdrom.busy &= 0xf7;
		gdrom.busy |= 0x80;

		switch (buffer[0]) {
		case 0x00:
			gdrom.busy &= 0x37;
			gdrom.busy |= 0x40;
			hwInt(0x0100);
			break;

		case 0x10:
		{
			u8  *src = &rom[2188];
			u8  *dst = buffer;
			u32 size = buffer[4];

			memcpy(dst, src, size);
			count = size;
			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}

		case 0x11:
		{
			u8  *src = &rom[1976] + buffer[2];
			u8  *dst = buffer;
			u32 size = buffer[4];

			memcpy(dst, src, size);

			count = size;
			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}
/*
		case 0x13:
			gdrom.busy = 0x58;//(gdrom.busy & 0x37) | 0x40;
			buffer[0] = 0x00f0;
			buffer[1] = 0x0002;
			buffer[2] = 0x0000;
			buffer[3] = 0x0000;
			buffer[4] = 0x003a;
			buffer[5] = 0x0000;
			hwInt(31, 0x0100);
			break;
*/
		case 0x14:
		{
			u8 size[2] = { buffer[4], buffer[3] };

			gdrReadTOC(buffer, (u32) * (u16*)size);

			count = (u32) * (u16*)size;
			gdrom.cntlo = size[0];
			gdrom.cnthi = size[1];
			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}

		case 0x15:
		{
			u32 session = buffer[2];
			u32 size = (u32)buffer[4];

			gdrReadInfoSession(buffer, session, size);

			count = size;
			gdrom.cntlo = (u8)size;
			gdrom.cnthi = 0;
			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}

		case 0x20:
		case 0x21:
			gdrom.busy &= 0x37;
			gdrom.busy |= 0x40;
			hwInt(0x0100);
			break;

		case 0x30:
		{
			u32 length = ((u32)buffer[9] << 8) | ((u32)buffer[10]);
			u32 sector = ((u32)buffer[2] << 16) | ((u32)buffer[3] << 8) | ((u32)buffer[4]);
			u32 mode = ((u32)buffer[1]);

			gdrReadSectors(buffer, sector, length, mode);

			if (gdrom.ftrs & 1) {
				position = 0;
				bufsize = length * 2048;
				gdrom.busy &= 0xf7;
				gdrom.busy |= 0x80;
			} else {
				count = length * 2048;
				gdrom.cntlo = ((count >> 0) & 0xff);
				gdrom.cnthi = ((count >> 8) & 0xff);

				gdrom.busy &= 0x76;
				gdrom.busy |= 0x08;
				hwInt(0x0100);
			}
			break;
		}

		case 0x40:
		{
			u8  *src;
			u8  *dst = buffer;
			u32 size = (u32)buffer[4];

			switch (buffer[1] & 3) {
			case 0: src = &rom[2024];   break;
			case 1: src = &rom[2124];   break;
			case 2: src = &rom[2140];   break;
			case 3: src = &rom[2164];   break;
			}

			memcpy(dst, src, size);

			count = size;
			gdrom.cntlo = ((size >> 0) & 0xff);
			gdrom.cnthi = ((size >> 8) & 0xff);

			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}

		case 0x70:
			gdrom.busy = 0x50;
			hwInt(0x0100);
			break;

		case 0x71:
		{
			static u32 flag = 0;

			if (!flag) {
				u8  *src = &rom[0];
				u8  *dst = buffer;

				memcpy(dst, src, 0x3f4);

				count = 0x03f4;
				gdrom.cntlo = 0xf4;
				gdrom.cnthi = 0x03;

				flag = 1;
			} else {
				u8  *src = &rom[0x3f8];
				u8  *dst = buffer;

				memcpy(dst, src, 0x3c0);

				count = 0x03c0;
				gdrom.cntlo = 0xC0;
				gdrom.cnthi = 0x03;

				flag = 0;
			}

			gdrom.busy &= 0x76;
			gdrom.busy |= 0x08;
			hwInt(0x0100);
			break;
		}

		default:
			//Printf("gdrom packet unknown command 0x%02x", buffer[0]);
			break;
		}
	}
}


