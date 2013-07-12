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
#include <time.h>

#include "gpuDemul.h"
#include "gpuWnd.h"
#include "gpuregs.h"
#include "gpuTransfer.h"
#include "gputexture.h"
#include "gpurenderer.h"

#include "plugins.h"
#include "config.h"

#define MAX_PROFILE_COUNT 8

static LARGE_INTEGER start[MAX_PROFILE_COUNT];
static LARGE_INTEGER finish[MAX_PROFILE_COUNT];

HINSTANCE hinstance = NULL;
DEmulInfo *gDemulInfo = NULL;

void ProfileStart(int n) {
	QueryPerformanceCounter(&start[n]);
}

void ProfileFinish(int n) {
	LONGLONG d;
	QueryPerformanceCounter(&finish[n]);
	d = finish[n].QuadPart - start[n].QuadPart;
	DEMUL_printf("profile[%d] %d\n", n, d);
}

void DEMUL_printf(char *format, ...) {
	FILE *ofile;
	char temp[2048];
	va_list ap;
	va_start(ap, format);
	vsprintf(temp, format, ap);
	ofile = fopen("stdout.txt", "ab");
	fwrite(temp, 1, strlen(temp), ofile);
	fclose(ofile);
	va_end(ap);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) {
	hinstance = (HINSTANCE)hModule;
	return TRUE;
}

u32 FASTCALL getType() {
	return PLUGIN_TYPE_GPU;
}

char* FASTCALL getName() {
	return GPU_MODULE_NAME;
}

int FASTCALL gpuOpen(DEmulInfo *demulInfo, void *pgpu) {
	gDemulInfo = demulInfo;

	if (!LoadConfig(true))
		return 0;

	VRAM = pgpu;

	if (CreateGpuWindow() == 0)
		return 0;

	if (InitialOpenGL() == 0)
		return 0;

//	if(InitialTextures()== 0)
//		return 0;

	return 1;
}

void FASTCALL gpuClose() {
	DeleteOpenGL();
	ClearTextures();
	CloseGpuWindow();
}

void FASTCALL gpuReset() {
}

void FASTCALL gpuConfigure() {
	if (LoadConfig(false))
		SetConfig();
}

void FASTCALL gpuAbout() {
}

int fps;
time_t t;
char szTitle[256];
extern float angle;

void FASTCALL gpuSync() {
	MSG msg;

	while (PeekMessage(&msg, gDemulInfo->hGpuWnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	{
		u32 FB_R_SOF1 = REGS32(0x8050);
		u32 FB_R_CTRL = REGS32(0x8044);
		u32 *pix1 = (u32*)&VRAM[(FB_R_SOF1 & 0x007FFFFF)];
		u32 *pix2 = (u32*)&VRAM[(FB_R_SOF1 & 0x007FFFFF) + 640 * 480];
		if (((*pix1) != 0xBABAC0C0) && ((*pix2) != 0xBABAC0C0)) {
			u32 FB_W_SOF1 = REGS32(0x8060);
			if ((!(FB_W_SOF1 & 0x01000000)) && (FB_W_SOF1 != FB_R_SOF1) && (FB_R_CTRL & 1))
				glRenderFramebuffer(FB_R_SOF1 & 0x007FFFFF, (FB_R_CTRL & 0x0C) >> 2);
		}
		renderingison = 0;
	}

	fps++;

	if (time(NULL) > t) {
		time(&t);
		sprintf(szTitle, "fps %0ld", fps);
		SetWindowText(gDemulInfo->hGpuWnd, szTitle);
		fps = 0;
	}
}

void FASTCALL gpuSetIrqHandler(void (*Handler)) {
	IRQ = Handler;
}
