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

#ifndef __SPUDEVICE_H__
#define __SPUDEVICE_H__

#include "types.h"

#define SPU_MODULE_NAME "spuDemul"

extern u8   *ARAM;

bool OpenDevice();
void CloseDevice();

void DeviceSync();

#define ARAM8(x)    (*(u8*)&ARAM[(x) & 0x001fffff])
#define ARAM16(x)   (*(u16*)&ARAM[(x) & 0x001fffff])
#define ARAM32(x)   (*(u32*)&ARAM[(x) & 0x001fffff])


#endif