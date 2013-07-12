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
#ifndef __INIFILE_H__
#define __INIFILE_H__

#include "types.h"

#define MAX_VALUE_SIZE 256

typedef struct {
	char szIniFile[MAX_PATH];
	char szValue[MAX_VALUE_SIZE];
} IniFile;


bool IniFile_open(IniFile* iniFile, char*name);
long IniFile_getLong(IniFile* iniFile, char*sectionName, char*keyName);
void IniFile_setLong(IniFile* iniFile, char*sectionName, char*keyName, long value);
void IniFile_getString(IniFile* iniFile, char*sectionName, char*keyName, char*value);
void IniFile_setString(IniFile* iniFile, char*sectionName, char*keyName, char*value);

bool IniFile_exist(IniFile* iniFile);

void strUpCpy(char *dst, const char* src);

#endif