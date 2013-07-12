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
#include "stdio.h"

#include "inifile.h"

bool IniFile_open(IniFile* iniFile, char*name) {
	char *szTemp;
	int i;
	i = strlen(name);
	GetModuleFileName(GetModuleHandle(NULL), iniFile->szIniFile, 256);
	szTemp = strrchr(iniFile->szIniFile, '\\');

	if (szTemp == NULL) return false;
	szTemp++;
	strcpy(szTemp, name);
	szTemp += i;
	strcpy(szTemp, ".ini");
	return true;
}

long IniFile_getLong(IniFile* iniFile, char*sectionName, char*keyName) {
	GetPrivateProfileString(sectionName, keyName, NULL, iniFile->szValue, MAX_VALUE_SIZE, iniFile->szIniFile);
	return atol(iniFile->szValue);
}

void IniFile_setLong(IniFile* iniFile, char*sectionName, char*keyName, long value) {
	sprintf(iniFile->szValue, "%d", value);
	WritePrivateProfileString(sectionName, keyName, iniFile->szValue, iniFile->szIniFile);
}

void IniFile_getString(IniFile* iniFile, char*sectionName, char*keyName, char*value) {
	GetPrivateProfileString(sectionName, keyName, NULL, value, MAX_VALUE_SIZE, iniFile->szIniFile);
}

void IniFile_setString(IniFile* iniFile, char*sectionName, char*keyName, char*value) {
	WritePrivateProfileString(sectionName, keyName, value, iniFile->szIniFile);
}

bool IniFile_exist(IniFile* iniFile) {
	FILE *f;
	if ((f = fopen(iniFile->szIniFile, "rt")) == NULL)
		return false;
	fclose(f);
	return true;
}

void strUpCpy(char *dst, const char* src) {
	int i;

	i = strlen(src);
	dst[i--] = 0;
	while (i >= 0) {
		dst[i] = toupper(src[i]);
		i--;
	}
}
