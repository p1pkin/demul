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

#include "x86.h"

void MOV32MtoR(x86Reg32Type reg, u32 mem) {
	if (reg == EAX) {
		write8(0xa1);
	} else {
		write8(0x8b);
		ModRm(0, reg, 5);
	}

	write32(mem);
}

void MOV32ItoR(x86Reg32Type reg, u32 imm) {
	if (imm == 0) {
		XOR32RtoR(reg, reg);
	} else if (imm == 1) {
		XOR32RtoR(reg, reg);
		INC32R(reg);
	} else if (imm == -1) {
		XOR32RtoR(reg, reg);
		DEC32R(reg);
	} else if ((imm < 128) || (imm > 0xffffff81)) {
		XOR32RtoR(reg, reg);
		OR32ItoR(reg, imm);
	} else {
		write8(0xb8 | reg);
		write32(imm);
	}
}

void MOV32RtoM(u32 mem, x86Reg32Type reg) {
	if (reg == EAX) {
		write8(0xa3);
	} else {
		write8(0x89);
		ModRm(0, reg, 5);
	}

	write32(mem);
}

void MOV32ItoM(u32 mem, u32 imm) {
	write8(0xc7);
	ModRm(0, 0, 5);
	write32(mem);
	write32(imm);
}

void MOV8ItoM(u32 mem, u8 imm) {
	write8(0xc6);
	ModRm(0, 0, 5);
	write32(mem);
	write8(imm);
}

void MOV32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	if (dst == src) return;

	write8(0x89);
	ModRm(3, src, dst);
}

void MOV16RtoR(x86Reg32Type dst, x86Reg32Type src) {
	if (dst == src) return;

	write16(0x8966);
	ModRm(3, src, dst);
}

void MOVZX32R16toR(x86Reg32Type dst, x86Reg32Type src) {
	write16(0xb70f);
	ModRm(3, dst, src);
}

void MOVZX32M16toR(x86Reg32Type reg, u32 mem) {
	write16(0xb70f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVZX32R8toR(x86Reg32Type dst, x86Reg32Type src) {
	write16(0xb60f);
	ModRm(3, dst, src);
}

void MOVSX32R16toR(x86Reg32Type dst, x86Reg32Type src) {
	write16(0xbf0f);
	ModRm(3, dst, src);
}

void MOVSX32M16toR(x86Reg32Type reg, u32 mem) {
	write16(0xbf0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void MOVSX32R8toR(x86Reg32Type dst, x86Reg32Type src) {
	write16(0xbe0f);
	ModRm(3, dst, src);
}

void MOVSX32M8toR(x86Reg32Type reg, u32 mem) {
	write16(0xbe0f);
	ModRm(0, reg, 5);
	write32(mem);
}

void XCHG32MtoR(x86Reg32Type reg, u32 mem) {
	write8(0x87);
	ModRm(0, reg, 5);
	write32(mem);
}

void XCHG32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	if (dst == src) return;

	if (dst == EAX) {
		write8(0x90 | src);
	} else {
		write8(0x87);
		ModRm(3, src, dst);
	}
}

void LEA32IRtoR(x86Reg32Type dst, x86Reg32Type src, u32 imm) {
	write8(0x8d);

	if ((imm == 0) && (src != EBP)) {
		ModRm(0, dst, src);
	} else if ((imm < 128) || (imm > 0xffffff81)) {
		ModRm(1, dst, src);
		write8(imm);
	} else {
		ModRm(2, dst, src);
		write32(imm);
	}
}

void LEA32RRtoR(x86Reg32Type dst, x86Reg32Type src1, x86Reg32Type src2) {
	write8(0x8d);

	if (src1 != EBP) {
		ModRm(0, dst, 4);
		SibSb(0, src2, src1);
	} else if (src2 != EBP) {
		ModRm(0, dst, 4);
		SibSb(0, src1, src2);
	} else {
		ModRm(1, dst, 4);
		SibSb(0, src1, src2);
		write8(0);
	}
}

void MUL32R(x86Reg32Type reg) {
	write8(0xF7);
	ModRm(3, 4, reg);
}

void IMUL32R(x86Reg32Type reg) {
	write8(0xF7);
	ModRm(3, 5, reg);
}

void DIV32R(x86Reg32Type reg) {
	write8(0xF7);
	ModRm(3, 6, reg);
}

void IDIV32R(x86Reg32Type reg) {
	write8(0xF7);
	ModRm(3, 7, reg);
}

void ADD32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x01);
	ModRm(3, src, dst);
}

void ADD32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 0, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x05);
		} else {
			write8(0x81);
			ModRm(3, 0, reg);
		}

		write32(imm);
	}
}

