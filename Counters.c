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
#include "sh4.h"
#include "onchip.h"
#include "asic.h"
#include "aica.h"
#include "gdrom.h"
#include "arm.h"
#include "counters.h"
#include "gpu.h"
#include "plugins.h"
#include "console.h"

void HBLANK(u32 cycle) {
	static u32 hcount = 0;

	hcount += cycle;

	if (hcount > (DCHBLANK_NTSC / BIAS)) {
		u32 count;

		hcount -= (DCHBLANK_NTSC / BIAS);
		count = ASIC32(0x810c) & 0x3ff;

		ASIC32(0x810c) &= ~0x2000;

		if (count == ((ASIC32(0x80cc) >> 16) & 0x3ff)) hwInt(0x0004);
		if (count == ((ASIC32(0x80cc) >> 0) & 0x3ff)) hwInt(0x0003);

		if (count == ((ASIC32(0x80f0) >> 0) & 0x3ff)) {
			ASIC32(0x810c) |= 0x2000;
			hwInt(0x0005);
		}

		count++;

		if (count > 524) {
			gpuSync();
			count = 0;
		}

		ASIC32(0x810c) &= ~0x3ff;
		ASIC32(0x810c) |= count;
	}
}

void TMU(u32 cycle) {
	static u16 div[8] = { 4, 16, 64, 256, 1024, 0, 0, 0 };

	if (TSTR & 1) {
		static u32 remainder = 0;
		u32 tcount = TCNT0;

		TCNT0 -= (cycle + remainder) / (div[TCR0 & 7] + BIAS);
		remainder = (cycle + remainder) % (div[TCR0 & 7] + BIAS);

		if ((s32)tcount < (s32)TCNT0) {
			TCR0 |= TUNF;
			TCNT0 = TCOR0;
		}
	}

	if (TSTR & 2) {
		static u32 remainder = 0;
		u32 tcount = TCNT1;

		TCNT1 -= (cycle + remainder) / (div[TCR1 & 7] + BIAS);
		remainder = (cycle + remainder) % (div[TCR1 & 7] + BIAS);

		if ((s32)tcount < (s32)TCNT1) {
			TCR1 |= TUNF;
			TCNT1 = TCOR1;
		}
	}

	if (TSTR & 4) {
		static u32 remainder = 0;
		u32 tcount = TCNT2;

		TCNT2 -= (cycle + remainder) / (div[TCR2 & 7] + BIAS);
		remainder = (cycle + remainder) % (div[TCR2 & 7] + BIAS);

		if ((s32)tcount < (s32)TCNT2) {
			TCR2 |= TUNF;
			TCNT2 = TCOR2;
		}
	}
}

void AICA(u32 cycle) {
	int i;
/*	static u32 carry   = 0;
	static u32 samples = 0;

	u8 count = (cycle - carry) / (DCCLK / (64 * 44100));
	carry    = (cycle - carry) % (DCCLK / (64 * 44100));

	samples += count;
	if(samples >= 128)
	{
		spuSync(samples);
		samples = 0;
	}*/

	for (i = 0; i < 3; i++) {
		u32 tcount = AICA32_UNMASKED(0x2890 + (i * 4)) + count;

		if (tcount > 0x100) {
			tcount &= 0xff;
			AICA32(0x28a0) |= (0x40 << i);
			AICA32(0x28b8) |= (0x40 << i);
		}

		AICA32_UNMASKED(0x2890 + (i * 4)) = tcount;
	}

	if ((AICA8(0x2c00) & 1) == 0)
		armExecute(cycle / (DCCLK / ARM7CLK));

	if (AICA32(0x28a0) & AICA32(0x289c))
		armFIQ();

	if (AICA32(0x28b8) & AICA32(0x28b4))
		hwInt(0x0101);
}

void GDRCHECK(u32 cycle) {
	if (gdrom.busy & 1) {
		static u32 check = 0;

		check += cycle;

		if (check > 0x1000) {
			check = 0;
			gdrom.busy = 0x50;
			hwInt(0x0100);
		}
	}
}

void AsicInterrupt() {
	if (!(sh4.sr.bl)) {
		if
		(
			(ASIC32(0x6900) & ASIC32(0x6930)) ||
			(ASIC32(0x6904) & ASIC32(0x6934)) ||
			(ASIC32(0x6908) & ASIC32(0x6938))
		) {
			if (sh4.sr.imask < 6) {
				ONCHIP32(0x0028) = 0x320;

				if (!(sh4.sr.rb)) {
					int i;

					for (i = 0; i < 8; i++) {
						u32 value = sh4.r[ 16 + i ];
						sh4.r[ 16 + i ] = sh4.r[  0 + i ];
						sh4.r[  0 + i ] = value;
					}
				}

				sh4.ssr = sh4.sr.all;
				sh4.sr.all = 0x70000000;
				sh4.spc = sh4.pc;
				sh4.pc = sh4.vbr + 0x600;

//				sh4.sgr	= sh4.cpu.r[15].UL;
				return;
			}
		}

		if
		(
			(ASIC32(0x6900) & ASIC32(0x6920)) ||
			(ASIC32(0x6904) & ASIC32(0x6924)) ||
			(ASIC32(0x6908) & ASIC32(0x6928))
		) {
			if (sh4.sr.imask < 4) {
				ONCHIP32(0x0028) = 0x360;

				if (!(sh4.sr.rb)) {
					int i;

					for (i = 0; i < 8; i++) {
						u32 value = sh4.r[ 16 + i ];
						sh4.r[ 16 + i ] = sh4.r[  0 + i ];
						sh4.r[  0 + i ] = value;
					}
				}

				sh4.ssr = sh4.sr.all;
				sh4.sr.all = 0x70000000;
				sh4.spc = sh4.pc;
				sh4.pc = sh4.vbr + 0x600;
//				sh4.sgr	= sh4.cpu.r[15].UL;
				return;
			}
		}

		if
		(
			(ASIC32(0x6900) & ASIC32(0x6910)) ||
			(ASIC32(0x6904) & ASIC32(0x6914)) ||
			(ASIC32(0x6908) & ASIC32(0x6918))
		) {
			if (sh4.sr.imask < 2) {
				ONCHIP32(0x0028) = 0x3a0;

				if (!(sh4.sr.rb)) {
					int i;

					for (i = 0; i < 8; i++) {
						u32 value = sh4.r[ 16 + i ];
						sh4.r[ 16 + i ] = sh4.r[  0 + i ];
						sh4.r[  0 + i ] = value;
					}
				}

				sh4.ssr = sh4.sr.all;
				sh4.sr.all = 0x70000000;
				sh4.spc = sh4.pc;
				sh4.pc = sh4.vbr + 0x600;
//				sh4.sgr	= sh4.cpu.r[15].UL;
				return;
			}
		}
	}
}

void CpuTest() {
	HBLANK(sh4.cycle);
	TMU(sh4.cycle);
	AICA(sh4.cycle);
	GDRCHECK(sh4.cycle);
	AsicInterrupt();
	sh4.cycle = 0;
}
