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
#include <math.h>

#include "demul.h"
#include "interpreter.h"
#include "counters.h"
#include "onchip.h"
#include "cache.h"
#include "mmu.h"
#include "plugins.h"

static bool NEED_INTERRUPTS_TEST;
static s32 iount;
static u32 fpucws;

static void INLINE SAVE_ROUNDING_MODE() {
	u32 fpucw;

	__asm fnstcw dword ptr fpucws
	__asm mov eax, dword ptr sh4.fpscr.all
	__asm and eax, 3
	__asm neg eax
	__asm sbb eax, eax
	__asm and eax, 0x00000c00
	__asm add eax, 0x0000007f
	__asm mov dword ptr fpucw, eax
	__asm fldcw dword ptr fpucw
}

static void INLINE RESTORE_ROUNDING_MODE() {
	__asm fldcw dword ptr fpucws
}


static void INLINE FASTCALL NI(u32 code) {
	#ifdef DEMUL_DEBUG
	hookDebugException();
	#endif
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVI(u32 code) {
	u32 i = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = (u32)(s32)(s8)i;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWI(u32 code) {
	u32 d = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead16(sh4.pc + (d << 1) + 4);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLI(u32 code) {
	u32 d = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead32((sh4.pc & 0xfffffffc) + (d << 2) + 4);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOV(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite8(sh4.r[n], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite16(sh4.r[n], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite32(sh4.r[n], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead8(sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead16(sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead32(sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite8(sh4.r[n] - 1, sh4.r[m]);
	sh4.r[n] -= 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite16(sh4.r[n] - 2, sh4.r[m]);
	sh4.r[n] -= 2;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite32(sh4.r[n] - 4, sh4.r[m]);
	sh4.r[n] -= 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead8(sh4.r[m]);
	if (n != m) sh4.r[m] += 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead16(sh4.r[m]);
	if (n != m) sh4.r[m] += 2;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead32(sh4.r[m]);
	if (n != m) sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 n = ((code >> 4) & 0x0f);

	memWrite8(sh4.r[n] + (d << 0), sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 n = ((code >> 4) & 0x0f);

	memWrite16(sh4.r[n] + (d << 1), sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite32(sh4.r[n] + (d << 2), sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);

	sh4.r[0] = memRead8(sh4.r[m] + (d << 0));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);

	sh4.r[0] = memRead16(sh4.r[m] + (d << 1));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead32(sh4.r[m] + (d << 2));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite8(sh4.r[n] + sh4.r[0], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite16(sh4.r[n] + sh4.r[0], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	memWrite32(sh4.r[n] + sh4.r[0], sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead8(sh4.r[m] + sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead16(sh4.r[m] + sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = memRead32(sh4.r[m] + sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	memWrite8(sh4.gbr + (d << 0), sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	memWrite16(sh4.gbr + (d << 1), sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	memWrite32(sh4.gbr + (d << 2), sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVBLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	sh4.r[0] = memRead8(sh4.gbr + (d << 0));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVWLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	sh4.r[0] = memRead16(sh4.gbr + (d << 1));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVLLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	sh4.r[0] = memRead32(sh4.gbr + (d << 2));

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVCAL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	memWrite32(sh4.r[n], sh4.r[0]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVA(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	sh4.r[0] = ((sh4.pc & 0xfffffffc) + (d << 2) + 4);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MOVT(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.sr.t;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SWAPB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = ((sh4.r[m] & 0xffff0000) << 0) |
			   ((sh4.r[m] & 0x000000ff) << 8) |
			   ((sh4.r[m] & 0x0000ff00) >> 8);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SWAPW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = ((sh4.r[m] & 0xffff0000) >> 16) |
			   ((sh4.r[m] & 0x0000ffff) << 16);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL XTRCT(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = ((sh4.r[n] & 0xffff0000) >> 16) |
			   ((sh4.r[m] & 0x0000ffff) << 16);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ADD(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] += sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ADDI(u32 code) {
	u32 i = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] += (u32)(s32)(s8)i;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ADDC(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 tmp0 = sh4.r[n];
	u32 tmp1 = sh4.r[n] + sh4.r[m];
	sh4.r[n] = tmp1 + sh4.sr.t;
	sh4.sr.t = tmp0 > tmp1;
	if (tmp1 > sh4.r[n]) sh4.sr.t = 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ADDV(u32 code) {
	u32 ans;
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 dest = (sh4.r[n] >> 31) & 1;
	u32 src = (sh4.r[m] >> 31) & 1;

	src += dest;
	sh4.r[n] += sh4.r[m];

	ans = (sh4.r[n] >> 31) & 1;
	ans += dest;

	if ((src == 0) || (src == 2))
		sh4.sr.t = (ans == 1);
	else
		sh4.sr.t = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPIM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	sh4.sr.t = (sh4.r[0] == (u32)(s32)(s8)i);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPEQ(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] == sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPHS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((u32)sh4.r[n] >= (u32)sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPGE(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((s32)sh4.r[n] >= (s32)sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPHI(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((u32)sh4.r[n] > (u32)sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPGT(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((s32)sh4.r[n] > (s32)sh4.r[m]);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPPZ(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((s32)sh4.r[n] >= 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPPL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((s32)sh4.r[n] > 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CMPSTR(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 tmp = sh4.r[n] ^ sh4.r[m];
	u32 HH = (tmp >> 24) & 0x000000FF;
	u32 HL = (tmp >> 16) & 0x000000FF;
	u32 LH = (tmp >> 8) & 0x000000FF;
	u32 LL = (tmp >> 0) & 0x000000FF;

	sh4.sr.t = ((HH && HL && LH && LL) == 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL DIV1(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 tmp1 = (sh4.r[n] >> 31) & 1;
	u32 tmp2 = sh4.r[n] = ((sh4.r[n] << 1) | sh4.sr.t);

	switch (sh4.sr.q) {
	case 0:
		switch (sh4.sr.m) {
		case 0:
			sh4.r[n] -= sh4.r[m];
			sh4.sr.q = ((sh4.r[n] > tmp2) ^ tmp1);
			break;

		case 1:
			sh4.r[n] += sh4.r[m];
			sh4.sr.q = ((sh4.r[n] >= tmp2) ^ tmp1);
			break;
		}
		break;

	case 1:
		switch (sh4.sr.m) {
		case 0:
			sh4.r[n] += sh4.r[m];
			sh4.sr.q = ((sh4.r[n] < tmp2) ^ tmp1);
			break;

		case 1:
			sh4.r[n] -= sh4.r[m];
			sh4.sr.q = ((sh4.r[n] <= tmp2) ^ tmp1);
			break;
		}
		break;
	}

	sh4.sr.t = (sh4.sr.q == sh4.sr.m);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL DIV0S(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.q = (sh4.r[n] >> 31) & 1;
	sh4.sr.m = (sh4.r[m] >> 31) & 1;
	sh4.sr.t = !(sh4.sr.q == sh4.sr.m);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL DIV0U(u32 code) {
	sh4.sr.q = sh4.sr.m = sh4.sr.t = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL DMULS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	s64 mult = (s64)(s32)sh4.r[n] * (s64)(s32)sh4.r[m];

	sh4.macl = (u32)((mult >> 0) & 0xffffffff);
	sh4.mach = (u32)((mult >> 32) & 0xffffffff);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL DMULU(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u64 mult = (u64)(u32)sh4.r[n] * (u64)(u32)sh4.r[m];

	sh4.macl = (u32)((mult >> 0) & 0xffffffff);
	sh4.mach = (u32)((mult >> 32) & 0xffffffff);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL DT(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (--sh4.r[n] == 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL EXTSB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = (u32)(s32)(s8)sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL EXTSW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = (u32)(s32)(s16)sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL EXTUB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = (u32)(u8)sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL EXTUW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = (u32)(u16)sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL MACL(u32 code) {
	u32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	u32 temp0, temp1, temp2, temp3;
	s32 tempm, tempn, fnLmL;

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	tempn = (s32)memRead32(sh4.r[n]);
	sh4.r[n] += 4;
	tempm = (s32)memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	if ((s32)(tempn ^ tempm) < 0)
		fnLmL = -1;
	else
		fnLmL = 0;

	if (tempn < 0)
		tempn = 0 - tempn;
	if (tempm < 0)
		tempm = 0 - tempm;

	temp1 = (u32)tempn;
	temp2 = (u32)tempm;

	RnL = (temp1 >> 0) & 0x0000FFFF;
	RnH = (temp1 >> 16) & 0x0000FFFF;
	RmL = (temp2 >> 0) & 0x0000FFFF;
	RmH = (temp2 >> 16) & 0x0000FFFF;

	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;

	Res2 = 0;
	Res1 = temp1 + temp2;

	if (Res1 < temp1)
		Res2 += 0x00010000;

	temp1 = (Res1 << 16) & 0xFFFF0000;
	Res0 = temp0 + temp1;

	if (Res0 < temp0)
		Res2++;

	Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;

	if (fnLmL < 0) {
		Res2 = ~Res2;
		if (Res0 == 0)
			Res2++;
		else
			Res0 = (~Res0) + 1;
	}

	if (sh4.sr.s == 1) {
		Res0 = sh4.macl + Res0;
		if (sh4.macl > Res0)
			Res2++;

		if (sh4.mach & 0x00008000) ;
		else Res2 += sh4.mach | 0xFFFF0000;

		Res2 += (sh4.macl & 0x0000FFFF);

		if (((s32)Res2 < 0) && (Res2 < 0xFFFF8000)) {
			Res2 = 0x00008000;
			Res0 = 0x00000000;
		}

		if (((s32)Res2 > 0) && (Res2 > 0x00007FFF)) {
			Res2 = 0x00007FFF;
			Res0 = 0xFFFFFFFF;
		}
		;

		sh4.mach = Res2;
		sh4.macl = Res0;
	} else {
		Res0 = sh4.macl + Res0;

		if (sh4.macl > Res0)
			Res2++;

		Res2 += sh4.mach;

		sh4.mach = Res2;
		sh4.macl = Res0;
	}
	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL MACW(u32 code) {
	s32 tempm, tempn, dest, src, ans;
	u32 templ;

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	tempn = (s32)memRead16(sh4.r[n]);
	sh4.r[n] += 2;
	tempm = (s32)memRead16(sh4.r[m]);
	sh4.r[m] += 2;

	templ = sh4.macl;
	tempm = ((s32)(s16)tempn * (s32)(s16)tempm);

	if ((s32)sh4.macl >= 0)
		dest = 0;
	else
		dest = 1;

	if ((s32)tempm >= 0) {
		src = 0;
		tempn = 0;
	} else {
		src = 1;
		tempn = 0xFFFFFFFF;
	}

	src += dest;

	sh4.macl += tempm;

	if ((s32)sh4.macl >= 0)
		ans = 0;
	else
		ans = 1;

	ans += dest;

	if (sh4.sr.s == 1) {
		if (ans == 1) {
			if (src == 0) sh4.macl = 0x7FFFFFFF;
			if (src == 2) sh4.macl = 0x80000000;
		}
	} else {
		sh4.mach += tempn;
		if (templ > sh4.macl)
			sh4.mach += 1;
	}

	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL MULL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.macl = sh4.r[n] * sh4.r[m];

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL MULSW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.macl = (s32)(s16)sh4.r[n] * (s32)(s16)sh4.r[m];

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL MULSU(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.macl = (u32)(u16)sh4.r[n] * (u32)(u16)sh4.r[m];

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL NEG(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = 0 - sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL NEGC(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 tmp = 0 - sh4.r[m];
	sh4.r[n] = tmp - sh4.sr.t;
	sh4.sr.t = 0 < tmp;
	if (tmp < sh4.r[n]) sh4.sr.t = 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SUB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SUBC(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 tmp0 = sh4.r[n];
	u32 tmp1 = sh4.r[n] - sh4.r[m];
	sh4.r[n] = tmp1 - sh4.sr.t;
	sh4.sr.t = (tmp0 < tmp1);
	if (tmp1 < sh4.r[n]) sh4.sr.t = 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SUBV(u32 code) {
	u32 ans;
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	u32 dest = (sh4.r[n] >> 31) & 1;
	u32 src = (sh4.r[m] >> 31) & 1;

	src += dest;
	sh4.r[n] -= sh4.r[m];

	ans = (sh4.r[n] >> 31) & 1;
	ans += dest;

	if (src == 1)
		sh4.sr.t = (ans == 1);
	else
		sh4.sr.t = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL AND(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] &= sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ANDI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	sh4.r[0] &= i;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ANDM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	u32 value = memRead8(sh4.gbr + sh4.r[0]);
	memWrite8(sh4.gbr + sh4.r[0], value & i);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL NOT(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = ~sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL OR(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] |= sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ORI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	sh4.r[0] |= i;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ORM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	u32 value = memRead8(sh4.gbr + sh4.r[0]);
	memWrite8(sh4.gbr + sh4.r[0], value | i);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL TAS(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u8 value = (u8)memRead8(sh4.r[n]);
	sh4.sr.t = (value == 0);
	memWrite8(sh4.r[n], value | 0x80);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL TST(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = ((sh4.r[n] & sh4.r[m]) == 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL TSTI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	sh4.sr.t = ((sh4.r[0] & i) == 0);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL TSTM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	u32 value = memRead8(sh4.gbr + sh4.r[0]);
	sh4.sr.t = ((value & i) == 0);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL XOR(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] ^= sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL XORI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	sh4.r[0] ^= i;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL XORM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	u32 value = memRead8(sh4.gbr + sh4.r[0]);
	memWrite8(sh4.gbr + sh4.r[0], value ^ i);

	sh4.pc += 2;
	iount -= 5;
}

static void INLINE FASTCALL ROTR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] >> 0) & 1;

	sh4.r[n] = ((sh4.r[n] & 0x00000001) << 31) |
			   ((sh4.r[n] & 0xfffffffe) >> 1);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ROTCR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 t = (sh4.r[n] >> 0) & 1;

	sh4.r[n] = ((sh4.sr.t & 0x00000001) << 31) |
			   ((sh4.r[n] & 0xfffffffe) >> 1);

	sh4.sr.t = t;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ROTL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] >> 31) & 1;

	sh4.r[n] = ((sh4.r[n] & 0x7fffffff) << 1) |
			   ((sh4.r[n] & 0x80000000) >> 31);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL ROTCL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 t = (sh4.r[n] >> 31) & 1;

	sh4.r[n] = ((sh4.r[n] & 0x7fffffff) << 1) |
			   ((sh4.sr.t & 0x00000001) << 0);

	sh4.sr.t = t;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHAD(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((sh4.r[m] & 0x80000000) == 0) {
		sh4.r[n] <<= (sh4.r[m] & 0x1f);
	} else if ((sh4.r[m] & 0x1f) == 0) {
		(s32)sh4.r[n] >>= 31;
	} else {
		(s32)sh4.r[n] >>= ((~sh4.r[m] & 0x1f) + 1);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHAR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = sh4.r[n] & 1;
	(s32)sh4.r[n] >>= 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLD(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((sh4.r[m] & 0x80000000) == 0) {
		sh4.r[n] <<= (sh4.r[m] & 0x1f);
	} else if ((sh4.r[m] & 0x1f) == 0) {
		sh4.r[n] = 0;
	} else {
		sh4.r[n] >>= ((~sh4.r[m] & 0x1f) + 1);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHAL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] >> 31) & 1;
	sh4.r[n] <<= 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] >> 31) & 1;
	sh4.r[n] <<= 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.sr.t = (sh4.r[n] >> 0) & 1;
	sh4.r[n] >>= 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLL2(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] <<= 2;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLR2(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] >>= 2;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLL8(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] <<= 8;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLR8(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] >>= 8;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLL16(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] <<= 16;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SHLR16(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] >>= 16;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL BF(u32 code) {
	if (sh4.sr.t == 0) {
		u32 d = ((code >> 0) & 0xff);

		sh4.pc += (((s32)(s8)d) << 1) + 4;
		iount -= 2;
	} else {
		sh4.pc += 2;
		iount -= 2;
	}

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BFS(u32 code) {
	if (sh4.sr.t == 0) {
		u32 d = ((code >> 0) & 0xff);

		u32 pc = sh4.pc + (((s32)(s8)d) << 1) + 4;

		intpStep(sh4.pc + 2);

		sh4.pc = pc;
		iount -= 2;
	} else {
		u32 pc = sh4.pc + 4;

		intpStep(sh4.pc + 2);

		sh4.pc = pc;
		iount -= 2;
	}

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BT(u32 code) {
	if (sh4.sr.t != 0) {
		u32 d = ((code >> 0) & 0xff);

		sh4.pc += (((s32)(s8)d) << 1) + 4;
		iount -= 2;
	} else {
		sh4.pc += 2;
		iount -= 2;
	}

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BTS(u32 code) {
	if (sh4.sr.t != 0) {
		u32 d = ((code >> 0) & 0xff);

		u32 pc = sh4.pc + (((s32)(s8)d) << 1) + 4;

		intpStep(sh4.pc + 2);

		sh4.pc = pc;
		iount -= 2;
	} else {
		u32 pc = sh4.pc + 4;

		intpStep(sh4.pc + 2);

		sh4.pc = pc;
		iount -= 2;
	}

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BRA(u32 code) {
	u32 d = ((s32)(code << 20) >> 20);

	u32 pc = sh4.pc + (d << 1) + 4;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BSR(u32 code) {
	u32 d = ((s32)(code << 20) >> 20);

	u32 pc = sh4.pc + (d << 1) + 4;
	sh4.pr = sh4.pc + 4;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BRAF(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 pc = sh4.pc + sh4.r[n] + 4;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL BSRF(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 pc = sh4.pc + sh4.r[n] + 4;
	sh4.pr = sh4.pc + 4;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL JMP(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 pc = sh4.r[n];

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL JSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	u32 pc = sh4.r[n];
	sh4.pr = sh4.pc + 4;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL RTS(u32 code) {
	u32 pc = sh4.pr;

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 2;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL RTE(u32 code) {
	u32 pc = sh4.spc;

	u32 rb = sh4.sr.rb;
	sh4.sr.all = sh4.ssr & 0x700083f3;

	if (sh4.sr.rb != rb) {
		int i;

		for (i = 0; i < 8; i++) {
			u32 value = sh4.r[ 16 + i ];
			sh4.r[ 16 + i ] = sh4.r[  0 + i ];
			sh4.r[  0 + i ] = value;
		}
	}

	intpStep(sh4.pc + 2);

	sh4.pc = pc;
	iount -= 5;

	NEED_INTERRUPTS_TEST = true;
}

static void INLINE FASTCALL CLRMAC(u32 code) {
	sh4.macl = sh4.mach = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CLRS(u32 code) {
	sh4.sr.s = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL CLRT(u32 code) {
	sh4.sr.t = 0;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	u32 rb = sh4.sr.rb;

	sh4.sr.all = sh4.r[m] & 0x700083f3;

	if ((sh4.sr.rb != rb) && (sh4.sr.md)) {
		int i;

		for (i = 0; i < 8; i++) {
			u32 value = sh4.r[ 16 + i ];
			sh4.r[ 16 + i ] = sh4.r[  0 + i ];
			sh4.r[  0 + i ] = value;
		}
	}

	sh4.pc += 2;
	iount -= 4;
}

static void INLINE FASTCALL LDCGBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.gbr = sh4.r[m];

	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL LDCVBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.vbr = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCSSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.ssr = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCSPC(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.spc = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCDBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.dbr = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 m = ((code >> 8) & 0x0f);

	sh4.r[b] = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCMSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	u32 rb = sh4.sr.rb;

	sh4.sr.all = memRead32(sh4.r[m]) & 0x700083f3;

	if ((sh4.sr.rb != rb) && (sh4.sr.md)) {
		int i;

		for (i = 0; i < 8; i++) {
			u32 value = sh4.r[ 16 + i ];
			sh4.r[ 16 + i ] = sh4.r[  0 + i ];
			sh4.r[  0 + i ] = value;
		}
	}

	sh4.r[m] += 4;	//??

	sh4.pc += 2;
	iount -= 4;
}

static void INLINE FASTCALL LDCMGBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.gbr = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL LDCMVBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.vbr = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCMSSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.ssr = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCMSPC(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.spc = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCMDBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.dbr = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDCMRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 m = ((code >> 8) & 0x0f);

	sh4.r[b] = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMACH(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.mach = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMACL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.macl = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSPR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.pr = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMMACH(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.mach = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMMACL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.macl = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMPR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.pr = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDTLB(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL NOP(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL OCBI(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL OCBP(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL OCBWB(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL PREF(u32 code) {
	u32 Rn = sh4.r[((code >> 8) & 0x0f)];
	u32 addr;
	u32 *SQ;

	if ((Rn >= 0xe0000000) && (Rn <= 0xe3fffffc)) {
		if (Rn & 0x20) {
			if (ONCHIP8(0x0010) & 1) addr = sqMemTranslate(Rn);
			else addr = (Rn & 0x03ffffe0) | ((ONCHIP8(0x003c) & 0x1c) << 24);
			SQ = SQ0 + 8;
		} else {
			if (ONCHIP8(0x0010) & 1) addr = sqMemTranslate(Rn);
			else addr = (Rn & 0x03ffffe0) | ((ONCHIP8(0x0038) & 0x1c) << 24);
			SQ = SQ0;
		}

		if ((addr >= 0x10000000) && (addr < 0x10800000)) {
			gpuTaTransfer(SQ, 32);
		} else {
			u8 *dst = (u8*)(memAccess[addr >> 16]) + (addr & 0xffff);

			memcpy(dst, SQ, 32);
		}
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SETS(u32 code) {
	sh4.sr.s = 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SETT(u32 code) {
	sh4.sr.t = 1;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL SLEEP(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STCSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.sr.all;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCGBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.gbr;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCVBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.vbr;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCSSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.vbr;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCSPC(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.spc;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCSGR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.sgr;

	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL STCDBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.dbr;

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.r[b];

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.sr.all);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMGBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.gbr);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMVBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.vbr);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMSSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.ssr);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMSPC(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.spc);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMSGR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.sgr);

	sh4.pc += 2;
	iount -= 3;
}

static void INLINE FASTCALL STCMDBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.dbr);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STCMRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.r[b]);

	sh4.pc += 2;
	iount -= 2;
}

static void INLINE FASTCALL STSMACH(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.mach;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMACL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.macl;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSPR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.pr;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMMACH(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.mach);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMMACL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.macl);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMPR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.pr);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL TRAPA(u32 code) {
	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSFPSCR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	u32 fr = sh4.fpscr.fr;

	sh4.fpscr.all = sh4.r[m] & 0x003fffff;

	if (sh4.fpscr.fr != fr) {
		int i;

		for (i = 0; i < 8; i++) {
			u64 value = sh4.dbli[ 8 + i ];
			sh4.dbli[ 8 + i ] = sh4.dbli[ 0 + i ];
			sh4.dbli[ 0 + i ] = value;
		}
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSFPUL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.fpul = sh4.r[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMFPSCR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	u32 fr = sh4.fpscr.fr;

	sh4.fpscr.all = memRead32(sh4.r[m]) & 0x003fffff;
	sh4.r[m] += 4;

	if (sh4.fpscr.fr != fr) {
		int i;

		for (i = 0; i < 8; i++) {
			u64 value = sh4.dbli[ 8 + i ];
			sh4.dbli[ 8 + i ] = sh4.dbli[ 0 + i ];
			sh4.dbli[ 0 + i ] = value;
		}
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL LDSMFPUL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	sh4.fpul = memRead32(sh4.r[m]);
	sh4.r[m] += 4;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSFPSCR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.fpscr.all;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSFPUL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] = sh4.fpul;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMFPSCR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.fpscr.all);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL STSMFPUL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	sh4.r[n] -= 4;
	memWrite32(sh4.r[n], sh4.fpul);

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FLDI0(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	sh4.flti[n] = 0x00000000;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FLDI1(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	sh4.flti[n] = 0x3f800000;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV(u32 code) {
	if (sh4.fpscr.sz) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		switch (code & 0x00000110) {
		case 0x00000000:
			sh4.dbli[n + 0] = sh4.dbli[m + 0];
			break;

		case 0x00000010:
			sh4.dbli[n + 0] = sh4.dbli[m + 8];
			break;

		case 0x00000100:
			sh4.dbli[n + 8] = sh4.dbli[m + 0];
			break;

		case 0x00000110:
			sh4.dbli[n + 8] = sh4.dbli[m + 8];
			break;
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] = sh4.flti[m];
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_LOAD(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			memRead64(sh4.r[m], (u64*)&sh4.dbli[n]);
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			memRead64(sh4.r[m], (u64*)&sh4.dbli[n]);
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] = memRead32(sh4.r[m]);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_INDEX_LOAD(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			memRead64(sh4.r[m] + sh4.r[0], (u64*)&sh4.dbli[n]);
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			memRead64(sh4.r[m] + sh4.r[0], (u64*)&sh4.dbli[n]);
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] = memRead32(sh4.r[m] + sh4.r[0]);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_RESTORE(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			memRead64(sh4.r[m], (u64*)&sh4.dbli[n]);
			sh4.r[m] += 8;
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			memRead64(sh4.r[m], (u64*)&sh4.dbli[n]);
			sh4.r[m] += 8;
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] = memRead32(sh4.r[m]);
		sh4.r[m] += 4;
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_SAVE(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			sh4.r[n] -= 8;
			memWrite64(sh4.r[n], (u64*)&sh4.dbli[m]);
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			sh4.r[n] -= 8;
			memWrite64(sh4.r[n], (u64*)&sh4.dbli[m]);
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.r[n] -= 4;
		memWrite32(sh4.r[n], sh4.flti[m]);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_STORE(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			memWrite64(sh4.r[n], (u64*)&sh4.dbli[m]);
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			memWrite64(sh4.r[n], (u64*)&sh4.dbli[m]);
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		memWrite32(sh4.r[n], sh4.flti[m]);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FMOV_INDEX_STORE(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			memWrite64(sh4.r[n] + sh4.r[0], (u64*)&sh4.dbli[m]);
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			memWrite64(sh4.r[n] + sh4.r[0], (u64*)&sh4.dbli[m]);
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		memWrite32(sh4.r[n] + sh4.r[0], sh4.flti[m]);
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FLDS(u32 code) {
	u32 m = ((code >> 8) & 0xf);

	sh4.fpul = sh4.flti[m];

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FSTS(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	sh4.flti[n] = sh4.fpul;

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FABS(u32 code) {
	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbli[n] &= 0x7fffffffffffffff;

		sh4.pc += 2;
		iount--;
	} else {
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] &= 0x7fffffff;

		sh4.pc += 2;
		iount--;
	}
}

static void INLINE FASTCALL FNEG(u32 code) {
	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbli[n] ^= 0x8000000000000000;

		sh4.pc += 2;
		iount--;
	} else {
		u32 n = ((code >> 8) & 0xf);

		sh4.flti[n] ^= 0x80000000;

		sh4.pc += 2;
		iount--;
	}
}

static void INLINE FASTCALL FCNVDS(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 9) & 0x7);

		*(float*)&sh4.fpul = (float)(double)sh4.dbl[m];
	}

	sh4.pc += 2;
	iount--;

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FCNVSD(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] = (double)*(float*)&sh4.fpul;
	}

	sh4.pc += 2;
	iount--;

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FTRC(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 9) & 0x7);

		sh4.fpul = (s32)(double)sh4.dbl[m];

		sh4.pc += 2;
		iount -= 2;
	} else {
		u32 m = ((code >> 8) & 0xf);

		sh4.fpul = (s32)(float)sh4.flt[m];

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FLOAT1(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] = (double)(s32)sh4.fpul;

		sh4.pc += 2;
		iount -= 2;
	} else {
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] = (float)(s32)sh4.fpul;

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FCMPEQ(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.sr.t = (sh4.dbl[n] == sh4.dbl[m]);

		sh4.pc += 2;
		iount -= 2;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.sr.t = (sh4.flt[n] == sh4.flt[m]);

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FCMPGT(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.sr.t = (sh4.dbl[n] > sh4.dbl[m]);

		sh4.pc += 2;
		iount -= 2;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.sr.t = (sh4.flt[n] > sh4.flt[m]);

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FMAC(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr == 0) {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] += sh4.flt[m] * sh4.flt[0];
	}

	sh4.pc += 2;
	iount--;

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FDIV(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] /= sh4.dbl[m];

		sh4.pc += 2;
		iount -= 23;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] /= sh4.flt[m];

		sh4.pc += 2;
		iount -= 10;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FMUL(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] *= sh4.dbl[m];

		sh4.pc += 2;
		iount -= 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] *= sh4.flt[m];

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FSQRT(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] = (double)sqrt(sh4.dbl[n]);

		sh4.pc += 2;
		iount -= 22;
	} else {
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] = (float)sqrt(sh4.flt[n]);

		sh4.pc += 2;
		iount -= 9;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FSRRA(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] = 1.0f / (double)sqrt(sh4.dbl[n]);

		sh4.pc += 2;
		iount -= 22;
	} else {
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] = 1.0f / (float)sqrt(sh4.flt[n]);

		sh4.pc += 2;
		iount -= 9;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FADD(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] += sh4.dbl[m];

		sh4.pc += 2;
		iount -= 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] += sh4.flt[m];

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FSUB(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		sh4.dbl[n] -= sh4.dbl[m];

		sh4.pc += 2;
		iount -= 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		sh4.flt[n] -= sh4.flt[m];

		sh4.pc += 2;
		iount--;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FIPR(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr == 0) {
		u32 m = ((code >> 8) & 0x3);
		u32 n = ((code >> 10) & 0x3);

		sh4.vec[n][3] = sh4.vec[m][0] * sh4.vec[n][0] +
						sh4.vec[m][1] * sh4.vec[n][1] +
						sh4.vec[m][2] * sh4.vec[n][2] +
						sh4.vec[m][3] * sh4.vec[n][3];
	}

	sh4.pc += 2;
	iount--;

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FTRV(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr == 0) {
		u32 n = ((code >> 10) & 0x3);

		float v0 = sh4.vec[4][0] * sh4.vec[n][0] +
				   sh4.vec[5][0] * sh4.vec[n][1] +
				   sh4.vec[6][0] * sh4.vec[n][2] +
				   sh4.vec[7][0] * sh4.vec[n][3];

		float v1 = sh4.vec[4][1] * sh4.vec[n][0] +
				   sh4.vec[5][1] * sh4.vec[n][1] +
				   sh4.vec[6][1] * sh4.vec[n][2] +
				   sh4.vec[7][1] * sh4.vec[n][3];

		float v2 = sh4.vec[4][2] * sh4.vec[n][0] +
				   sh4.vec[5][2] * sh4.vec[n][1] +
				   sh4.vec[6][2] * sh4.vec[n][2] +
				   sh4.vec[7][2] * sh4.vec[n][3];

		float v3 = sh4.vec[4][3] * sh4.vec[n][0] +
				   sh4.vec[5][3] * sh4.vec[n][1] +
				   sh4.vec[6][3] * sh4.vec[n][2] +
				   sh4.vec[7][3] * sh4.vec[n][3];

		sh4.vec[n][0] = v0;
		sh4.vec[n][1] = v1;
		sh4.vec[n][2] = v2;
		sh4.vec[n][3] = v3;

		sh4.pc += 2;
		iount -= 4;
	}

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FSCA(u32 code) {
	SAVE_ROUNDING_MODE();

	if (sh4.fpscr.pr == 0) {
		u32 n = ((code >> 8) & 0x0e);

		float angle = (float)(2.0f * 3.1415926535f / 65536.0f) * (float)(s16)sh4.fpul;

		sh4.flt[n + 0] = (float)sin(angle);
		sh4.flt[n + 1] = (float)cos(angle);
	}

	sh4.pc += 2;
	iount--;

	RESTORE_ROUNDING_MODE();
}

static void INLINE FASTCALL FRCHG(u32 code) {
	int i;

	sh4.fpscr.fr = ~sh4.fpscr.fr;

	for (i = 0; i < 8; i++) {
		u64 value = sh4.dbli[ 8 + i ];
		sh4.dbli[ 8 + i ] = sh4.dbli[ 0 + i ];
		sh4.dbli[ 0 + i ] = value;
	}

	sh4.pc += 2;
	iount--;
}

static void INLINE FASTCALL FSCHG(u32 code) {
	sh4.fpscr.sz = ~sh4.fpscr.sz;

	sh4.pc += 2;
	iount--;
}

int intpOpen() {
	sh4.sr.all = 0x700000f0;
	sh4.fpscr.all = 0x00040001;
	sh4.pc = 0xa0000000;

	return 1;
}

void intpClose() {
}

void intpExecute() {
	iount = 1024;

	for (;; ) {
		intpStep(sh4.pc);

		if (NEED_INTERRUPTS_TEST == true) {
			NEED_INTERRUPTS_TEST = false;

			if (iount <= 0) break;
		}
	}

	sh4.cycle += 1024 - iount;

	CpuTest();
}

u32 uinter = 0;

void intpStep(u32 pc) {
	u32 code;

#ifdef DEMUL_DEBUG
	pc = hookDebugExecute(pc);
#endif

	if ((pc & 0x1fffffff) < 0x00200000)
		code = *(u16*)&prom[pc & 0x001fffff];
	else
		code = *(u16*)&pram[pc & 0x00ffffff];

	if ((pc == 0x8c0548e4) && (code == 0x6022)) code = 0x7001;	// Psyvarriar
	if ((pc == 0x0C14B2F2) && (code == 0x30E0)) code = 0x0009;	// Gigawing 2
	if ((pc == 0x8C033792) && (code == 0x30E0)) code = 0x0009;	// Jedi Power Battles

	switch ((code >> 12) & 0xf) {
	case 0:
		switch ((code >> 0) & 0xf) {
		case 2:
			switch ((code >> 4) & 0xf) {
			case 0:  STCSR(code);    return;
			case 1:  STCGBR(code);   return;
			case 2:  STCVBR(code);   return;
			case 3:  STCSSR(code);   return;
			case 4:  STCSPC(code);   return;
			case 8:  STCRBANK(code); return;
			case 9:  STCRBANK(code); return;
			case 10: STCRBANK(code); return;
			case 11: STCRBANK(code); return;
			case 12: STCRBANK(code); return;
			case 13: STCRBANK(code); return;
			case 14: STCRBANK(code); return;
			case 15: STCRBANK(code); return;
			default: NI(code);       return;
			}

		case 3:
			switch ((code >> 4) & 0xf) {
			case 0:  BSRF(code);     return;
			case 2:  BRAF(code);     return;
			case 8:  PREF(code);     return;
			case 9:  OCBI(code);     return;
			case 10: OCBP(code);     return;
			case 11: OCBWB(code);    return;
			case 12: MOVCAL(code);    return;
			default: NI(code);       return;
			}

		case 4: MOVBS0(code);   return;
		case 5: MOVWS0(code);   return;
		case 6: MOVLS0(code);   return;
		case 7: MULL(code);     return;

		case 8:
			switch ((code >> 4) & 0xf) {
			case 0:  CLRT(code);     return;
			case 1:  SETT(code);     return;
			case 2:  CLRMAC(code);   return;
			case 3:  CLRS(code);     return;
			case 4:  SETS(code);     return;
			default: NI(code);       return;
			}

		case 9:
			switch ((code >> 4) & 0xf) {
			case 0:  NOP(code);      return;
			case 1:  DIV0U(code);    return;
			case 2:  MOVT(code);     return;
			default: NI(code);       return;
			}

		case 10:
			switch ((code >> 4) & 0xf) {
			case 0:  STSMACH(code);  return;
			case 1:  STSMACL(code);  return;
			case 2:  STSPR(code);    return;
			case 3:  STCSGR(code);   return;
			case 5:  STSFPUL(code);  return;
			case 6:  STSFPSCR(code); return;
			case 15: STCDBR(code);   return;
			default: NI(code);       return;
			}

		case 11:
			switch ((code >> 4) & 0xf) {
			case 0:  RTS(code);      return;
			case 1:  SLEEP(code);    return;
			case 2:  RTE(code);      return;
			default: NI(code);       return;
			}

		case 12: MOVBL0(code);  return;
		case 13: MOVWL0(code);  return;
		case 14: MOVLL0(code);  return;
		case 15: MACL(code);    return;
		default: NI(code);      return;
		}

	case 1: MOVLS4(code); return;

	case 2:
		switch ((code >> 0) & 0xf) {
		case 0:  MOVBS(code);  return;
		case 1:  MOVWS(code);  return;
		case 2:  MOVLS(code);  return;
		case 4:  MOVBM(code);  return;
		case 5:  MOVWM(code);  return;
		case 6:  MOVLM(code);  return;
		case 7:  DIV0S(code);  return;
		case 8:  TST(code);    return;
		case 9:  AND(code);    return;
		case 10: XOR(code);    return;
		case 11: OR(code);     return;
		case 12: CMPSTR(code); return;
		case 13: XTRCT(code);  return;
		case 14: MULSU(code);  return;
		case 15: MULSW(code);  return;
		default: NI(code);     return;
		}

	case 3:
		switch ((code >> 0) & 0xf) {
		case 0:  CMPEQ(code);  return;
		case 2:  CMPHS(code);  return;
		case 3:  CMPGE(code);  return;
		case 4:  DIV1(code);   return;
		case 5:  DMULU(code);  return;
		case 6:  CMPHI(code);  return;
		case 7:  CMPGT(code);  return;
		case 8:  SUB(code);    return;
		case 10: SUBC(code);   return;
		case 11: SUBV(code);   return;
		case 12: ADD(code);    return;
		case 13: DMULS(code);  return;
		case 14: ADDC(code);   return;
		case 15: ADDV(code);   return;
		default: NI(code);     return;
		}

	case 4:
		switch ((code >> 0) & 0xf) {
		case 0:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLL(code);     return;
			case 1:  DT(code);       return;
			case 2:  SHAL(code);     return;
			default: NI(code);       return;
			}

		case 1:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLR(code);     return;
			case 1:  CMPPZ(code);    return;
			case 2:  SHAR(code);     return;
			default: NI(code);       return;
			}

		case 2:
			switch ((code >> 4) & 0xf) {
			case 0:  STSMMACH(code); return;
			case 1:  STSMMACL(code); return;
			case 2:  STSMPR(code);   return;
			case 5:  STSMFPUL(code); return;
			case 6:  STSMFPSCR(code); return;
			default: NI(code);       return;
			}

		case 3:
			switch ((code >> 4) & 0xf) {
			case 0:  STCMSR(code);   return;
			case 1:  STCMGBR(code);  return;
			case 2:  STCMVBR(code);  return;
			case 3:  STCMSSR(code);  return;
			case 4:  STCMSPC(code);  return;
			case 8:  STCMRBANK(code); return;
			case 9:  STCMRBANK(code); return;
			case 10: STCMRBANK(code); return;
			case 11: STCMRBANK(code); return;
			case 12: STCMRBANK(code); return;
			case 13: STCMRBANK(code); return;
			case 14: STCMRBANK(code); return;
			case 15: STCMRBANK(code); return;
			default: NI(code);       return;
			}

		case 4:
			switch ((code >> 4) & 0xf) {
			case 0:  ROTL(code);     return;
			case 2:  ROTCL(code);    return;
			default: NI(code);       return;
			}

		case 5:
			switch ((code >> 4) & 0xf) {
			case 0:  ROTR(code);     return;
			case 1:  CMPPL(code);    return;
			case 2:  ROTCR(code);    return;
			default: NI(code);       return;
			}

		case 6:
			switch ((code >> 4) & 0xf) {
			case 0:  LDSMMACH(code); return;
			case 1:  LDSMMACL(code); return;
			case 2:  LDSMPR(code);   return;
			case 5:  LDSMFPUL(code); return;
			case 6:  LDSMFPSCR(code); return;
			case 15: LDCMDBR(code);  return;
			default: NI(code);       return;
			}

		case 7:
			switch ((code >> 4) & 0xf) {
			case 0:  LDCMSR(code);   return;
			case 1:  LDCMGBR(code);  return;
			case 2:  LDCMVBR(code);  return;
			case 3:  LDCMSSR(code);  return;
			case 4:  LDCMSPC(code);  return;
			case 8:  LDCMRBANK(code); return;
			case 9:  LDCMRBANK(code); return;
			case 10: LDCMRBANK(code); return;
			case 11: LDCMRBANK(code); return;
			case 12: LDCMRBANK(code); return;
			case 13: LDCMRBANK(code); return;
			case 14: LDCMRBANK(code); return;
			case 15: LDCMRBANK(code); return;
			default: NI(code);       return;
			}

		case 8:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLL2(code);    return;
			case 1:  SHLL8(code);    return;
			case 2:  SHLL16(code);   return;
			default: NI(code);       return;
			}

		case 9:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLR2(code);    return;
			case 1:  SHLR8(code);    return;
			case 2:  SHLR16(code);   return;
			default: NI(code);       return;
			}

		case 10:
			switch ((code >> 4) & 0xf) {
			case 0:  LDSMACH(code);  return;
			case 1:  LDSMACL(code);  return;
			case 2:  LDSPR(code);    return;
			case 5:  LDSFPUL(code);  return;
			case 6:  LDSFPSCR(code); return;
			case 15: LDCDBR(code);   return;
			default: NI(code);       return;
			}

		case 11:
			switch ((code >> 4) & 0xf) {
			case 0:  JSR(code);      return;
			case 1:  TAS(code);      return;
			case 2:  JMP(code);      return;
			default: NI(code);       return;
			}

		case 12: SHAD(code);    return;
		case 13: SHLD(code);    return;

		case 14:
			switch ((code >> 4) & 0xf) {
			case 0:  LDCSR(code);     return;
			case 1:  LDCGBR(code);    return;
			case 2:  LDCVBR(code);    return;
			case 3:  LDCSSR(code);    return;
			case 4:  LDCSPC(code);    return;
			case 8:  LDCRBANK(code);  return;
			case 9:  LDCRBANK(code);  return;
			case 10: LDCRBANK(code);  return;
			case 11: LDCRBANK(code);  return;
			case 12: LDCRBANK(code);  return;
			case 13: LDCRBANK(code);  return;
			case 14: LDCRBANK(code);  return;
			case 15: LDCRBANK(code);  return;
			default: NI(code);        return;
			}

		case 15: MACW(code);    return;
		default: NI(code);      return;
		}

	case 5: MOVLL4(code); return;

	case 6:
		switch ((code >> 0) & 0xf) {
		case 0:  MOVBL(code);   return;
		case 1:  MOVWL(code);   return;
		case 2:  MOVLL(code);   return;
		case 3:  MOV(code);     return;
		case 4:  MOVBP(code);   return;
		case 5:  MOVWP(code);   return;
		case 6:  MOVLP(code);   return;
		case 7:  NOT(code);     return;
		case 8:  SWAPB(code);   return;
		case 9:  SWAPW(code);   return;
		case 10: NEGC(code);    return;
		case 11: NEG(code);     return;
		case 12: EXTUB(code);   return;
		case 13: EXTUW(code);   return;
		case 14: EXTSB(code);   return;
		case 15: EXTSW(code);   return;
		}

	case 7: ADDI(code);   return;

	case 8:
		switch ((code >> 8) & 0xf) {
		case 0:  MOVBS4(code);  return;
		case 1:  MOVWS4(code);  return;
		case 4:  MOVBL4(code);  return;
		case 5:  MOVWL4(code);  return;
		case 8:  CMPIM(code);   return;
		case 9:  BT(code);      return;
		case 11: BF(code);      return;
		case 13: BTS(code);     return;
		case 15: BFS(code);     return;
		default: NI(code);      return;
		}

	case 9:  MOVWI(code); return;
	case 10: BRA(code);   return;
	case 11: BSR(code);   return;

	case 12:
		switch ((code >> 8) & 0xf) {
		case 0:  MOVBSG(code);  return;
		case 1:  MOVWSG(code);  return;
		case 2:  MOVLSG(code);  return;
		case 3:  TRAPA(code);   return;
		case 4:  MOVBLG(code);  return;
		case 5:  MOVWLG(code);  return;
		case 6:  MOVLLG(code);  return;
		case 7:  MOVA(code);    return;
		case 8:  TSTI(code);    return;
		case 9:  ANDI(code);    return;
		case 10: XORI(code);    return;
		case 11: ORI(code);     return;
		case 12: TSTM(code);    return;
		case 13: ANDM(code);    return;
		case 14: XORM(code);    return;
		case 15: ORM(code);     return;
		}

	case 13: MOVLI(code); return;
	case 14: MOVI(code);  return;

	case 15:
		switch ((code >> 0) & 0xf) {
		case 0:  FADD(code);                return;
		case 1:  FSUB(code);                return;
		case 2:  FMUL(code);                return;
		case 3:  FDIV(code);                return;
		case 4:  FCMPEQ(code);              return;
		case 5:  FCMPGT(code);              return;
		case 6:  FMOV_INDEX_LOAD(code);     return;
		case 7:  FMOV_INDEX_STORE(code);    return;
		case 8:  FMOV_LOAD(code);           return;
		case 9:  FMOV_RESTORE(code);        return;
		case 10: FMOV_STORE(code);          return;
		case 11: FMOV_SAVE(code);           return;
		case 12: FMOV(code);                return;

		case 13:
			switch ((code >> 4) & 0xf) {
			case 0:  FSTS(code);            return;
			case 1:  FLDS(code);            return;
			case 2:  FLOAT1(code);          return;
			case 3:  FTRC(code);            return;
			case 4:  FNEG(code);            return;
			case 5:  FABS(code);            return;
			case 6:  FSQRT(code);           return;
			case 7:  FSRRA(code);           return;
			case 8:  FLDI0(code);           return;
			case 9:  FLDI1(code);           return;
			case 10: FCNVSD(code);          return;
			case 11: FCNVDS(code);          return;
			case 14: FIPR(code);            return;

			case 15:
				switch ((code >> 8) & 0xf) {
				case 0:  FSCA(code);    return;
				case 1:  FTRV(code);    return;
				case 2:  FSCA(code);    return;
				case 3:  FSCHG(code);   return;
				case 4:  FSCA(code);    return;
				case 5:  FTRV(code);    return;
				case 6:  FSCA(code);    return;
				case 8:  FSCA(code);    return;
				case 9:  FTRV(code);    return;
				case 10: FSCA(code);    return;
				case 11: FRCHG(code);   return;
				case 12: FSCA(code);    return;
				case 13: FTRV(code);    return;
				case 14: FSCA(code);    return;
				default: NI(code);      return;
				}

			default: NI(code);  return;
			}

		case 14: FMAC(code);    return;
		case 15: NI(code);      return;
		}
	}
}