void ADD32RtoM(u32 mem, x86Reg32Type reg) {
	write8(0x01);
	ModRm(0, reg, 5);
	write32(mem);
}

void ADD32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 0, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 0, 5);
		write32(mem);
		write32(imm);
	}
}

void ADC32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x11);
	ModRm(3, src, dst);
}

void ADC32RtoM(u32 mem, x86Reg32Type reg) {
	write8(0x11);
	ModRm(0, reg, 5);
	write32(mem);
}

void SUB32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x29);
	ModRm(3, src, dst);
}

void SUB32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 5, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x2d);
		} else {
			write8(0x81);
			ModRm(3, 5, reg);
		}

		write32(imm);
	}
}

void SUB32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 5, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 5, 5);
		write32(mem);
		write32(imm);
	}
}

void SBB32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x19);
	ModRm(3, src, dst);
}


void DEC32R(x86Reg32Type reg) {
	write8(0x48 | reg);
}

void INC32R(x86Reg32Type reg) {
	write8(0x40 | reg);
}

void CMP32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x39);
	ModRm(3, src, dst);
}

void CMP32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 7, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x3d);
		} else {
			write8(0x81);
			ModRm(3, 7, reg);
		}

		write32(imm);
	}
}

void CMP32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 7, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 7, 5);
		write32(mem);
		write32(imm);
	}
}

void CMP16ItoM(u32 mem, u16 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write16(0x8366);
		ModRm(0, 7, 5);
		write32(mem);
		write8((u8)imm);
	} else {
		write16(0x8166);
		ModRm(0, 7, 5);
		write32(mem);
		write16(imm);
	}
}

void CMP8RtoR(x86Reg8Type dst, x86Reg8Type src) {
	write8(0x3a);
	ModRm(3, src, dst);
}

void TEST32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x85);
	ModRm(3, src, dst);
}

void TEST32ItoR(x86Reg32Type reg, u32 imm) {
	if (reg == EAX) {
		write8(0xa9);
	} else {
		write8(0xf7);
		ModRm(3, 0, reg);
	}

	write32(imm);
}

void TEST32ItoM(u32 mem, u32 imm) {
	write8(0xf7);
	ModRm(0, 0, 5);
	write32(mem);
	write32(imm);
}

void AND32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x21);
	ModRm(3, src, dst);
}

void AND32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 4, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x25);
		} else {
			write8(0x81);
			ModRm(3, 4, reg);
		}

		write32(imm);
	}
}

void BT32ItoM(u32 mem, u8 imm) {
	write16(0xba0f);
	ModRm(0, 4, 5);
	write32(mem);
	write8(imm);
}

void AND32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 4, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 4, 5);
		write32(mem);
		write32(imm);
	}
}

void XOR32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x31);
	ModRm(3, src, dst);
}

void XOR32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 6, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x35);
		} else {
			write8(0x81);
			ModRm(3, 6, reg);
		}

		write32(imm);
	}
}

void XOR32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 6, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 6, 5);
		write32(mem);
		write32(imm);
	}
}

void OR32RtoR(x86Reg32Type dst, x86Reg32Type src) {
	write8(0x09);
	ModRm(3, src, dst);
}

