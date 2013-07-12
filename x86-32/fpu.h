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

#ifndef __FPU_H__
#define __FPU_H__

#include "x86.h"

typedef enum {
	st0 = 0,
	st1 = 1,
	st2 = 2,
	st3 = 3,
	st4 = 4,
	st5 = 5,
	st6 = 6,
	st7 = 7
} x86FPURegType;

void FLD1();
void FLDZ();
void FLD64M(u32 mem);
void FSTP64M(u32 mem);
void FLD32M(u32 mem);
void FILD32M(u32 mem);
void FILD16M(u32 mem);
void FST32M(u32 mem);
void FSTP32R(u32 reg);
void FSTP32M(u32 mem);
void FISTP32M(u32 mem);
void FCOMP64M(u32 mem);
void FCOMP32M(u32 mem);
void FCOMIP(u32 reg);
void FNSTSWtoAX();
void FLDCW(u32 mem);
void FNSTCW(u32 mem);
void FADD64M(u32 mem);
void FADD32M(u32 mem);
void FADDP32RtoST0(u8 reg);
void FSUB64M(u32 mem);
void FSUB32M(u32 mem);
void FMUL64M(u32 mem);
void FMUL32M(u32 mem);
void FMUL32ST0toR(u8 reg);
void FMULP32RtoST0(u8 reg);
void FDIV64M(u32 mem);
void FDIV32M(u32 mem);
void FDIVP32RtoST0(u8 reg);
void FSINCOS32();
void FSQRT32();

#endif