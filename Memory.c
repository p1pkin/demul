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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "sh4.h"
#include "mmu.h"
#include "onchip.h"
#include "cache.h"
#include "asic.h"
#include "aica.h"
#include "gdrom.h"
#include "rts.h"
#include "modem.h"
#include "gaps.h"
#include "configure.h"
#include "demul.h"

#ifdef DEMUL_DEBUG
static bool mem_quiet_mode;
#endif

int memOpen() {
	int i;

	prom = malloc(0x00200000);
	pram = malloc(0x01000000);
	pflash = malloc(0x00020000);
	pgpu = malloc(0x00800000);
	pspu = malloc(0x00200000);

	prec = (u8*)VirtualAlloc(NULL, 16 * 1024 * 1024, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if ((prom == NULL) || (pram == NULL) ||
		(pflash == NULL) || (pspu == NULL) || (pgpu == NULL)) {
		memClose();
		return 0;
	}
	;

	memset(pram, 0, 0x01000000);
	memset(pgpu, 0, 0x00800000);
	memset(pspu, 0, 0x00200000);

	if (!loadBios())
		return 0;

	for (i = 0; i < 0x10000; i++) memAccess[i] = 0;

	for (i = 0; i < 0x0020; i++) memAccess[i + 0x0000] = (u32) & prom[i << 16];
	for (i = 0; i < 0x0020; i++) memAccess[i + 0x8000] = (u32) & prom[i << 16];
	for (i = 0; i < 0x0020; i++) memAccess[i + 0xA000] = (u32) & prom[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x0c00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xAc00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x8c00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x0d00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xAd00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x8d00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x0e00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xAe00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0100; i++) memAccess[i + 0x8e00] = (u32) & pram[i << 16];
	for (i = 0; i < 0x0020; i++) memAccess[i + 0x0080] = (u32) & pspu[i << 16];
	for (i = 0; i < 0x0020; i++) memAccess[i + 0xA080] = (u32) & pspu[i << 16];
	for (i = 0; i < 0x0002; i++) memAccess[i + 0x0020] = (u32) & pflash[i << 16];
	for (i = 0; i < 0x0002; i++) memAccess[i + 0xa020] = (u32) & pflash[i << 16];
	for (i = 0; i < 0x0080; i++) memAccess[i + 0x0400] = (u32) & pgpu[i << 16];
	for (i = 0; i < 0x0080; i++) memAccess[i + 0xa400] = (u32) & pgpu[i << 16];
	for (i = 0; i < 0x0080; i++) memAccess[i + 0x0500] = (u32) & pgpu[i << 16];
	for (i = 0; i < 0x0080; i++) memAccess[i + 0xa500] = (u32) & pgpu[i << 16];

	for (i = 0; i < 0x0400; i++) memAccess[i + 0x7C00] = 1;

	for (i = 0; i < 0x0001; i++) memAccess[i + 0x005F] = 2;
	for (i = 0; i < 0x0001; i++) memAccess[i + 0xA05F] = 2;

	for (i = 0; i < 0x0001; i++) memAccess[i + 0xA060] = 3;

	for (i = 0; i < 0x0001; i++) memAccess[i + 0x0070] = 4;
	for (i = 0; i < 0x0001; i++) memAccess[i + 0xA070] = 4;

	for (i = 0; i < 0x0001; i++) memAccess[i + 0x0071] = 5;
	for (i = 0; i < 0x0001; i++) memAccess[i + 0xA071] = 5;

	for (i = 0; i < 0x0001; i++) memAccess[i + 0xA100] = 6;

	for (i = 0; i < 0x0400; i++) memAccess[i + 0xE000] = 7;

	for (i = 0; i < 0x0100; i++) memAccess[i + 0xFF00] = 8;

	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF000] = 9;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF100] = 10;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF200] = 11;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF300] = 12;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF400] = 13;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF500] = 14;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF600] = 15;
	for (i = 0; i < 0x0100; i++) memAccess[i + 0xF700] = 16;

	gdrom.busy = 0x50;

	return 1;
}

