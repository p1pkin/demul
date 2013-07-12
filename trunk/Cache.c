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

#include "cache.h"
#include "onchip.h"

void FASTCALL WriteSQ32(u32 mem, u32 value) {
	SQ0[(mem >> 2) & 15] = value;
}

void FASTCALL WriteSQ64(u32 mem, u64 *value) {
	*(u64*)&SQ0[(mem >> 2) & 14] = *value;
}

u8 FASTCALL ReadCacheRam8(u32 mem) {
	u32 value = 0;

	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			value = *(u8*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)];
		} else {
			value = *(u8*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)];
		}
	}

	return value;
}

u16 FASTCALL ReadCacheRam16(u32 mem) {
	u32 value = 0;

	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			value = *(u16*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)];
		} else {
			value = *(u16*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)];
		}
	}

	return value;
}

u32 FASTCALL ReadCacheRam32(u32 mem) {
	u32 value = 0;

	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			value = *(u32*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)];
		} else {
			value = *(u32*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)];
		}
	}

	return value;
}

void FASTCALL ReadCacheRam64(u32 mem, u64 *out) {
	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			*out = *(u64*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)];
			return;
		} else {
			*out = *(u64*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)];
			return;
		}
	}

	*out = 0;
}

void FASTCALL WriteCacheRam8(u32 mem, u8 value) {
	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			*(u8*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)] = value;
		} else {
			*(u8*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)] = value;
		}
	}
}

void FASTCALL WriteCacheRam16(u32 mem, u16 value) {
	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			*(u16*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)] = value;
		} else {
			*(u16*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)] = value;
		}
	}
}

void FASTCALL WriteCacheRam32(u32 mem, u32 value) {
	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			*(u32*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)] = value;
		} else {
			*(u32*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)] = value;
		}
	}
}

void FASTCALL WriteCacheRam64(u32 mem, u64 *value) {
	if (pOnChip[0x001c] & 0x20) {
		if (pOnChip[0x001c] & 0x80) {
			*(u64*)&pCram[((mem >> 13) & 0x1000) + (mem & 0x0fff)] = *value;
		} else {
			*(u64*)&pCram[((mem >> 1) & 0x1000) + (mem & 0x0fff)] = *value;
		}
	}
}