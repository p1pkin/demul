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

#include <windows.h>
#include <devioctl.h>
#include <ntddcdrm.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <string.h>
#include "device.h"

HANDLE hDev = INVALID_HANDLE_VALUE;

u32 sessions[100];

typedef struct {
	u32 entry[99];
	u32 first;
	u32 last;
	u32 leadout;
} GDR_TOC;

GDR_TOC GD_TOC;

s32 INLINE msf_to_lba(u8 m, u8 s, u8 f) {
	u32 lsn;
	lsn = f;
	lsn += (s - 2) * 75;
	lsn += m * 75 * 60;
	return lsn;
}

void INLINE lba_to_msf(s32 lba, u8* m, u8* s, u8* f) {
	lba += 150;
	*m = lba / (60 * 75);
	*s = (lba / 75) % 60;
	*f = lba % 75;
}

void ReadInfoDevice() {
	TOC Toc;
	TOC_FULL TocFull;
	SCSI_PASS_THROUGH_DIRECT srb;
	DWORD BytesReturned;

	u32 i;
	u32 track;
	u32 value;
	u32 nSesion;

	memset(&Toc, 0, sizeof(TOC));
	memset(&TocFull, 0, sizeof(TOC_FULL));

	srb.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	srb.ScsiStatus = 0;
	srb.PathId = 0;
	srb.TargetId = 0;
	srb.Lun = 0;
	srb.CdbLength = 12;
	srb.SenseInfoLength = 0;
	srb.DataIn = SCSI_IOCTL_DATA_IN;
	srb.DataTransferLength = sizeof(TOC);
	srb.TimeOutValue = 60;
	srb.DataBuffer = &Toc;
	srb.SenseInfoOffset = 0;

	srb.Cdb[ 0] = 0x43;
	srb.Cdb[ 1] = 0x02;
	srb.Cdb[ 2] = 0x00;
	srb.Cdb[ 3] = 0x00;
	srb.Cdb[ 4] = 0x00;
	srb.Cdb[ 5] = 0x00;
	srb.Cdb[ 6] = 0x00;
	srb.Cdb[ 7] = 0x03;
	srb.Cdb[ 8] = 0x24;
	srb.Cdb[ 9] = 0x00;
	srb.Cdb[10] = 0x00;
	srb.Cdb[11] = 0x00;
	srb.Cdb[12] = 0x00;
	srb.Cdb[13] = 0x00;
	srb.Cdb[14] = 0x00;
	srb.Cdb[15] = 0x00;

	DeviceIoControl(
		hDev,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&srb,
		sizeof(SCSI_PASS_THROUGH_DIRECT),
		NULL,
		0,
		&BytesReturned,
		NULL
		);

	srb.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	srb.ScsiStatus = 0;
	srb.PathId = 0;
	srb.TargetId = 0;
	srb.Lun = 0;
	srb.CdbLength = 12;
	srb.SenseInfoLength = 0;
	srb.DataIn = SCSI_IOCTL_DATA_IN;
	srb.DataTransferLength = sizeof(TOC_FULL);
	srb.TimeOutValue = 60;
	srb.DataBuffer = &TocFull;
	srb.SenseInfoOffset = 0;

	srb.Cdb[ 0] = 0x43;
	srb.Cdb[ 1] = 0x02;
	srb.Cdb[ 2] = 0x00;
	srb.Cdb[ 3] = 0x00;
	srb.Cdb[ 4] = 0x00;
	srb.Cdb[ 5] = 0x00;
	srb.Cdb[ 6] = 0x00;
	srb.Cdb[ 7] = 0x04;
	srb.Cdb[ 8] = 0x50;
	srb.Cdb[ 9] = 0x80;
	srb.Cdb[10] = 0x00;
	srb.Cdb[11] = 0x00;
	srb.Cdb[12] = 0x00;
	srb.Cdb[13] = 0x00;
	srb.Cdb[14] = 0x00;
	srb.Cdb[15] = 0x00;

	DeviceIoControl(
		hDev,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&srb,
		sizeof(SCSI_PASS_THROUGH_DIRECT),
		NULL,
		0,
		&BytesReturned,
		NULL
		);

	memset(&GD_TOC, 0xffffffff, sizeof(GD_TOC));

	track = Toc.FirstTrack - 1;
	GD_TOC.first = (Toc.FirstTrack << 8) | ((u32)Toc.TrackData[track].Control << 4) | ((u32)Toc.TrackData[track].Adr << 0);

	track = Toc.LastTrack - 1;
	GD_TOC.last = (Toc.LastTrack << 8) | ((u32)Toc.TrackData[track].Control << 4) | ((u32)Toc.TrackData[track].Adr << 0);

	track = Toc.LastTrack - Toc.FirstTrack + 1;

	for (i = 0; i <= track; i++) {
		value = msf_to_lba(
			Toc.TrackData[i].Address[1],
			Toc.TrackData[i].Address[2],
			Toc.TrackData[i].Address[3]
			) + 150;

		value = ((value << 24) & 0xff000000) |
				((value << 8) & 0x00ff0000) |
				((value >> 8) & 0x0000ff00) |
				((value >> 24) & 0x000000ff);

		if (Toc.TrackData[i].TrackNumber == 0xaa) {
			GD_TOC.leadout = value | ((u32)Toc.TrackData[i].Control << 4) | ((u32)Toc.TrackData[i].Adr << 0);
		} else {
			GD_TOC.entry[i] = value | ((u32)Toc.TrackData[i].Control << 4) | ((u32)Toc.TrackData[i].Adr << 0);
		}
	}

	value = msf_to_lba(
		Toc.TrackData[track].Address[1],
		Toc.TrackData[track].Address[2],
		Toc.TrackData[track].Address[3]
		) + 150;

	value = ((value << 24) & 0xff000000) |
			((value << 8) & 0x00ff0000) |
			((value >> 8) & 0x0000ff00) |
			((value >> 24) & 0x000000ff);

	sessions[0] = value | (TocFull.LastCompleteSession - TocFull.FirstCompleteSession + 1);

	for (i = 0, nSesion = 1;; i++) {
		if (TocFull.TrackData[i].Adr == 0) break;

		if ((TocFull.TrackData[i].Point & 0x80) == 0x00) {
			if (nSesion == TocFull.TrackData[i].SessionNumber) {
				value = msf_to_lba(
					TocFull.TrackData[i].Msf[0],
					TocFull.TrackData[i].Msf[1],
					TocFull.TrackData[i].Msf[2]
					) + 150;

				value = ((value << 24) & 0xff000000) |
						((value << 8) & 0x00ff0000) |
						((value >> 8) & 0x0000ff00) |
						((value >> 24) & 0x000000ff);

				sessions[nSesion] = value | TocFull.TrackData[i].Point;
				nSesion++;
			}
		}
	}
}

