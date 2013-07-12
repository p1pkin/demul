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

#ifndef __DEMUL_H__
#define __DEMUL_H__

//#define DEMUL_DEBUG     // Allow Debugger.
//#define STAT_ENABLE

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "memory.h"
#include "sh4.h"
#include "disasm.h"
#include "console.h"
#include "debug.h"

int  OpenEmu();
void RunEmu(void *p);
void ResetEmu();
void CloseEmu();
void SaveSnapshot();

extern bool EmuRunning;

#endif
