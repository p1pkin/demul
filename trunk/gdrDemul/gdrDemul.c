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
#include "device.h"
#include "config.h"
#include "plugins.h"

HINSTANCE hinstance;
HWND hWnd;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) {
	hinstance = (HINSTANCE)hModule;
	return TRUE;
}

u32 FASTCALL getType() {
	return PLUGIN_TYPE_GDR;
}

char* FASTCALL getName() {
	return "gdrDemul";
}

u32 FASTCALL gdrOpen(DEmulInfo *demulInfo) {
	if (!LoadConfig(true))
		return 0;
	return OpenDevice(cfg.drive);
}

void FASTCALL gdrClose() {
	CloseDevice();
}

void FASTCALL gdrReset() {
}

void FASTCALL gdrConfigure() {
	if (LoadConfig(false))
		SetConfig();
}

void FASTCALL gdrAbout() {
}

u32 FASTCALL gdrGetStatus() {
	return GerStatusDevice();
}

void FASTCALL gdrReadTOC(u8 *buffer, u32 size) {
	ReadTOCDevice(buffer, size);
}

void FASTCALL gdrReadInfoSession(u8 *buffer, u32 session, u32 size) {
	ReadInfoSessionDevice(buffer, session, size);
}

void FASTCALL gdrReadSectors(u8 *buffer, u32 sector, u32 count, u32 mode) {
	u32 flags;
	u32 sSize;

	if ((mode & 0xe) == 0x8) {
		sSize = 2048;
		flags = 0x10;
	} else {
		sSize = 2056;
		flags = 0x50;
	}

	for (;; ) {
		u32 sCount;
		u32 size;

		if (count > MAX_SECTOR) {
			size = sSize * MAX_SECTOR;
			sCount = MAX_SECTOR;
		} else {
			size = sSize * count;
			sCount = count;
		}

		ReadDevice(buffer, size, sector, sCount, flags);

		if ((s32)(count -= MAX_SECTOR) <= 0) break;
		sector += MAX_SECTOR;
		buffer += size;
	}
}