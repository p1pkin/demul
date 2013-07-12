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

#include <string.h>
#include <stdio.h>
#include "crc32.h"
#include "zlib\zlib.h"
#include "gpuwnd.h"

u32 big_endian(u32 src) {
	return((src >> 24) | (src << 24) | ((src & 0x00FF0000) >> 8) | ((src & 0x0000FF00) << 8));
}

void fwritedata(FILE *file, u32 size, char *signature, u8 *data) {
	u32 crc = big_endian(CalcCrc32cont(CalcCrc32cont(0, (u8*)signature, 4), data, size));
	u32 datasize = big_endian(size);
	fwrite(&datasize, 1, 4, file);
	fwrite(signature, 1, 4, file);
	if (size) fwrite(data, 1, size, file);
	fwrite(&crc, 1, 4, file);
}

void SaveSnapshot(u8 *src, u32 source_size) {
	static u8 header[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	static u8 *copyright = "Created by DEMUL Dreamcast Emulator for Windows, Chanka must die! ;)";
	static u32 filenumber = 0;

	FILE *out_file = NULL;
	char *filename[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	u32 file_not_exists = 1;
	u32 compressed_size = (u32)(source_size * 1.001 + 12);
	u8 *compressed_data = malloc(compressed_size);
	u8 *tmp = malloc(source_size + height);
	u32 i = 0, j = 0, k = 0;

	while (i < ((width * height * 3) + height)) {
		if ((j % (width * 3)) == 0) {
			tmp[i++] = 00;
			k++;
			j = 0;
		}
		tmp[i++] = src[(height - k) * width * 3 + (j++)];
		tmp[i++] = src[(height - k) * width * 3 + (j++)];
		tmp[i++] = src[(height - k) * width * 3 + (j++)];
	}

	while (file_not_exists) {
		sprintf((char*)filename, "%08d.png", filenumber++);
		if (out_file = fopen((char*)filename, "rb"))
			fclose(out_file);
		else
			file_not_exists = 0;
	}

	if (out_file = fopen((char*)filename, "wb")) {
		u32 output_data[4] = { big_endian(width), big_endian(height), 0x00000208, 0 };
		fwrite(header, 1, 8, out_file);
		fwritedata(out_file, 13, "IHDR", (u8*)output_data);
		fwritedata(out_file, strlen(copyright), "tEXt", copyright);
		compress(compressed_data, &compressed_size, tmp, source_size + height);
		fwritedata(out_file, compressed_size, "IDAT", compressed_data);
		fwritedata(out_file, 0, "IEND", 0);
		fclose(out_file);
	}

	free(compressed_data);
}
