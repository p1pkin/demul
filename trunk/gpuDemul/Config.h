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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "types.h"
#include "gpuDemul.h"


typedef struct {
	long fullScreen;
	long ListSorting;
	long AlphasubSort;
	long AlphaZWriteDisable;
	long PunchsubSort;
	long Wireframe;
	long check7;
	long check8;
} CFG;

extern CFG cfg;


bool LoadConfig(bool autoSetConfig);
bool SetConfig();
void SaveConfig();
void CalcJoyCaps();

#endif
