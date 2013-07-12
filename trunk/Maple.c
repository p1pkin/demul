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

#include <stdio.h>
#include <string.h>
#include "maple.h"
#include "plugins.h"

static u8 product_name[30] =
{
	0x44, 0x72, 0x65, 0x61, 0x6D, 0x63, 0x61, 0x73, 0x74, 0x20,
	0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static u8 product_name_vms[30] =
{
	0x56, 0x49, 0x53, 0x55, 0x41, 0x4C, 0x20, 0x4D, 0x45, 0x4D,
	0x4F, 0x52, 0x59, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static u8 product_license[60] =
{
	0x50, 0x72, 0x6F, 0x64, 0x75, 0x63, 0x65, 0x64, 0x20, 0x42,
	0x79, 0x20, 0x6F, 0x72, 0x20, 0x55, 0x6E, 0x64, 0x65, 0x72,
	0x20, 0x4C, 0x69, 0x63, 0x65, 0x6E, 0x73, 0x65, 0x20, 0x46,
	0x72, 0x6F, 0x6D, 0x20, 0x53, 0x45, 0x47, 0x41, 0x20, 0x45,
	0x4E, 0x54, 0x45, 0x52, 0x50, 0x52, 0x49, 0x53, 0x45, 0x53,
	0x2C, 0x4C, 0x54, 0x44, 0x2E, 0x20, 0x20, 0x20, 0x20, 0x20
};

void ResetDevice(u8 *addr, u32 *frame) {
	RESPONSE    *response = (RESPONSE*)addr;

	response->response = 0x07;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = 0;
}

void RequestDeviceInformation(u8 *addr, u32 *frame) {
	RESPONSE    *response = (RESPONSE*)addr;
	DEVINFO     *devinfo = (DEVINFO*)(addr + 4);

	memset(devinfo, 0, sizeof(DEVINFO));

	if (*frame & 0x2000) {
		frame[0] |= 0x300;

		devinfo->functions = FUNC_CONTROLLER;
		devinfo->function_data[0] = padJoyCaps((frame[0] >> 14) & 3);
		devinfo->area_code = 0xff;
		memcpy(&devinfo->product_name, &product_name, 30);
		memcpy(&devinfo->product_license, &product_license, 60);
		devinfo->standby_power = 0xae01;
		devinfo->max_power = 0xf401;
	} else {
		devinfo->functions = FUNC_MEMCARD;
		devinfo->function_data[0] = 0x00410f00;
		devinfo->area_code = 0xff;
		memcpy(&devinfo->product_name, &product_name_vms, 30);
		memcpy(&devinfo->product_license, &product_license, 60);
		devinfo->standby_power = 0x7c00;
		devinfo->max_power = 0x8200;
	}

	response->response = 0x05;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = sizeof(DEVINFO) / 4;
}

void Getcondition(u8 *addr, u32 *frame) {
	RESPONSE    *response = (RESPONSE*)addr;
	CONTROLLER  *controller = (CONTROLLER*)(addr + 4);

	memset(controller, 0, sizeof(CONTROLLER));

	frame[0] |= 0x300;

	response->response = 0x08;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = sizeof(CONTROLLER) / 4;

	controller->functions = FUNC_CONTROLLER;
	padReadJoy((u8*)&controller->joyButtons, (frame[0] >> 14) & 3);
}

void GetMemoryInformation(u8 *addr, u32 *frame) {
	RESPONSE *response = (RESPONSE*)addr;
	MEMINFO  *meminfo = (MEMINFO*)(addr + 4);

	response->response = 0x08;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = sizeof(MEMINFO) / 4;

	meminfo->functions = FUNC_MEMCARD;
	meminfo->maxblk = 0x00ff;
	meminfo->minblk = 0x0000;
	meminfo->infpos = 0x00ff;
	meminfo->fatpos = 0x00fe;
	meminfo->fatsz = 0x0001;
	meminfo->dirpos = 0x00fd;
	meminfo->dirsz = 0x000d;
	meminfo->icon = 0x0000;
	meminfo->datasz = 0x00c8;
	meminfo->rsvd[0] = 0x001f;
	meminfo->rsvd[1] = 0x0000;
}

void BlockRead(u8 *addr, u32 *frame) {
	RESPONSE *response = (RESPONSE*)addr;
	READBLOCK *readblock = (READBLOCK*)(addr + 4);

	response->response = 0x08;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = sizeof(READBLOCK) / 4;

	readblock->functions = FUNC_MEMCARD;
	readblock->blkno = frame[2];

	ReadVms(
		(u8*)&readblock->data,
		(((frame[2] >> 24) & 0xff) | ((frame[2] >> 8) & 0xff00)) * 512,
		(frame[0] >> 8) & 0xff
		);
}

void BlockWrite(u8 *addr, u32 *frame) {
	RESPONSE *response = (RESPONSE*)addr;

	response->response = 0x07;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = 0;

	WriteVms(
		(u8*)&frame[3],
		(((frame[2] >> 22) & 0x3fe) + ((frame[2] >> 8) & 0xff)) * 128,
		(((frame[0] >> 24) & 0xff) - 2) * 4,
		(frame[0] >> 8) & 0xff
		);
}

void GetLastErorr(u8 *addr, u32 *frame) {
	RESPONSE *response = (RESPONSE*)addr;

	response->response = 0x07;
	response->dst = (frame[0] >> 16) & 0xff;
	response->src = (frame[0] >> 8) & 0xff;
	response->lenght = 0;
}

void ReadVms(u8 *data, u32 addr, u32 vms) {
	FILE *f;
	u32 seek = addr + (((((vms >> 6) & 3)) * (((vms >> 0) & 3))) * 1024 * 128);

	if ((f = fopen("VMS.BIN", "r+b")) == NULL) {
		int i;

		if ((f = fopen("VMS.BIN", "w+b")) == NULL)
			return;

		for (i = 0; i < 1024 * 128 * 8; i++) fputc(0, f);
	}

	fseek(f, seek, SEEK_SET);
	fread((void*)data, 512, 1, f);
	fclose(f);
}

void WriteVms(u8 *data, u32 addr, u32 count, u32 vms) {
	FILE *f;
	u32 seek = addr + (((((vms >> 6) & 3)) * (((vms >> 0) & 3))) * 1024 * 128);

	if ((f = fopen("VMS.BIN", "r+b")) == NULL) {
		int i;

		if ((f = fopen("VMS.BIN", "w+b")) == NULL)
			return;

		for (i = 0; i < 1024 * 128 * 8; i++) fputc(0, f);
	}

	fseek(f, seek, SEEK_SET);
	fwrite((void*)data, count, 1, f);
	fclose(f);
}