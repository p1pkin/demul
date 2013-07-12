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

#ifndef __SSE_H__
#define __SSE_H__

#include "x86.h"

typedef enum {
	XMM0 = 0,
	XMM1 = 1,
	XMM2 = 2,
	XMM3 = 3,
	XMM4 = 4,
	XMM5 = 5,
	XMM6 = 6,
	XMM7 = 7,
} x86SSERegType;

#define XMMREGS 8

typedef struct {
	u32 reg;
	u32 inuse;
	u32 age;
} _xmmregs;

_xmmregs xmmregs[XMMREGS];

void STMXCSR(u32 mem);
void LDMXCSR(u32 mem);

void MOVAPS128MtoXMM(x86SSERegType reg, u32 mem);
void MOVAPS128XMMtoM(u32 mem, x86SSERegType reg);

void MOVSS32MtoXMM(x86SSERegType reg, u32 mem);
void MOVSS32XMMtoM(u32 mem, x86SSERegType reg);

void MOVAPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src);
void MOVSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src);

void MOVLPS64MtoXMM(x86SSERegType dst, u32 src);
void MOVLPS64XMMtoM(u32 dst, x86SSERegType src);

void MULPS128MtoXMM(x86SSERegType reg, u32 mem);
void MULSSM32toXMM(x86SSERegType reg, u32 mem);

void DIVSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src);
void DIVSS32MtoXMM(x86SSERegType reg, u32 mem);

void ADDPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src);
void ADDSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src);
void ADDSS32MtoXMM(x86SSERegType reg, u32 mem);

void SUBSSM32toXMM(x86SSERegType reg, u32 mem);

void SHUFPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src, u8 imm8);

void MOVHLPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src);

void UCOMISS32MtoXMM(x86SSERegType reg, u32 mem);

void CVTTSS2SI32MtoR(x86Reg32Type reg, u32 mem);

void CVTTSI2SS32MtoXMM(x86SSERegType reg, u32 mem);

void RSQRTSSM32toXMM(x86SSERegType reg, u32 mem);

void SQRTSSM32toXMM(x86SSERegType reg, u32 mem);

#endif