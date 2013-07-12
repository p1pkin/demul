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

#include "sse.h"

void STMXCSR(u32 mem) {
	write16(0xae0f);
	ModRm(0, 3, 5);
	write32(mem);
}

void LDMXCSR(u32 mem) {
	write16(0xae0f);
	ModRm(0, 2, 5);
	write32(mem);
}

void MOVAPS128MtoXMM(x86SSERegType reg, u32 mem) {
	write16(0x280f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVAPS128XMMtoM(u32 mem, x86SSERegType reg) {
	write16(0x290f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVSS32MtoXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x100f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVSS32XMMtoM(u32 mem, x86SSERegType reg) {
	write8(0xf3);
	write16(0x110f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVAPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write16(0x280f);
	ModRm(3, dst, src);
}

void MOVSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write8(0xf3);
	write16(0x100f);
	ModRm(3, dst, src);
}

void MOVLPS64MtoXMM(x86SSERegType dst, u32 src) {
	write16(0x120f);
	ModRm(0, dst, 5);
	write32(src);
}

void MOVLPS64XMMtoM(u32 dst, x86SSERegType src) {
	write16(0x130f);
	ModRm(0, src, 5);
	write32(dst);
}

void MULPS128MtoXMM(x86SSERegType reg, u32 mem) {
	write16(0x590f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MULSSM32toXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x590f);
	ModRm(0, reg, 5);
	write32(mem);
}

void DIVSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write8(0xf3);
	write16(0x5e0f);
	ModRm(3, dst, src);
}

void DIVSS32MtoXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x5e0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void ADDPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write16(0x580f);
	ModRm(3, dst, src);
}

void ADDSS32XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write8(0xf3);
	write16(0x580f);
	ModRm(3, dst, src);
}

void ADDSS32MtoXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x580f);
	ModRm(0, reg, 5);
	write32(mem);
}

void SUBSSM32toXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x5c0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void SHUFPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src, u8 imm8) {
	write16(0xc60f);
	ModRm(3, dst, src);
	write8(imm8);
}

void MOVHLPS128XMMtoXMM(x86SSERegType dst, x86SSERegType src) {
	write16(0x120f);
	ModRm(3, dst, src);
}

void UCOMISS32MtoXMM(x86SSERegType reg, u32 mem) {
	write16(0x2e0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void CVTTSS2SI32MtoR(x86Reg32Type reg, u32 mem) {
	write8(0xf3);
	write16(0x2c0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void CVTTSI2SS32MtoXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x2a0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void RSQRTSSM32toXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x520f);
	ModRm(0, reg, 5);
	write32(mem);
}

void SQRTSSM32toXMM(x86SSERegType reg, u32 mem) {
	write8(0xf3);
	write16(0x510f);
	ModRm(0, reg, 5);
	write32(mem);
}