void memClose() {
	if (prom != NULL) free(prom);
	if (pram != NULL) free(pram);
	if (pflash != NULL) free(pflash);
	if (pgpu != NULL) free(pgpu);
	if (pspu != NULL) free(pspu);
	if (prec != NULL) VirtualFree(prec, 0, MEM_RELEASE);

	prom = NULL;
	pram = NULL;
	pflash = NULL;
	pgpu = NULL;
	pspu = NULL;
	prec = NULL;
}

bool loadBios() {
	char *biosDescription;
	char *flashDescription;
	char message[200 + MAX_DESCRIPTION * 2];

	if (!LoadBIOSFiles(&cfg, prom, pflash, &biosDescription, &flashDescription)) {
		sprintf(message, "Unable to load bios files.\nBios: %s\nFlash: %s", biosDescription, flashDescription);
		MessageBox(GetActiveWindow(), message, "Memory", MB_ICONERROR);
		return false;
	}
	return true;
}

u32 FASTCALL memRead8(u32 mem) {
	u8 *p = (u8*)memAccess[mem >> 16];
	u32 value;

	if ((int)p > 0x7f)
		value = (u32)(s32) * (s8*)(p + (mem & 0xffff));
	else
		switch ((int)p) {
		case 1:
			value = (u32)(s32)(s8)ReadCacheRam8(mem);
			break;
		case 2:
			value = (u32)(s32)(s8)ReadAsic8(mem);
			break;
		case 3:
			value = (u32)(s32)(s8)ReadModem8(mem);
			break;
		case 8:
			value = (u32)(s32)(s8)ReadOnChip8(mem);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memRead8  address = 0x%08lx\n", mem);
				hookDebugException();
			}
#else
			Printf("Unknown memRead8  address = 0x%08lx\n", mem);
#endif
			value = 0;
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugRead(mem, 1);
#endif
	return value;
}

u32 FASTCALL memRead16(u32 mem) {
	u8 *p = (u8*)memAccess[mem >> 16];
	u32 value;

	if ((int)p > 0x7f)
		value = (u32)(s32) * (s16*)(p + (mem & 0xffff));
	else
		switch ((int)p) {
		case 1:
			value = (u32)(s32)(s16)ReadCacheRam16(mem);
			break;
		case 2:
			value = (u32)(s32)(s16)ReadAsic16(mem);
			break;
		case 8:
			value = (u32)(s32)(s16)ReadOnChip16(mem);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memRead16 address = 0x%08lx\n", mem);
				hookDebugException();
			}
#else
			Printf("Unknown memRead16 address = 0x%08lx\n", mem);
#endif
			value = 0;
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugRead(mem, 2);
#endif
	return value;
}

u32 FASTCALL memRead32(u32 mem) {
	u8 *p = (u8*)memAccess[mem >> 16];
	u32 value;

	if ((int)p > 0x7f)
		value = *(u32*)(p + (mem & 0xffff));
	else
		switch ((int)p) {
		case 1:
			value = ReadCacheRam32(mem);
			break;
		case 2:
			value = ReadAsic32(mem);
			break;
		case 4:
			value = ReadAica32(mem);
			break;
		case 5:
			value = ReadRTS32(mem);
			break;
		case 6:
			value = ReadGaps32(mem);
			break;
		case 8:
			value = ReadOnChip32(mem);
			break;
		case 11:
			value = ReadItlbAddr32(mem);
			break;
		case 12:
			value = ReadItlbData32(mem);
			break;
		case 13:
			value = 0;
			break;
		case 15:
			value = ReadUtlbAddr32(mem);
			break;
		case 16:
			value = ReadUtlbData32(mem);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memRead32 address = 0x%08lx\n", mem);
				hookDebugException();
			}
#else
			Printf("Unknown memRead32 address = 0x%08lx\n", mem);
#endif
			value = 0;
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugRead(mem, 4);
#endif
	return value;
}

void FASTCALL memRead64(u32 mem, u64 *out) {
	u8 *p = (u8*)memAccess[mem >> 16];

	if ((int)p > 0x7f)
		*out = *(u64*)(p + (mem & 0xffff));
	else
		switch ((int)p) {
		case 1:
			ReadCacheRam64(mem, out);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memRead64 address = 0x%08lx\n", mem);
				hookDebugException();
			}
#else
			Printf("Unknown memRead64 address = 0x%08lx\n", mem);
#endif
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugRead(mem, 8);
#endif
}

