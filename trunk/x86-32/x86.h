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

#ifndef __IX86_H__
#define __IX86_H__

#include "types.h"

typedef enum {
	EAX = 0,
	ECX = 1,
	EDX = 2,
	EBX = 3,
	ESP = 4,
	EBP = 5,
	ESI = 6,
	EDI = 7,
} x86Reg32Type;

typedef enum {
	AX = 0,
	CX = 1,
	DX = 2,
	BX = 3,
	SP = 4,
	BP = 5,
	SI = 6,
	DI = 7,
} x86Reg16Type;

typedef enum {
	AL = 0,
	CL = 1,
	DL = 2,
	BL = 3,
	AH = 4,
	CH = 5,
	DH = 6,
	BH = 7,
} x86Reg8Type;

u8  *x86ptr;

#define X86REGS     8
#define ModeWrite   0
#define ModeRead    1

typedef struct {
	u32 reg;
	u32 inuse;
	u32 age;
} _x86regs;

_x86regs x86regs[X86REGS];

#define x86link32(link)	\
	*link = (u32)((x86ptr - (s8*)link) - 4); \

#define x86link8(link) \
	*link = (u8)((x86ptr - (s8*)link) - 1);	\

#define ModRm(mod, rm, ro) \
	write8((u8)((mod << 6) | ((rm & 7) << 3) | (ro & 7)));

#define SibSb(ss, rm, index) \
	write8((u8)((ss << 6) | (rm << 3) | (index)));

#define  write8(value) (*((u8*)x86ptr)++ = value);
#define write16(value) (*((u16*)x86ptr)++ = value);
#define write32(value) (*((u32*)x86ptr)++ = value);

void MOV32MtoR(x86Reg32Type reg, u32 mem);
void MOV32ItoR(x86Reg32Type reg, u32 imm);

void MOV32RtoM(u32 mem, x86Reg32Type reg);
void MOV32ItoM(u32 mem, u32 imm);

void MOV32RtoR(x86Reg32Type dst, x86Reg32Type src);
void MOV16RtoR(x86Reg32Type dst, x86Reg32Type src);

void MOV8ItoM(u32 mem, u8 imm);

void MOVZX32R16toR(x86Reg32Type dst, x86Reg32Type src);
void MOVZX32M16toR(x86Reg32Type reg, u32 mem);
void MOVZX32R8toR(x86Reg32Type dst, x86Reg32Type src);

void MOVSX32R16toR(x86Reg32Type dst, x86Reg32Type src);
void MOVSX32M16toR(x86Reg32Type reg, u32 mem);
void MOVSX32R8toR(x86Reg32Type dst, x86Reg32Type src);
void MOVSX32M8toR(x86Reg32Type reg, u32 mem);

void XCHG32MtoR(x86Reg32Type reg, u32 mem);
void XCHG32RtoR(x86Reg32Type dst, x86Reg32Type src);

void LEA32IRtoR(x86Reg32Type dst, x86Reg32Type src, u32 imm);
void LEA32RRtoR(x86Reg32Type dst, x86Reg32Type src1, x86Reg32Type src2);

void MUL32R(x86Reg32Type reg);
void IMUL32R(x86Reg32Type reg);

void DIV32R(x86Reg32Type reg);
void IDIV32R(x86Reg32Type reg);

void ADD32RtoR(x86Reg32Type dst, x86Reg32Type src);
void ADD32ItoR(x86Reg32Type reg, u32 imm);
void ADD32RtoM(u32 mem, x86Reg32Type reg);
void ADD32ItoM(u32 mem, u32 imm);

void ADC32RtoR(x86Reg32Type dst, x86Reg32Type src);
void ADC32RtoM(u32 mem, x86Reg32Type reg);

void SUB32RtoR(x86Reg32Type dst, x86Reg32Type src);
void SUB32ItoR(x86Reg32Type reg, u32 imm);
void SUB32ItoM(u32 mem, u32 imm);

void SBB32RtoR(x86Reg32Type dst, x86Reg32Type src);

void DEC32R(x86Reg32Type reg);
void INC32R(x86Reg32Type reg);

void CMP32RtoR(x86Reg32Type dst, x86Reg32Type src);
void CMP32ItoR(x86Reg32Type reg, u32 imm);
void CMP32ItoM(u32 mem, u32 imm);
void CMP16ItoM(u32 mem, u16 imm);

void CMP8RtoR(x86Reg8Type dst, x86Reg8Type src);

void TEST32RtoR(x86Reg32Type dst, x86Reg32Type src);
void TEST32ItoR(x86Reg32Type reg, u32 imm);
void TEST32ItoM(u32 mem, u32 imm);

void BT32ItoM(u32 mem, u8 imm);

void AND32RtoR(x86Reg32Type dst, x86Reg32Type src);
void AND32ItoR(x86Reg32Type reg, u32 imm);
void AND32ItoM(u32 mem, u32 imm);

