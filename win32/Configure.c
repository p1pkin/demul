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
#include "configure.h"
#include "resource.h"
#include "inifile.h"
#include "plugins.h"
#include "crc32.h"
#include "debug.h"
#include "demul.h"

typedef struct {
	char fileName[MAX_PATH];
	char upFileName[MAX_PATH];
	char name[MAX_PLUGIN_NAME + 1];
	int type;
	int comboIndex;
} PluginInfo;

typedef struct {
	char description[MAX_DESCRIPTION];
	u32 crc32;
	u32 size;
} DatInfo;

CFG cfg;
CFG tuneCfg;

DatInfo dats[] = {
	{ "Sega Dreamcast BIOS v1.004 (1998)(Sega)(Jp)", 0x5454841f, 2097152 },
	{ "Sega Dreamcast BIOS v1.01d (1998)(Sega)(Eu)", 0xa2564fad, 2097152 },
	{ "Sega Dreamcast BIOS v1.01d (1998)(Sega)(US)", 0x89f2b1a1, 2097152 },
	{ "Dreamcast Flash Rom (Eu-Pal)", 0xb7e5aeeb, 131072 },
	{ "Dreamcast Flash Rom (Eu-Pal)[a]", 0x00a8638b, 131072 },
	{ "Dreamcast Flash Rom (Jp-Ntsc)", 0x5f92bf76, 131072 },
	{ "Dreamcast Flash Rom (Jp-Ntsc)[a2]", 0x8af98e26, 131072 },
	{ "Dreamcast Flash Rom (Jp-Ntsc)[a]", 0x307a7035, 131072 },
	{ "Dreamcast Flash Rom (US-Ntsc)", 0xc611b498, 131072 }
};

PluginInfo *plugins = NULL;
int pluginsCount = 0;
u8 *biosFileBuf = NULL;

bool LoadConfig() {
	char pluginFileName[MAX_PATH];
	IniFile iniFile;

	memset(&cfg, 0, sizeof(CFG));
	if (!IniFile_open(&iniFile, DEMUL_NAME)) return false;

	if (!IniFile_exist(&iniFile)) {
		MessageBox(GetActiveWindow(), "BIOS & 插件未配置", "Demul", MB_ICONINFORMATION);
		return SetConfig();
	}

	cfg.sh4mode = IniFile_getLong(&iniFile, "main", "sh4mode");

#ifdef DEMUL_DEBUG
	memQuietMode(cfg.sh4mode);
#endif

	IniFile_getString(&iniFile, "plugins", "gdr", pluginFileName);
	strUpCpy(cfg.gdrPluginName, pluginFileName);
	IniFile_getString(&iniFile, "plugins", "gpu", pluginFileName);
	strUpCpy(cfg.gpuPluginName, pluginFileName);
	IniFile_getString(&iniFile, "plugins", "spu", pluginFileName);
	strUpCpy(cfg.spuPluginName, pluginFileName);
	IniFile_getString(&iniFile, "plugins", "pad", pluginFileName);
	strUpCpy(cfg.padPluginName, pluginFileName);
	IniFile_getString(&iniFile, "files", "bios", cfg.biosFileName);
	IniFile_getString(&iniFile, "files", "flash", cfg.flashFileName);

	return true;
}

void SaveConfig() {
	IniFile iniFile;
	if (!IniFile_open(&iniFile, DEMUL_NAME)) return;

	IniFile_setLong(&iniFile, "main", "sh4mode", cfg.sh4mode);
	IniFile_setString(&iniFile, "plugins", "gdr", cfg.gdrPluginName);
	IniFile_setString(&iniFile, "plugins", "gpu", cfg.gpuPluginName);
	IniFile_setString(&iniFile, "plugins", "spu", cfg.spuPluginName);
	IniFile_setString(&iniFile, "plugins", "pad", cfg.padPluginName);
	IniFile_setString(&iniFile, "files", "bios", cfg.biosFileName);
	IniFile_setString(&iniFile, "files", "flash", cfg.flashFileName);
}

void UpdateBIOSFilesInfo(HWND hwnd) {
	char *biosDescription;
	char *flashDescription;

	LoadBIOSFiles(&tuneCfg, biosFileBuf, biosFileBuf, &biosDescription, &flashDescription);
	SetDlgItemText(hwnd, IDC_BIOS_DIALOG_BIOS, tuneCfg.biosFileName);
	SetDlgItemText(hwnd, IDC_BIOS_DIALOG_FLASH, tuneCfg.flashFileName);
	SetDlgItemText(hwnd, IDC_BIOS_DIALOG_BIOS_DESCRIPTION, biosDescription);
	SetDlgItemText(hwnd, IDC_BIOS_DIALOG_FLASH_DESCRIPTION, flashDescription);
}

