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
#include "resource.h"
#include "config.h"
#include "gpuDemul.h"
#include "inifile.h"

CFG cfg;

WPARAM GetCBState(long state) {
	return state != 0 ? BST_CHECKED : BST_UNCHECKED;
}

void UpdateCfg(HWND hwnd) {
	cfg.fullScreen = SendDlgItemMessage(hwnd, IDC_FULL_SCREEN, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.ListSorting = SendDlgItemMessage(hwnd, IDC_CHECK2, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.AlphasubSort = SendDlgItemMessage(hwnd, IDC_CHECK3, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.PunchsubSort = SendDlgItemMessage(hwnd, IDC_CHECK5, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.AlphaZWriteDisable = SendDlgItemMessage(hwnd, IDC_CHECK4, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.Wireframe = SendDlgItemMessage(hwnd, IDC_CHECK6, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.check7 = SendDlgItemMessage(hwnd, IDC_CHECK7, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	cfg.check8 = SendDlgItemMessage(hwnd, IDC_CHECK8, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
}

BOOL CALLBACK Configure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
	{
		SendDlgItemMessage(hwnd, IDC_FULL_SCREEN, BM_SETCHECK, GetCBState(cfg.fullScreen), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK2, BM_SETCHECK, GetCBState(cfg.ListSorting), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK3, BM_SETCHECK, GetCBState(cfg.AlphasubSort), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK4, BM_SETCHECK, GetCBState(cfg.AlphaZWriteDisable), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK5, BM_SETCHECK, GetCBState(cfg.PunchsubSort), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK6, BM_SETCHECK, GetCBState(cfg.Wireframe), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK7, BM_SETCHECK, GetCBState(cfg.check7), 0);
		SendDlgItemMessage(hwnd, IDC_CHECK8, BM_SETCHECK, GetCBState(cfg.check8), 0);

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			UpdateCfg(hwnd);
			EndDialog(hwnd, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_CHECK2:
		case IDC_CHECK3:
		case IDC_CHECK4:
		case IDC_CHECK5:
		case IDC_CHECK6:
		case IDC_CHECK7:
		case IDC_CHECK8:
			UpdateCfg(hwnd);
			break;
		}
		break;
	}

	return FALSE;
}


bool SetConfig() {
	CFG backUp;
	backUp = cfg;
	if (DialogBox(hinstance, MAKEINTRESOURCE(IDD_DIALOG), GetActiveWindow(), (DLGPROC)Configure) == IDOK) {
		SaveConfig();
		return true;
	}
	cfg = backUp;
	return false;
}


bool LoadConfig(bool autoSetConfig) {
	IniFile iniFile;

	memset(&cfg, 0, sizeof(CFG));
	if (!IniFile_open(&iniFile, GPU_MODULE_NAME)) return false;

	if (!IniFile_exist(&iniFile)) {
		if (!autoSetConfig)
			return true;
		MessageBox(GetActiveWindow(), "gpuDemul not configured", GPU_MODULE_NAME, MB_ICONINFORMATION);
		return SetConfig();
	}
	cfg.fullScreen = IniFile_getLong(&iniFile, "main", "FullScreen");
	cfg.ListSorting = IniFile_getLong(&iniFile, "main", "ListSorting");
	cfg.AlphasubSort = IniFile_getLong(&iniFile, "main", "AlphasubSort");
	cfg.AlphaZWriteDisable = IniFile_getLong(&iniFile, "main", "AlphaZWriteDisable");
	cfg.PunchsubSort = IniFile_getLong(&iniFile, "main", "PunchsubSort");
	cfg.Wireframe = IniFile_getLong(&iniFile, "main", "Wireframe");
	cfg.check7 = IniFile_getLong(&iniFile, "main", "check7");
	cfg.check8 = IniFile_getLong(&iniFile, "main", "check8");
	return true;
}

void SaveConfig() {
	IniFile iniFile;
	if (!IniFile_open(&iniFile, GPU_MODULE_NAME)) return;

	IniFile_setLong(&iniFile, "main", "FullScreen", cfg.fullScreen);
	IniFile_setLong(&iniFile, "main", "ListSorting", cfg.ListSorting);
	IniFile_setLong(&iniFile, "main", "AlphasubSort", cfg.AlphasubSort);
	IniFile_setLong(&iniFile, "main", "AlphaZWriteDisable", cfg.AlphaZWriteDisable);
	IniFile_setLong(&iniFile, "main", "PunchsubSort", cfg.PunchsubSort);
	IniFile_setLong(&iniFile, "main", "Wireframe", cfg.Wireframe);
	IniFile_setLong(&iniFile, "main", "check7", cfg.check7);
	IniFile_setLong(&iniFile, "main", "check8", cfg.check8);
}