void OR32ItoR(x86Reg32Type reg, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(3, 1, reg);
		write8(imm);
	} else {
		if (reg == EAX) {
			write8(0x0d);
		} else {
			write8(0x81);
			ModRm(3, 1, reg);
		}
		write32(imm);
	}
}

void OR32ItoM(u32 mem, u32 imm) {
	if ((imm < 128) || (imm > 0xffffff81)) {
		write8(0x83);
		ModRm(0, 1, 5);
		write32(mem);
		write8(imm);
	} else {
		write8(0x81);
		ModRm(0, 1, 5);
		write32(mem);
		write32(imm);
	}
}

void NOT32R(x86Reg32Type reg) {
	write8(0xf7);
	ModRm(3, 2, reg);
}

void NEG32R(x86Reg32Type reg) {
	write8(0xf7);
	ModRm(3, 3, reg);
}

void ROL32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 0, reg);
	} else {
		write8(0xc1);
		ModRm(3, 0, reg);
		write8(imm);
	}
}

void ROR32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 1, reg);
	} else {
		write8(0xc1);
		ModRm(3, 1, reg);
		write8(imm);
	}
}

void RCL32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 2, reg);
	} else {
		write8(0xc1);
		ModRm(3, 2, reg);
		write8(imm);
	}
}

void RCR32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 3, reg);
	} else {
		write8(0xc1);
		ModRm(3, 3, reg);
		write8(imm);
	}
}

void SHL32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 4, reg);
	} else {
		write8(0xc1);
		ModRm(3, 4, reg);
		write8(imm);
	}
}

void SHL32CLtoR(x86Reg32Type reg) {
	write8(0xd3);
	ModRm(3, 4, reg);
}

void SHR32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write8(0xd1);
		ModRm(3, 5, reg);
	} else {
		write8(0xc1);
		ModRm(3, 5, reg);
		write8(imm);
	}
}

void SHR32CLtoR(x86Reg32Type reg) {
	write8(0xd3);
	ModRm(3, 5, reg);
}

void SAR32ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (reg == 1) {
		write8(0xd1);
		ModRm(3, 7, reg);
	} else {
		write8(0xc1);
		ModRm(3, 7, reg);
		write8(imm);
	}
}

void SAR32CLtoR(x86Reg32Type reg) {
	write8(0xd3);
	ModRm(3, 7, reg);
}

void ROL16ItoR(x86Reg32Type reg, u8 imm) {
	if (imm == 0) return;

	if (imm == 1) {
		write16(0xd166);
		ModRm(3, 0, reg);
	} else {
		write16(0xc166);
		ModRm(3, 0, reg);
		write8(imm);
	}
}

u8* JO8(u8 mem) {
	write8(0x70);
	write8(mem);
	return x86ptr - 1;
}

u8* JNO8(u8 mem) {
	write8(0x71);
	write8(mem);
	return x86ptr - 1;
}

u8* JB8(u8 mem) {
	write8(0x72);
	write8(mem);
	return x86ptr - 1;
}

u8* JAE8(u8 mem) {
	write8(0x73);
	write8(mem);
	return x86ptr - 1;
}

u8* JE8(u8 mem) {
	write8(0x74);
	write8(mem);
	return x86ptr - 1;
}

u8* JZ8(u8 mem) {
	write8(0x74);
	write8(mem);
	return x86ptr - 1;
}

u8* JNE8(u8 mem) {
	write8(0x75);
	write8(mem);
	return x86ptr - 1;
}

u8* JNZ8(u8 mem) {
	write8(0x75);
	write8(mem);
	return x86ptr - 1;
}

u8* JBE8(u8 mem) {
	write8(0x76);
	write8(mem);
	return x86ptr - 1;
}

u8* JA8(u8 mem) {
	write8(0x77);
	write8(mem);
	return x86ptr - 1;
}

u8* JS8(u8 mem) {
	write8(0x78);
	write8(mem);
	return x86ptr - 1;
}

u8* JNS8(u8 mem) {
	write8(0x79);
	write8(mem);
	return x86ptr - 1;
}