BOOL CALLBACK Configure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int i;
	int combo;
	int selectedGdr;
	int selectedGpu;
	int selectedSpu;
	int selectedPad;
	OPENFILENAME openFileName;
	char openFileBuf[MAX_PATH];

	switch (msg) {
	case WM_INITDIALOG:
	{
		if (pluginsCount != 0) {
			selectedPad = selectedGpu = selectedSpu = selectedGdr = -1;
			for (i = 0; i < pluginsCount; i++) {
				switch (plugins[i].type) {
				case PLUGIN_TYPE_GDR:
					combo = IDC_BIOS_DIALOG_GDR;
					if (strcmp(tuneCfg.gdrPluginName, plugins[i].upFileName) == 0)
						selectedGdr = i;
					break;
				case PLUGIN_TYPE_GPU:
					combo = IDC_BIOS_DIALOG_GPU;
					if (strcmp(tuneCfg.gpuPluginName, plugins[i].upFileName) == 0)
						selectedGpu = i;
					break;
				case PLUGIN_TYPE_SPU:
					combo = IDC_BIOS_DIALOG_SPU;
					if (strcmp(tuneCfg.spuPluginName, plugins[i].upFileName) == 0)
						selectedSpu = i;
					break;
				case PLUGIN_TYPE_PAD:
					combo = IDC_BIOS_DIALOG_PAD;
					if (strcmp(tuneCfg.padPluginName, plugins[i].upFileName) == 0)
						selectedPad = i;
					break;
				}
				plugins[i].comboIndex = SendDlgItemMessage(hwnd, combo, CB_ADDSTRING, 0, (LPARAM)plugins[i].name);
			}

			SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_GDR, CB_SETCURSEL, selectedGdr != -1 ? plugins[selectedGdr].comboIndex : 0, 0);
			SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_GPU, CB_SETCURSEL, selectedGpu != -1 ? plugins[selectedGpu].comboIndex : 0, 0);
			SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_SPU, CB_SETCURSEL, selectedSpu != -1 ? plugins[selectedSpu].comboIndex : 0, 0);
			SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_PAD, CB_SETCURSEL, selectedPad != -1 ? plugins[selectedPad].comboIndex : 0, 0);
		}
		UpdateBIOSFilesInfo(hwnd);
		return TRUE;
	}


	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BIOS_DIALOG_FLASH_SELECT:
		case IDC_BIOS_DIALOG_BIOS_SELECT:
			ZeroMemory(&openFileName, sizeof(openFileName));
			openFileName.lStructSize = sizeof(openFileName);
			openFileName.hwndOwner = hwnd;
			openFileName.lpstrFile = openFileBuf;
			openFileName.lpstrFile[0] = '\0';
			openFileName.nMaxFile = sizeof(openFileBuf);
			openFileName.lpstrFilter = "所有 (*.*)\0*.*\0Bios 文件\0dc_bios.bin\0Flash 文件\0dc_flash.bin\0";
			if (LOWORD(wParam) == IDC_BIOS_DIALOG_BIOS_SELECT)
				openFileName.nFilterIndex = 2;
			if (LOWORD(wParam) == IDC_BIOS_DIALOG_FLASH_SELECT)
				openFileName.nFilterIndex = 3;
			openFileName.lpstrFileTitle = NULL;
			openFileName.nMaxFileTitle = 0;
			openFileName.lpstrInitialDir = NULL;
			openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&openFileName) == TRUE) {
				if (LOWORD(wParam) == IDC_BIOS_DIALOG_BIOS_SELECT)
					strcpy(tuneCfg.biosFileName, openFileBuf);
				if (LOWORD(wParam) == IDC_BIOS_DIALOG_FLASH_SELECT)
					strcpy(tuneCfg.flashFileName, openFileBuf);

				UpdateBIOSFilesInfo(hwnd);
			}
			break;
		case IDOK:
			selectedGdr = SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_GDR, CB_GETCURSEL, 0, 0);
			selectedGpu = SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_GPU, CB_GETCURSEL, 0, 0);
			selectedSpu = SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_SPU, CB_GETCURSEL, 0, 0);
			selectedPad = SendDlgItemMessage(hwnd, IDC_BIOS_DIALOG_PAD, CB_GETCURSEL, 0, 0);
			tuneCfg.gdrPluginName[0] = 0;
			tuneCfg.gpuPluginName[0] = 0;
			tuneCfg.spuPluginName[0] = 0;
			tuneCfg.padPluginName[0] = 0;
			for (i = 0; i < pluginsCount; i++) {
				switch (plugins[i].type) {
				case PLUGIN_TYPE_GDR:
					if (plugins[i].comboIndex == selectedGdr)
						strcpy(tuneCfg.gdrPluginName, plugins[i].upFileName);
					break;
				case PLUGIN_TYPE_GPU:
					if (plugins[i].comboIndex == selectedGpu)
						strcpy(tuneCfg.gpuPluginName, plugins[i].upFileName);
					break;
				case PLUGIN_TYPE_SPU:
					if (plugins[i].comboIndex == selectedSpu)
						strcpy(tuneCfg.spuPluginName, plugins[i].upFileName);
					break;
				case PLUGIN_TYPE_PAD:
					if (plugins[i].comboIndex == selectedPad)
						strcpy(tuneCfg.padPluginName, plugins[i].upFileName);
					break;
				}
			}

			EndDialog(hwnd, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

HMODULE SafeLoadLibrary(char *name) {
	HMODULE result;
	UINT oldMode;
	oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	result = LoadLibraryEx(name, 0, 0);

	SetErrorMode(oldMode);
	return result;
}

void EnumPlugins() {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	HMODULE hModule;
	_getType getType = NULL;
	_getName getName = NULL;
	PluginInfo *cur;
	char *szTemp;
	char searchPath[MAX_PATH];

	free(plugins);
	plugins = NULL;
	pluginsCount = 0;


	GetModuleFileName(GetModuleHandle(NULL), searchPath, sizeof(searchPath));
	szTemp = strrchr(searchPath, '\\');
	if (szTemp == NULL) return;
	szTemp++;
	strcpy(szTemp, "*.*");

	hFind = FindFirstFile(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) continue;
		hModule = SafeLoadLibrary(findFileData.cFileName);
		if (hModule == NULL) continue;
		getType = (_getType)GetProcAddress(hModule, "getType");
		getName = (_getName)GetProcAddress(hModule, "getName");
		if ((getType == NULL) || (getName == NULL)) {
			FreeLibrary(hModule);
			continue;
		}


		pluginsCount++;
		plugins = realloc(plugins, pluginsCount * sizeof(*plugins));
		cur = &plugins[pluginsCount - 1];
		strncpy(cur->name, getName(), MAX_PLUGIN_NAME);
		cur->name[MAX_PLUGIN_NAME] = 0;
		cur->type = getType();

		strcpy(cur->fileName, findFileData.cFileName);
		strUpCpy(cur->upFileName, cur->fileName);
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

bool SetConfig() {
	tuneCfg = cfg;
	EnumPlugins();
	biosFileBuf = (u8*)malloc(BIOS_SIZE);
	if (DialogBox(demulInfo.hMainInstance, MAKEINTRESOURCE(IDD_BIOS_AND_PLUGINS), GetActiveWindow(), (DLGPROC)Configure) == IDOK) {
		free(biosFileBuf);
		cfg = tuneCfg;
		SaveConfig();
		return true;
	}
	free(biosFileBuf);
	return false;
}

bool LoadBIOSFiles(CFG* aCfg, u8*biosBuf, u8*flashBuf, char** biosDescription, char** flashDescription) {
	static char unableToRead[MAX_DESCRIPTION] = "无法读取文件";
	static char unknownFile[MAX_DESCRIPTION] = "未知文件";
	static char goodSize[MAX_DESCRIPTION] = "好的大小";
	FILE *f;
	u32 i;
	u32 crc32;
	bool goodBios = false;
	bool goodFlash = false;
	*biosDescription = *flashDescription = unableToRead;


	f = fopen(aCfg->biosFileName, "rb");
	if (f != NULL) {
		if (fread(biosBuf, BIOS_SIZE, 1, f) == 1) {
			*biosDescription = unknownFile;
			crc32 = CalcCrc32(biosBuf, BIOS_SIZE);
			for (i = 0; i < ARRAYSIZE(dats); i++)
				if ((dats[i].crc32 == crc32) && (dats[i].size == BIOS_SIZE)) {
					*biosDescription = dats[i].description;
					goodBios = true;;
					break;
				}
		}
		fclose(f);
	}

	f = fopen(aCfg->flashFileName, "rb");
	if (f != NULL) {
		if (fread(flashBuf, FLASH_SIZE, 1, f) == 1) {
			*flashDescription = goodSize;
			goodFlash = true;	//even bad content be good
			crc32 = CalcCrc32(flashBuf, FLASH_SIZE);
			for (i = 0; i < ARRAYSIZE(dats); i++)
				if ((dats[i].crc32 == crc32) && (dats[i].size == FLASH_SIZE)) {
					*flashDescription = dats[i].description;
					break;
				}
		}
		fclose(f);
	}

	return goodFlash && goodBios;
}
