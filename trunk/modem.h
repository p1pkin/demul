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

#ifndef __MODEM_H__
#define __MODEM_H__

#define ModemId0 2	//USA
#define ModemId1 0	//SEGA 33.6

#include "types.h"

u32 FASTCALL ReadModem8(u32 mem);
void FASTCALL WriteModem8(u32 mem, u8 value);

#endif