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

#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdlib.h>
#include "types.h"
u32 FASTCALL CalcCrc32(const u8*aBuf, u32 aSize);
u32 FASTCALL CalcCrc32cont(u32 crc, const u8*aBuf, u32 aSize);
u32 FASTCALL CalcChecksum32(const u8 *aBuf, u32 aSize);
u32 FASTCALL CalcChecksum32cont(u32 crc, const u8 *aBuf, u32 aSize);

#endif