void FASTCALL memWrite8(u32 mem, u8 value) {
	u8 *p = (u8*)memAccess[mem >> 16];

	if ((int)p > 0x7f) {
		*(u8*)(p + (mem & 0xffff)) = value;
		recInvalideBlock(mem, 1);
	} else
		switch ((int)p) {
		case 1:
			WriteCacheRam8(mem, value);
			break;
		case 2:
			WriteAsic8(mem, value);
			break;
		case 3:
			WriteModem8(mem, value);
			break;
		case 8:
			WriteOnChip8(mem, value);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memWrite8  address = 0x%08lx, value = 0x%02x\n", mem, value);
				hookDebugException();
			}
#else
			Printf("Unknown memWrite8  address = 0x%08lx, value = 0x%02x\n", mem, value);
#endif
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugWrite(mem, 1);
#endif
}

void FASTCALL memWrite16(u32 mem, u16 value) {
	u8 *p = (u8*)memAccess[mem >> 16];

	if ((int)p > 0x7f) {
		*(u16*)(p + (mem & 0xffff)) = value;
		recInvalideBlock(mem, 2);
	} else
		switch ((int)p) {
		case 1:
			WriteCacheRam16(mem, value);
			break;
		case 2:
			WriteAsic16(mem, value);
			break;
		case 8:
			WriteOnChip16(mem, value);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memWrite16  address = 0x%08lx, value = 0x%04x\n", mem, value);
				hookDebugException();
			}
#else
			Printf("Unknown memWrite16  address = 0x%08lx, value = 0x%04x\n", mem, value);
#endif
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugWrite(mem, 2);
#endif
}

void FASTCALL memWrite32(u32 mem, u32 value) {
	u8 *p = (u8*)memAccess[mem >> 16];

	if ((int)p > 0x7f) {
		*(u32*)(p + (mem & 0xffff)) = value;
		recInvalideBlock(mem, 4);
	} else
		switch ((int)p) {
		case 1:
			WriteCacheRam32(mem, value);
			break;
		case 2:
			WriteAsic32(mem, value);
			break;
		case 4:
			WriteAica32(mem, value);
			break;
		case 5:
			WriteRTS32(mem, value);
			break;
		case 6:
			WriteGaps32(mem, value);
			break;
		case 7:
			WriteSQ32(mem, value);
			break;
		case 8:
			WriteOnChip32(mem, value);
			break;
		case 11:
			WriteItlbAddr32(mem, value);
			break;
		case 12:
			WriteItlbData32(mem, value);
			break;
		case 13:
			break;
		case 15:
			WriteUtlbAddr32(mem, value);
			break;
		case 16:
			WriteUtlbData32(mem, value);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memWrite32  address = 0x%08lx, value = 0x%08lx\n", mem, value);
				hookDebugException();
			}
#else
			Printf("Unknown memWrite32  address = 0x%08lx, value = 0x%08lx\n", mem, value);
#endif
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugWrite(mem, 4);
#endif
}

void FASTCALL memWrite64(u32 mem, u64 *value) {
	u8 *p = (u8*)memAccess[mem >> 16];

	if ((int)p > 0x7f) {
		*(u64*)(p + (mem & 0xffff)) = *value;
		recInvalideBlock(mem, 8);
	} else
		switch ((int)p) {
		case 1:
			WriteCacheRam64(mem, value);
			break;
		case 7:
			WriteSQ64(mem, value);
			break;
		default:
#ifdef DEMUL_DEBUG
			if (!mem_quiet_mode) {
				Printf("Unknown memWrite64  address = 0x%08lx, value = 0x%08lx%08lx\n", mem, *value);
				hookDebugException();
			}
#else
			Printf("Unknown memWrite64  address = 0x%08lx, value = 0x%08lx%08lx\n", mem, *value);
#endif
		}
#ifdef DEMUL_DEBUG
	if (!mem_quiet_mode) hookDebugWrite(mem, 8);
#endif
}

#ifdef DEMUL_DEBUG
void memQuietMode(bool enable) {
	mem_quiet_mode = enable;
}
#endif
