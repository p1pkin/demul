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

#include "gpuWnd.h"
#include "gpurenderer.h"
#include "config.h"
#include "gputexture.h"
#include "png.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		return TRUE;

	case WM_KEYDOWN:
		switch (LOWORD(wParam)) {
		case VK_F9:
		{
			u8 *buf = malloc(width * height * 3);
			getSnapshot(buf);
			SaveSnapshot(buf, width * height * 3);
			free(buf);
		}
		}

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return FALSE;
}

int CreateGpuWindow() {
	DEVMODE dmScreenSettings;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;
	RECT WindowRect;

	width = 640;
	height = 480;

	WindowRect.left = 0;
	WindowRect.right = width;
	WindowRect.top = 0;
	WindowRect.bottom = height;

	if (cfg.fullScreen) {
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = width;
		dmScreenSettings.dmPelsHeight = height;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			return 0;
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor(FALSE);
	} else {
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
		/*
		WS_OVERLAPPED	|
		                        WS_SYSMENU		|
		                        WS_MINIMIZEBOX	|
		                        WS_CLIPCHILDREN
		*/
	}
	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	wc.lpszClassName = "WC";
	wc.lpfnWndProc = WndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	if (!RegisterClass(&wc))
		return 0;

	if (!(gDemulInfo->hGpuWnd = CreateWindowEx(dwExStyle,
											   "WC",
											   "GPU PLUGIN", dwStyle
											   ,
											   CW_USEDEFAULT,
											   CW_USEDEFAULT,
											   width + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + 2,
											   height + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) + 2,
											   NULL,
											   NULL,
											   hinstance,
											   NULL
											   )
		  )) return 0;

	UnregisterClass("WC", hinstance);
	ShowWindow(gDemulInfo->hGpuWnd, SW_SHOWNORMAL);
	UpdateWindow(gDemulInfo->hGpuWnd);

	return 1;
}

void CloseGpuWindow() {
	if (gDemulInfo->hGpuWnd != NULL)
		DestroyWindow(gDemulInfo->hGpuWnd);
	gDemulInfo->hGpuWnd = NULL;
}