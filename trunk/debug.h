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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"

void Printf(char *format, ...);
void PrintfLog(char *format, ...);

void ProfileStart(u16 cs);
void ProfileFinish(void);

void statFlush(void);
void statFinalize(void);
void statUpdate(u16 code);

#ifdef DEMUL_DEBUG

void openDebug(void);
void closeDebug(void);

u32 FASTCALL hookDebugExecute(u32 pc);
void FASTCALL hookDebugRead(u32 mem, u32 align);
void FASTCALL hookDebugWrite(u32 mem, u32 align);
void FASTCALL hookDebugException(void);

#endif

#endif