u8* JP8(u8 mem) {
	write8(0x7a);
	write8(mem);
	return x86ptr - 1;
}

u8* JNP8(u8 mem) {
	write8(0x7b);
	write8(mem);
	return x86ptr - 1;
}

u8* JL8(u8 mem) {
	write8(0x7c);
	write8(mem);
	return x86ptr - 1;
}

u8* JNGE8(u8 mem) {
	write8(0x7c);
	write8(mem);
	return x86ptr - 1;
}

u8* JGE8(u8 mem) {
	write8(0x7d);
	write8(mem);
	return x86ptr - 1;
}

u8* JNL8(u8 mem) {
	write8(0x7d);
	write8(mem);
	return x86ptr - 1;
}

u8* JLE8(u8 mem) {
	write8(0x7e);
	write8(mem);
	return x86ptr - 1;
}

u8* JNG8(u8 mem) {
	write8(0x7e);
	write8(mem);
	return x86ptr - 1;
}

u8* JG8(u8 mem) {
	write8(0x7f);
	write8(mem);
	return x86ptr - 1;
}

u8* JNLE8(u8 mem) {
	write8(0x7f);
	write8(mem);
	return x86ptr - 1;
}

u8* JMP8(u8 mem) {
	write8(0xeb);
	write8(mem);
	return x86ptr - 1;
}

