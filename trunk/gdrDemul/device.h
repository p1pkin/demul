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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#define MAX_SECTOR 16

#include "types.h"

#define GDR_MODULE_NAME "gdrDemul"

typedef struct {
	UCHAR Reserved0;
	UCHAR Control : 4;
	UCHAR Adr     : 4;
	UCHAR TrackNumber;
	UCHAR Reserved1;
	UCHAR Address[4];
} TRACK;

typedef struct {
	u8 Length[2];
	u8 FirstTrack;
	u8 LastTrack;
	TRACK TrackData[100];
} TOC;

typedef struct {
	u8 SessionNumber;
	u8 Control : 4;
	u8 Adr     : 4;
	u8 Tno;
	u8 Point;
	u8 MsfExtra[3];
	u8 Zero;
	u8 Msf[3];
} TRACK_FULL;

typedef struct {
	u16 Length;
	u8 FirstCompleteSession;
	u8 LastCompleteSession;
	TRACK_FULL TrackData[100];
} TOC_FULL;

int  OpenDevice(char *dev);
u32  GerStatusDevice();
void ReadTOCDevice(u8 *buffef, u32 size);
void ReadInfoSessionDevice(u8 *buffer, u32 session, u32 count);
void ReadDevice(u8 *buffer, u32 size, u32 sector, u32 count, u32 flags);
void CloseDevice();

#endif