void XOR32RtoR(x86Reg32Type dst, x86Reg32Type src);
void XOR32ItoR(x86Reg32Type reg, u32 imm);
void XOR32ItoM(u32 mem, u32 imm);

void OR32RtoR(x86Reg32Type dst, x86Reg32Type src);
void OR32ItoR(x86Reg32Type reg, u32 imm);
void OR32ItoM(u32 mem, u32 imm);

void NOT32R(x86Reg32Type reg);

void NEG32R(x86Reg32Type reg);

void ROL32ItoR(x86Reg32Type reg, u8 imm);
void ROR32ItoR(x86Reg32Type reg, u8 imm);

void RCL32ItoR(x86Reg32Type reg, u8 imm);
void RCR32ItoR(x86Reg32Type reg, u8 imm);

void SHL32ItoR(x86Reg32Type reg, u8 imm);
void SHL32CLtoR(x86Reg32Type reg);

void SHR32ItoR(x86Reg32Type reg, u8 imm);
void SHR32CLtoR(x86Reg32Type reg);

void SAR32ItoR(x86Reg32Type reg, u8 imm);
void SAR32CLtoR(x86Reg32Type reg);

void ROL16ItoR(x86Reg32Type reg, u8 imm);

u8* JO8(u8 mem);
u8* JNO8(u8 mem);
u8* JB8(u8 mem);
u8* JAE8(u8 mem);
u8* JE8(u8 mem);
u8* JZ8(u8 mem);
u8* JNE8(u8 mem);
u8* JNZ8(u8 mem);
u8* JBE8(u8 mem);
u8* JA8(u8 mem);
u8* JS8(u8 mem);
u8* JNS8(u8 mem);
u8* JP8(u8 mem);
u8* JNP8(u8 mem);
u8* JL8(u8 mem);
u8* JNGE8(u8 mem);
u8* JGE8(u8 mem);
u8* JNL8(u8 mem);
u8* JLE8(u8 mem);
u8* JNG8(u8 mem);
u8* JG8(u8 mem);
u8* JNLE8(u8 mem);
u8* JMP8(u8 mem);

u32* JO32(u32 mem);
u32* JNO32(u32 mem);
u32* JB32(u32 mem);
u32* JAE32(u32 mem);
u32* JE32(u32 mem);
u32* JZ32(u32 mem);
u32* JNE32(u32 mem);
u32* JNZ32(u32 mem);
u32* JBE32(u32 mem);
u32* JA32(u32 mem);
u32* JS32(u32 mem);
u32* JNS32(u32 mem);
u32* JP32(u32 mem);
u32* JNP32(u32 mem);
u32* JL32(u32 mem);
u32* JNGE32(u32 mem);
u32* JGE32(u32 mem);
u32* JNL32(u32 mem);
u32* JNG32(u32 mem);
u32* JLE32(u32 mem);
u32* JG32(u32 mem);
u32* JNLE32(u32 mem);
u32* JMP32(u32 mem);
u32* CALL32(u32 mem);

void RET();

void SETO8R(x86Reg32Type reg);
void SETNO8R(x86Reg32Type reg);
void SETB8R(x86Reg32Type reg);
void SETAE8R(x86Reg32Type reg);
void SETE8R(x86Reg32Type reg);
void SETZ8R(x86Reg32Type reg);
void SETNE8R(x86Reg32Type reg);
void SETNZ8R(x86Reg32Type reg);
void SETBE8R(x86Reg32Type reg);
void SETA8R(x86Reg32Type reg);
void SETS8R(x86Reg32Type reg);
void SETNS8R(x86Reg32Type reg);
void SETP8R(x86Reg32Type reg);
void SETNP8R(x86Reg32Type reg);
void SETL8R(x86Reg32Type reg);
void SETNGE8R(x86Reg32Type reg);
void SETGE8R(x86Reg32Type reg);
void SETNL8R(x86Reg32Type reg);
void SETNG8R(x86Reg32Type reg);
void SETLE8R(x86Reg32Type reg);
void SETG8R(x86Reg32Type reg);
void SETNLE8R(x86Reg32Type reg);

void SETO8M(u32 mem);
void SETNO8M(u32 mem);
void SETB8M(u32 mem);
void SETAE8M(u32 mem);
void SETE8M(u32 mem);
void SETZ8M(u32 mem);
void SETNE8M(u32 mem);
void SETNZ8M(u32 mem);
void SETBE8M(u32 mem);
void SETA8M(u32 mem);
void SETS8M(u32 mem);
void SETNS8M(u32 mem);
void SETP8M(u32 mem);
void SETNP8M(u32 mem);
void SETL8M(u32 mem);
void SETNGE8M(u32 mem);
void SETGE8M(u32 mem);
void SETNL8M(u32 mem);
void SETNG8M(u32 mem);
void SETLE8M(u32 mem);
void SETG8M(u32 mem);
void SETNLE8M(u32 mem);

void PUSH32R(x86Reg32Type reg);
void POP32R(x86Reg32Type reg);

void PUSHA32();
void POPA32();

#endif