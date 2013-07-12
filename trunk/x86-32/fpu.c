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

#include "fpu.h"

void FLD1() {
	write16(0xe8d9);
}

void FLDZ() {
	write16(0xeed9);
}

void FLD64M(u32 mem) {
	write8(0xdd);
	ModRm(0, 0, 5);
	write32(mem);
}

void FST64M(u32 mem) {
	write8(0xdd);
	ModRm(0, 2, 5);
	write32(mem);
}

void FSTP64M(u32 mem) {
	write8(0xdd);
	ModRm(0, 3, 5);
	write32(mem);
}

void FLD32M(u32 mem) {
	write8(0xd9);
	ModRm(0, 0, 5);
	write32(mem);
}

void FILD32M(u32 mem) {
	write8(0xdb);
	ModRm(0, 0, 5);
	write32(mem);
}

void FILD16M(u32 mem) {
	write8(0xdf);
	ModRm(0, 0, 5);
	write32(mem);
}

void FST32M(u32 mem) {
	write8(0xd9);
	ModRm(0, 2, 5);
	write32(mem);
}

void FSTP32R(u32 reg) {
	write8(0xdd);
	ModRm(3, 3, reg);
}

void FSTP32M(u32 mem) {
	write8(0xd9);
	ModRm(0, 3, 5);
	write32(mem);
}

void FISTP32M(u32 mem) {
	write8(0xdb);
	ModRm(0, 0x3, 5);
	write32(mem);
}

void FCOMP64M(u32 mem) {
	write8(0xdc);
	ModRm(0, 3, 5);
	write32(mem);
}

void FCOMP32M(u32 mem) {
	write8(0xd8);
	ModRm(0, 3, 5);
	write32(mem);
}

void FCOMIP(u32 reg) {
	write8(0xdf);
	write8(0xf0 + reg);
}

void FNSTSWtoAX() {
	write16(0xe0df);
}

void FLDCW(u32 mem) {
	write8(0xd9);
	ModRm(0, 5, 5);
	write32(mem);
}

void FNSTCW(u32 mem) {
	write8(0xd9);
	ModRm(0, 7, 5);
	write32(mem);
}

void FADD64M(u32 mem) {
	write8(0xdc);
	ModRm(0, 0, 5);
	write32(mem);
}

void FADD32M(u32 mem) {
	write8(0xd8);
	ModRm(0, 0, 5);
	write32(mem);
}

void FADDP32RtoST0(u8 reg) {
	write8(0xde);
	write8(0xc0 + reg);
}

void FSUB64M(u32 mem) {
	write8(0xdc);
	ModRm(0, 4, 5);
	write32(mem);
}

void FSUB32M(u32 mem) {
	write8(0xd8);
	ModRm(0, 4, 5);
	write32(mem);
}

void FMUL64M(u32 mem) {
	write8(0xdc);
	ModRm(0, 1, 5);
	write32(mem);
}

void FMUL32M(u32 mem) {
	write8(0xd8);
	ModRm(0, 1, 5);
	write32(mem);
}

void  FMUL32ST0toR(u8 reg) {
	write8(0xd8);
	write8(0xc8 + reg);
}

void FMULP32RtoST0(u8 reg) {
	write8(0xde);
	write8(0xc8 + reg);
}

void FDIV64M(u32 mem) {
	write8(0xdc);
	ModRm(0, 0x6, 5);
	write32(mem);
}

void FDIV32M(u32 mem) {
	write8(0xd8);
	ModRm(0, 0x6, 5);
	write32(mem);
}

void FDIVP32RtoST0(u8 reg) {
	write8(0xde);
	write8(0xf8 + reg);
}

void FSINCOS32() {
	write16(0xfbd9);
}

void FSQRT32() {
	write16(0xfad9);
}
