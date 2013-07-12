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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "types.h"
#include <windows.h>

#define PLUGIN_TYPE_GPU 0
#define PLUGIN_TYPE_SPU 1
#define PLUGIN_TYPE_GDR 2
#define PLUGIN_TYPE_PAD 3

#define MAX_PLUGIN_NAME 32

// global - main module variables
typedef struct DEmulInfo {
	HANDLE hEmu;
	HWND hMainWnd;
	HINSTANCE hMainInstance;
	HWND hGpuWnd;
} DEmulInfo;

#ifdef _HOST_PLUGINS

bool  openPlugins();
void closePlugins();

bool  loadPlugins();
void unloadPlugins();


typedef u32 (FASTCALL * _getType)();
typedef char* (FASTCALL * _getName)();

typedef int (FASTCALL * _gpuOpen)(DEmulInfo *demulInfo, void* pgpu);
typedef void (FASTCALL * _gpuClose)();
typedef void (FASTCALL * _gpuReset)();
typedef void (FASTCALL * _gpuConfigure)();
typedef void (FASTCALL * _gpuAbout)();
typedef void (FASTCALL * _gpuSync)();
typedef u32 (FASTCALL * _gpuRead32)(u32 mem);
typedef void (FASTCALL * _gpuWrite32)(u32 mem, u32 value);
typedef void (FASTCALL * _gpuSetIrqHandler)(void (*Handler));
typedef void (FASTCALL * _gpuTaTransfer)(u32 *mem, u32 size);
typedef void (FASTCALL * _gpuYuvTransfer)(u8 *src, u8 *dst);

extern _gpuOpen gpuOpen;
extern _gpuClose gpuClose;
extern _gpuReset gpuReset;
extern _gpuConfigure gpuConfigure;
extern _gpuAbout gpuAbout;
extern _gpuSync gpuSync;
extern _gpuRead32 gpuRead32;
extern _gpuWrite32 gpuWrite32;
extern _gpuSetIrqHandler gpuSetIrqHandler;
extern _gpuTaTransfer gpuTaTransfer;
extern _gpuYuvTransfer gpuYuvTransfer;

typedef int (FASTCALL * _gdrOpen)(DEmulInfo *demulInfo);
typedef void (FASTCALL * _gdrClose)();
typedef void (FASTCALL * _gdrReset)();
typedef void (FASTCALL * _gdrConfigure)();
typedef void (FASTCALL * _gdrAbout)();
typedef void (FASTCALL * _gdrReadTOC)(u8 *buffer, u32 size);
typedef void (FASTCALL * _gdrReadInfoSession)(u8 *buffer, u32 session, u32 size);
typedef u32 (FASTCALL * _gdrGetStatus)();
typedef void (FASTCALL * _gdrReadSectors)(u8 *buffer, u32 sector, u32 count, u32 mode);

extern _gdrOpen gdrOpen;
extern _gdrClose gdrClose;
extern _gdrReset gdrReset;
extern _gdrConfigure gdrConfigure;
extern _gdrAbout gdrAbout;
extern _gdrReadTOC gdrReadTOC;
extern _gdrReadInfoSession gdrReadInfoSession;
extern _gdrGetStatus gdrGetStatus;
extern _gdrReadSectors gdrReadSectors;

typedef int (FASTCALL * _padOpen)(DEmulInfo *demulInfo);
typedef void (FASTCALL * _padClose)();
typedef void (FASTCALL * _padReset)();
typedef void (FASTCALL * _padConfigure)();
typedef void (FASTCALL * _padAbout)();
typedef u32 (FASTCALL * _padJoyCaps)(u32 port);
typedef void (FASTCALL * _padReadJoy)(u8 *keymask, u32 port);

extern _padOpen padOpen;
extern _padClose padClose;
extern _padReset padReset;
extern _padConfigure padConfigure;
extern _padAbout padAbout;
extern _padJoyCaps padJoyCaps;
extern _padReadJoy padReadJoy;

typedef int (FASTCALL * _spuOpen)(DEmulInfo *demulInfo, void *pspu);
typedef void (FASTCALL * _spuClose)();
typedef void (FASTCALL * _spuReset)();
typedef void (FASTCALL * _spuConfigure)();
typedef void (FASTCALL * _spuAbout)();
typedef void (FASTCALL * _spuSync)(u32 samples);
typedef u8 (FASTCALL * _spuRead8)(u32 mem);
typedef u16 (FASTCALL * _spuRead16)(u32 mem);
typedef u32 (FASTCALL * _spuRead32)(u32 mem);
typedef void (FASTCALL * _spuWrite8)(u32 mem, u8 value);
typedef void (FASTCALL * _spuWrite16)(u32 mem, u16 value);
typedef void (FASTCALL * _spuWrite32)(u32 mem, u32 value);

extern _spuOpen spuOpen;
extern _spuClose spuClose;
extern _spuReset spuReset;
extern _spuConfigure spuConfigure;
extern _spuAbout spuAbout;
extern _spuSync spuSync;
extern _spuRead8 spuRead8;
extern _spuRead16 spuRead16;
extern _spuRead32 spuRead32;
extern _spuWrite8 spuWrite8;
extern _spuWrite16 spuWrite16;
extern _spuWrite32 spuWrite32;
#endif
#endif