int OpenDevice(char *dev) {
	char buf[256];

	if ((GetDriveType(dev) != DRIVE_CDROM))
		return -1;

	sprintf(buf, "\\\\.\\%s", dev);

	if ((hDev = CreateFile(
			 buf,
			 GENERIC_READ | GENERIC_WRITE,
			 FILE_SHARE_READ | FILE_SHARE_WRITE,
			 NULL,
			 OPEN_EXISTING,
			 0,
			 NULL
			 )) == INVALID_HANDLE_VALUE) return 0;
	return 1;
}

void ReadTOCDevice(u8 *buffef, u32 size) {
	ReadInfoDevice();
	memcpy(buffef, &GD_TOC, sizeof(GDR_TOC) < size ? sizeof(GDR_TOC) : size);
}

void ReadInfoSessionDevice(u8 *buffer, u32 session, u32 size) {
	ReadInfoDevice();
	buffer[0] = 1;
	buffer[1] = 0;
	*(u32*)&buffer[2] = sessions[session];
}

u32 GerStatusDevice() {
	if (hDev == INVALID_HANDLE_VALUE) return 0x6;

	return 0x22;
}

void ReadDevice(u8 *buffer, u32 size, u32 sector, u32 count, u32 flags) {
	SCSI_PASS_THROUGH_DIRECT srb;
	DWORD BytesReturned;

	srb.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	srb.ScsiStatus = 0;
	srb.PathId = 0;
	srb.TargetId = 0;
	srb.Lun = 0;
	srb.CdbLength = 12;
	srb.SenseInfoLength = 0;
	srb.DataIn = SCSI_IOCTL_DATA_IN;
	srb.DataTransferLength = size;
	srb.TimeOutValue = 60;
	srb.DataBuffer = buffer;
	srb.SenseInfoOffset = 0;

	srb.Cdb[ 0] = 0xbe;
	srb.Cdb[ 1] = 0x00;

	sector -= 150;
	srb.Cdb[ 2] = ((sector >> 24) & 0xff);
	srb.Cdb[ 3] = ((sector >> 16) & 0xff);
	srb.Cdb[ 4] = ((sector >> 8) & 0xff);
	srb.Cdb[ 5] = ((sector >> 0) & 0xff);

	srb.Cdb[ 6] = ((count >> 16) & 0xff);
	srb.Cdb[ 7] = ((count >> 8) & 0xff);
	srb.Cdb[ 8] = ((count >> 0) & 0xff);

	srb.Cdb[ 9] = flags;
	srb.Cdb[10] = 0x00;
	srb.Cdb[11] = 0x00;

	srb.Cdb[12] = 0x00;
	srb.Cdb[13] = 0x00;
	srb.Cdb[14] = 0x00;
	srb.Cdb[15] = 0x00;

	DeviceIoControl(
		hDev,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&srb,
		sizeof(SCSI_PASS_THROUGH_DIRECT),
		NULL,
		0,
		&BytesReturned,
		NULL
		);
}

void CloseDevice() {
	if (hDev != INVALID_HANDLE_VALUE)
		CloseHandle(hDev);
}