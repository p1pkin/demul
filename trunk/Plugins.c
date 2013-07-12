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
#include "plugins.h"
#include "memory.h"
#include "gpu.h"
#include "configure.h"

#define LoadFunc(lib, name) {												\
	## name ## = (_ ## name ## )GetProcAddress(h ## lib ## Plugin, # name);	\
	if ( ## name ## == NULL) return false;									\
}

HMODULE hGPUPlugin = NULL;
HMODULE hGDRPlugin = NULL;
HMODULE hPADPlugin = NULL;
HMODULE hSPUPlugin = NULL;

_gpuOpen gpuOpen;
_gpuClose gpuClose;
_gpuReset gpuReset;
_gpuConfigure gpuConfigure;
_gpuAbout gpuAbout;
_gpuSync gpuSync;
_gpuRead32 gpuRead32;
_gpuWrite32 gpuWrite32;
_gpuSetIrqHandler gpuSetIrqHandler;
_gpuTaTransfer gpuTaTransfer;
_gpuYuvTransfer gpuYuvTransfer;

_gdrOpen gdrOpen;
_gdrClose gdrClose;
_gdrReset gdrReset;
_gdrConfigure gdrConfigure;
_gdrAbout gdrAbout;
_gdrReadTOC gdrReadTOC;
_gdrReadInfoSession gdrReadInfoSession;
_gdrGetStatus gdrGetStatus;
_gdrReadSectors gdrReadSectors;

_padOpen padOpen;
_padClose padClose;
_padReset padReset;
_padConfigure padConfigure;
_padAbout padAbout;
_padJoyCaps padJoyCaps;
_padReadJoy padReadJoy;

_spuOpen spuOpen;
_spuClose spuClose;
_spuReset spuReset;
_spuConfigure spuConfigure;
_spuAbout spuAbout;
_spuSync spuSync;
_spuRead8 spuRead8;
_spuRead16 spuRead16;
_spuRead32 spuRead32;
_spuWrite8 spuWrite8;
_spuWrite16 spuWrite16;
_spuWrite32 spuWrite32;


bool loadGPUPlugin(char *name) {
	hGPUPlugin = LoadLibrary(name);
	if (hGPUPlugin == NULL) {
		return false;
	}

	LoadFunc(GPU, gpuOpen);
	LoadFunc(GPU, gpuClose);
	LoadFunc(GPU, gpuReset);
	LoadFunc(GPU, gpuConfigure);
	LoadFunc(GPU, gpuAbout);
	LoadFunc(GPU, gpuSync);
	LoadFunc(GPU, gpuRead32);
	LoadFunc(GPU, gpuWrite32);
	LoadFunc(GPU, gpuSetIrqHandler);
	LoadFunc(GPU, gpuTaTransfer);
	LoadFunc(GPU, gpuYuvTransfer);

	return true;
}

int loadGDRPlugin(char *name) {
	hGDRPlugin = LoadLibrary(name);
	if (hGDRPlugin == NULL) {
		return false;
	}

	LoadFunc(GDR, gdrOpen);
	LoadFunc(GDR, gdrClose);
	LoadFunc(GDR, gdrReset);
	LoadFunc(GDR, gdrConfigure);
	LoadFunc(GDR, gdrAbout);
	LoadFunc(GDR, gdrReadTOC);
	LoadFunc(GDR, gdrReadInfoSession);
	LoadFunc(GDR, gdrGetStatus);
	LoadFunc(GDR, gdrReadSectors);

	return true;
}

int loadPADPlugin(char *name) {
	hPADPlugin = LoadLibrary(name);
	if (hPADPlugin == NULL) {
		return false;
	}

	LoadFunc(PAD, padOpen);
	LoadFunc(PAD, padClose);
	LoadFunc(PAD, padReset);
	LoadFunc(PAD, padConfigure);
	LoadFunc(PAD, padAbout);
	LoadFunc(PAD, padJoyCaps);
	LoadFunc(PAD, padReadJoy);

	return true;
}

bool loadSPUPlugin(char *name) {
	hSPUPlugin = LoadLibrary(name);
	if (hSPUPlugin == NULL) {
		return false;
	}

	LoadFunc(SPU, spuOpen);
	LoadFunc(SPU, spuClose);
	LoadFunc(SPU, spuReset);
	LoadFunc(SPU, spuConfigure);
	LoadFunc(SPU, spuAbout);
	LoadFunc(SPU, spuSync);
	LoadFunc(SPU, spuRead8);
	LoadFunc(SPU, spuRead16);
	LoadFunc(SPU, spuRead32);
	LoadFunc(SPU, spuWrite8);
	LoadFunc(SPU, spuWrite16);
	LoadFunc(SPU, spuWrite32);

	return true;
}


bool openPlugins() {
	int st;

	st = gpuOpen(&demulInfo, pgpu);
	if (st == 0) {
		closePlugins();
		return false;
	}
	gpuSetIrqHandler((void*)gpuIrq);

	st = gdrOpen(&demulInfo);
	if (st == 0) {
		closePlugins();
		return false;
	}

	st = padOpen(&demulInfo);
	if (st == 0) {
		closePlugins();
		return false;
	}

	st = spuOpen(&demulInfo, pspu);
	if (st == 0) {
		closePlugins();
		return false;
	}

	return true;
}

void closePlugins() {
	gpuClose();
	gdrClose();
	padClose();
	spuClose();
}

bool loadPlugins() {
	if (!loadGPUPlugin(cfg.gpuPluginName)) {
		MessageBox(GetActiveWindow(), "Unable to load", "GPUPlugin", MB_ICONERROR);
		unloadPlugins();
		return false;
	}

	if (!loadGDRPlugin(cfg.gdrPluginName)) {
		MessageBox(GetActiveWindow(), "Unable to load", "GDRPlugin", MB_ICONERROR);
		unloadPlugins();
		return false;
	}

	if (!loadPADPlugin(cfg.padPluginName)) {
		MessageBox(GetActiveWindow(), "Unable to load", "PADPlugin", MB_ICONERROR);
		unloadPlugins();
		return false;
	}

	if (!loadSPUPlugin(cfg.spuPluginName)) {
		MessageBox(GetActiveWindow(), "Unable to load", "SPUPlugin", MB_ICONERROR);
		unloadPlugins();
		return false;
	}

	return true;
}

void unloadPlugins() {
	if (hGPUPlugin != NULL) FreeLibrary((HMODULE)hGPUPlugin);
	if (hGDRPlugin != NULL) FreeLibrary((HMODULE)hGDRPlugin);
	if (hPADPlugin != NULL) FreeLibrary((HMODULE)hPADPlugin);
	if (hSPUPlugin != NULL) FreeLibrary((HMODULE)hSPUPlugin);

	hGPUPlugin = NULL;
	hGDRPlugin = NULL;
	hPADPlugin = NULL;
	hSPUPlugin = NULL;
}

