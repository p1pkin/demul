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

#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include <windows.h>
#include "types.h"

u32 listtype;
int completed;

void (FASTCALL *IRQ)(int irq);

void FASTCALL gpuTaTransfer(u32 *mem, u32 size);
void FASTCALL gpuYuvTransfer(u8 *src, u8 *dst);
void gpuBackground(u32 *mem);

#endif