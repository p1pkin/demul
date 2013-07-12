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

#ifndef __MAPLE_H__
#define __MAPLE_H__

#include "types.h"

#define FUNC_PURUPURU       0x00010000
#define FUNC_MOUSE          0x00020000
#define FUNC_CONTROLLER     0x01000000
#define FUNC_MEMCARD        0x02000000
#define FUNC_LCD            0x04000000
#define FUNC_CLOCK          0x08000000
#define FUNC_MICROPHONE     0x10000000
#define FUNC_ARGUN          0x20000000
#define FUNC_KEYBOARD       0x40000000
#define FUNC_LIGHTGUN       0x80000000

typedef struct {
	u8 response;
	u8 dst;
	u8 src;
	u8 lenght;
} RESPONSE;

typedef struct {
	u32 functions;
	u32 function_data[3];
	u8 area_code;
	u8 connector_direction;
	u8 product_name[30];
	u8 product_license[60];
	u16 standby_power;
	u16 max_power;
} DEVINFO;

typedef struct {
	u32 functions;
	u16 maxblk;
	u16 minblk;
	u16 infpos;
	u16 fatpos;
	u16 fatsz;
	u16 dirpos;
	u16 dirsz;
	u16 icon;
	u16 datasz;
	u16 rsvd[2];
} MEMINFO;

typedef struct {
	u32 functions;
	u32 blkno;
	u8 data[];
} READBLOCK;

typedef struct {
	u32 functions;
	u8 pt;
	u8 phase;
	u16 block[];
} WRITEBLOCK;

typedef struct {
	u32 functions;
	u16 joyButtons;
	u8 rightTrig;
	u8 leftTrig;
	u8 stick1X;
	u8 stick1Y;
	u8 stick2X;
	u8 stick2Y;
} CONTROLLER;

void ResetDevice(u8 *addr, u32 *frame);
void RequestDeviceInformation(u8 *addr, u32 *frame);
void Getcondition(u8 *addr, u32 *frame);
void GetMemoryInformation(u8 *addr, u32 *frame);
void BlockRead(u8 *addr, u32 *frame);
void BlockWrite(u8 *addr, u32 *frame);
void GetLastErorr(u8 *addr, u32 *frame);

void ReadVms(u8 *data, u32 addr, u32 vms);
void WriteVms(u8 *data, u32 addr, u32 count, u32 vms);

#endif