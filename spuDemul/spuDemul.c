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
#include "spuRegs.h"
#include "spudevice.h"
#include "plugins.h"

HINSTANCE hinstance;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) {
	hinstance = (HINSTANCE)hModule;
	return TRUE;
}

u32 FASTCALL getType() {
	return PLUGIN_TYPE_SPU;
}

char* FASTCALL getName() {
	return "spuDemul";
}

u32 FASTCALL spuOpen(DEmulInfo *demulInfo, void *pspu) {
	ARAM = pspu;
	if (!OpenDevice()) {
		CloseDevice();
		return 0;
	}
	return 1;
}

void FASTCALL spuClose() {
	CloseDevice();
	ARAM = NULL;
}

void FASTCALL spuReset() {
}

void FASTCALL spuConfigure() {
}

void FASTCALL spuAbout() {
}
void FASTCALL spuSync(u32 samples) {
	//DeviceSync(samples);
}