u32* JO32(u32 mem) {
	write8(0x0f);
	write8(0x80);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNO32(u32 mem) {
	write8(0x0f);
	write8(0x81);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JB32(u32 mem) {
	write8(0x0f);
	write8(0x82);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JAE32(u32 mem) {
	write8(0x0f);
	write8(0x83);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JE32(u32 mem) {
	write8(0x0f);
	write8(0x84);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JZ32(u32 mem) {
	write8(0x0f);
	write8(0x84);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNE32(u32 mem) {
	write8(0x0f);
	write8(0x85);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNZ32(u32 mem) {
	write8(0x0f);
	write8(0x85);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JBE32(u32 mem) {
	write8(0x0f);
	write8(0x86);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JA32(u32 mem) {
	write8(0x0f);
	write8(0x87);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JS32(u32 mem) {
	write8(0x88);
	write8(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNS32(u32 mem) {
	write8(0x89);
	write8(mem);
	return (u32*)(x86ptr - 4);
}

u32* JP32(u32 mem) {
	write8(0x8a);
	write8(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNP32(u32 mem) {
	write8(0x8b);
	write8(mem);
	return (u32*)(x86ptr - 4);
}

u32* JL32(u32 mem) {
	write8(0x0f);
	write8(0x8c);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNGE32(u32 mem) {
	write8(0x0f);
	write8(0x8c);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JGE32(u32 mem) {
	write8(0x0f);
	write8(0x8d);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNL32(u32 mem) {
	write8(0x0f);
	write8(0x8d);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNG32(u32 mem) {
	write8(0x0f);
	write8(0x8e);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JLE32(u32 mem) {
	write8(0x0f);
	write8(0x8e);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JG32(u32 mem) {
	write8(0x0f);
	write8(0x8f);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JNLE32(u32 mem) {
	write8(0x0f);
	write8(0x8f);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* JMP32(u32 mem) {
	write8(0xe9);
	write32(mem);
	return (u32*)(x86ptr - 4);
}

u32* CALL32(u32 mem) {
	write8(0xe8);
	write32(mem - ((u32)x86ptr + 4));
	return (u32*)(x86ptr - 4);
}

void RET() {
	write8(0xc3);
}

void SETO8R(x86Reg32Type reg) {
	write16(0x900f);
	ModRm(3, 0, reg);
}

void SETNO8R(x86Reg32Type reg) {
	write16(0x910f);
	ModRm(3, 0, reg);
}

void SETB8R(x86Reg32Type reg) {
	write16(0x920f);
	ModRm(3, 0, reg);
}

void SETAE8R(x86Reg32Type reg) {
	write16(0x930f);
	ModRm(3, 0, reg);
}

void SETE8R(x86Reg32Type reg) {
	write16(0x940f);
	ModRm(3, 0, reg);
}

void SETZ8R(x86Reg32Type reg) {
	write16(0x940f);
	ModRm(3, 0, reg);
}

void SETNE8R(x86Reg32Type reg) {
	write16(0x950f);
	ModRm(3, 0, reg);
}

void SETNZ8R(x86Reg8Type reg) {
	write16(0x950f);
	ModRm(3, 0, reg);
}

void SETBE8R(x86Reg8Type reg) {
	write16(0x960f);
	ModRm(3, 0, reg);
}

void SETA8R(x86Reg8Type reg) {
	write16(0x970f);
	ModRm(3, 0, reg);
}

void SETS8R(x86Reg8Type reg) {
	write16(0x980f);
	ModRm(3, 0, reg);
}

void SETNS8R(x86Reg8Type reg) {
	write16(0x990f);
	ModRm(3, 0, reg);
}

void SETP8R(x86Reg8Type reg) {
	write16(0x9a0f);
	ModRm(3, 0, reg);
}

void SETNP8R(x86Reg8Type reg) {
	write16(0x9b0f);
	ModRm(3, 0, reg);
}

void SETL8R(x86Reg8Type reg) {
	write16(0x9c0f);
	ModRm(3, 0, reg);
}

void SETNGE8R(x86Reg8Type reg) {
	write16(0x9c0f);
	ModRm(3, 0, reg);
}

void SETGE8R(x86Reg8Type reg) {
	write16(0x9d0f);
	ModRm(3, 0, reg);
}

void SETNL8R(x86Reg8Type reg) {
	write16(0x9d0f);
	ModRm(3, 0, reg);
}

void SETNG8R(x86Reg8Type reg) {
	write16(0x9e0f);
	ModRm(3, 0, reg);
}

void SETLE8R(x86Reg8Type reg) {
	write16(0x9e0f);
	ModRm(3, 0, reg);
}

void SETG8R(x86Reg8Type reg) {
	write16(0x9f0f);
	ModRm(3, 0, reg);
}

void SETNLE8R(x86Reg8Type reg) {
	write16(0x9f0f);
	ModRm(3, 0, reg);
}

void SETO8M(u32 mem) {
	write16(0x900f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNO8M(u32 mem) {
	write16(0x910f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETB8M(u32 mem) {
	write16(0x920f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETAE8M(u32 mem) {
	write16(0x930f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETE8M(u32 mem) {
	write16(0x940f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETZ8M(u32 mem) {
	write16(0x940f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNE8M(u32 mem) {
	write16(0x950f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNZ8M(u32 mem) {
	write16(0x950f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETBE8M(u32 mem) {
	write16(0x960f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETA8M(u32 mem) {
	write16(0x970f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETS8M(u32 mem) {
	write16(0x980f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNS8M(u32 mem) {
	write16(0x990f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETP8M(u32 mem) {
	write16(0x9a0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNP8M(u32 mem) {
	write16(0x9b0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETL8M(u32 mem) {
	write16(0x9c0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNGE8M(u32 mem) {
	write16(0x9c0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETGE8M(u32 mem) {
	write16(0x9d0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNL8M(u32 mem) {
	write16(0x9d0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNG8M(u32 mem) {
	write16(0x9e0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETLE8M(u32 mem) {
	write16(0x9e0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETG8M(u32 mem) {
	write16(0x9f0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void SETNLE8M(u32 mem) {
	write16(0x9f0f);
	ModRm(0, 0, 5);
	write32(mem);
}

void PUSH32R(x86Reg32Type reg) {
	write8(0x50 | reg);
}

void POP32R(x86Reg32Type reg) {
	write8(0x58 | reg);
}

void PUSHA32() {
	write8(0x60);
}

void POPA32() {
	write8(0x61);
}
