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

#ifndef __CPUID_H__
#define __CPUID_H__

#include "types.h"

void cpuid();

u32 cpuMMX;
u32 cpu3DNOW;
u32 cpuSSE;
u32 cpuSSE2;
u32 cpuSSE3;

#endif