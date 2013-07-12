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
#include <process.h>
#include <wincon.h>

#include "resource.h"
#include "console.h"
#include "demul.h"
#include "plugins.h"
#include "configure.h"

DEmulInfo demulInfo;

void UpdateGUIMenuState() {
	HMENU hmenu = GetMenu(demulInfo.hMainWnd);

	CheckMenuItem(hmenu, ID_MODE_INTERPRETER, MF_BYCOMMAND | (!cfg.sh4mode) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_MODE_DYNAREC, MF_BYCOMMAND | (cfg.sh4mode) ? MF_CHECKED : MF_UNCHECKED);
	#ifdef DEMUL_DEBUG
	if (!cfg.sh4mode)
		openDebug();
	else
		closeDebug();
	memQuietMode(cfg.sh4mode);
	#endif
}


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_QUIT:
	case WM_CLOSE:
	case WM_DESTROY:
		if (demulInfo.hEmu != INVALID_HANDLE_VALUE) SuspendThread(demulInfo.hEmu);
		CloseEmu();
		if (demulInfo.hEmu != INVALID_HANDLE_VALUE) TerminateThread(demulInfo.hEmu, 1);
		if (demulInfo.hEmu != INVALID_HANDLE_VALUE) CloseHandle(demulInfo.hEmu);
		exit(1);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDR_EMU_RUN:
			if (demulInfo.hEmu == INVALID_HANDLE_VALUE)
				demulInfo.hEmu = (HANDLE)_beginthread(RunEmu, 0, NULL);
			return TRUE;

		case ID_CONFIG_GDR:
			gdrConfigure();
			return TRUE;
		case ID_CONFIG_GPU:
			gpuConfigure();
			return TRUE;
		case ID_CONFIG_SPU:
			spuConfigure();
			return TRUE;
		case ID_CONFIG_PAD:
			padConfigure();
			return TRUE;
		case ID_CONFIG_BIOS_AND_PLUGINS:
			SetConfig();
			return TRUE;
		case ID_MODE_INTERPRETER:
			cfg.sh4mode = MODE_INTERPRETER;
			SaveConfig();
			UpdateGUIMenuState();
			return TRUE;
		case ID_MODE_DYNAREC:
			cfg.sh4mode = MODE_DYNAREC;
			SaveConfig();
			UpdateGUIMenuState();
			return TRUE;

		default:
			return TRUE;
		}

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;
	MSG msg;
	demulInfo.hEmu = INVALID_HANDLE_VALUE;
	demulInfo.hMainInstance = hInstance;

	if (OpenEmu() == 0) return 0;

	wc.lpszClassName = "window";
	wc.lpfnWndProc = WndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = "MENU";
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	RegisterClass(&wc);

	demulInfo.hMainWnd = CreateWindow(
		"window",
		"DEMUL",
		WS_OVERLAPPED | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_CLIPCHILDREN,
		20,
		20,
		320 + GetSystemMetrics(SM_CXFIXEDFRAME) * 2,
		200 + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION),
		NULL,
		NULL,
		hInstance,
		NULL
		);


	UnregisterClass("window", hInstance);
	ShowWindow(demulInfo.hMainWnd, SW_SHOWNORMAL);
	UpdateWindow(demulInfo.hMainWnd);

	UpdateGUIMenuState();
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}