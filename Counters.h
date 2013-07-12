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

#ifndef __COUNTERS_H__
#define __COUNTERS_H__

#include "types.h"

#define BIAS            ((int)4)
#define DCCLK           ((int)200000000)
#define DCHBLANK_NTSC   ((int)(DCCLK / 30 / 525))
#define DCHBLANK_PAL    ((int)(DCCLK / 25 / 625))
#define ARM7CLK         ((int)25000000)

u32 countcycle;

void CpuTest();
int  openCounters();
void closeCounters();

#endif
