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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dynarec.h"

#include "cpuid.h"
#include "x86.h"
#include "fpu.h"
#include "sse.h"
#include "mmx.h"
#include "3dnow.h"

#include "counters.h"
#include "memory.h"
#include "sh4.h"
#include "mmu.h"

#include "onchip.h"
#include "cache.h"
#include "asic.h"
#include "aica.h"
#include "gdrom.h"
#include "rts.h"
#include "modem.h"
#include "gaps.h"

#include "plugins.h"
#include "debug.h"
#include "demul.h"

static s32 iount;

static int GetX86reg(int reg, int mode) {
	static u32 age = 0;

	x86Reg32Type i;

	for (i = X86REGS - 1; i >= 0; i--) {
		if (i == EAX) continue;
		if (i == ESP) continue;

		if (x86regs[i].inuse) {
			if (x86regs[i].reg == reg) {
				x86regs[i].age = ++age;
				return i;
			}
		}
	}

	for (i = X86REGS - 1; i >= 0; i--) {
		if (i == EAX) continue;
		if (i == ESP) continue;

		if (x86regs[i].inuse == 0) {
			x86regs[i].reg = reg;
			x86regs[i].inuse = 1;
			x86regs[i].age = ++age;

			if (mode == ModeRead)
				MOV32MtoR(i, (u32) & sh4.r[reg]);
			return i;
		}
	}

	{
		u32 Age = -1;
		u32 Reg = 0;

		for (i = X86REGS - 1; i >= 0; i--) {
			if (i == EAX) continue;
			if (i == ESP) continue;

			if (Age > x86regs[i].age) {
				Age = x86regs[i].age;
				Reg = i;
			}
		}

		MOV32RtoM((u32) & sh4.r[x86regs[Reg].reg], Reg);

		x86regs[Reg].reg = reg;
		x86regs[Reg].inuse = 1;
		x86regs[Reg].age = ++age;

		if (mode == ModeRead)
			MOV32MtoR(Reg, (u32) & sh4.r[reg]);

		return Reg;
	}
}

static int isMapX86Reg(int reg) {
	return x86regs[reg].inuse == 1;
}

static void unMapSh4Reg(int reg) {
	int i;

	for (i = X86REGS - 1; i >= 0; i--) {
		if (i == EAX) continue;
		if (i == ESP) continue;

		if (x86regs[i].inuse) {
			if (x86regs[i].reg == reg) {
				x86regs[i].inuse = 0;
				return;
			}
		}
	}
}

void Flushall() {
	int i;

	for (i = 0; i < X86REGS; i++) {
		if (i == EAX) continue;
		if (i == ESP) continue;

		if (x86regs[i].inuse == 1) {
			x86regs[i].inuse = 0;
			MOV32RtoM((u32) & sh4.r[x86regs[i].reg], i);
		}
	}

	for (i = 0; i < 16; i++) {
		if (!isMappedConst(i)) continue;

		invalidConst(i);
		MOV32ItoM((u32) & sh4.r[i], ConstantMap[i].constant);
	}
}

static void MemRead8Translate(u32 mem, x86Reg32Type Rn) {
	if (((mem & 0xe0000000) != 0xe0000000) &&
		(((mem & 0xe0000000) == 0x00000000) ||
		 ((mem & 0x40000000) != 0x00000000))) {
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, mem);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		u8 *p = (u8*)memAccess[mem >> 16];

		if ((int)p > 0x7f) {
			MOVSX32M8toR(Rn, (u32)(p + (mem & 0xffff)));
		} else {
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, mem);

			switch ((int)p) {
			case 1:
				CALL32((u32) & ReadCacheRam8);
				break;
			case 2:
				CALL32((u32) & ReadAsic8);
				break;
			case 3:
				CALL32((u32) & ReadModem8);
				break;
			case 8:
				CALL32((u32) & ReadOnChip8);
				break;
			default:
				XOR32RtoR(EAX, EAX);
				break;
			}

			if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

			MOVSX32R8toR(Rn, EAX);
		}
	}
}

static void MemRead16Translate(u32 mem, x86Reg32Type Rn) {
	if (((mem & 0xe0000000) != 0xe0000000) &&
		(((mem & 0xe0000000) == 0x00000000) ||
		 ((mem & 0x40000000) != 0x00000000))) {
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, mem);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		u8 *p = (u8*)memAccess[mem >> 16];

		if ((int)p > 0x7f) {
			MOVSX32M16toR(Rn, (u32)(p + (mem & 0xffff)));
		} else {
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, mem);

			switch ((int)p) {
			case 1:
				CALL32((u32) & ReadCacheRam16);
				break;
			case 2:
				CALL32((u32) & ReadAsic16);
				break;
			case 8:
				CALL32((u32) & ReadOnChip16);
				break;
			default:
				XOR32RtoR(EAX, EAX);
				break;
			}

			if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

			MOVSX32R16toR(Rn, EAX);
		}
	}
}

static void MemRead32Translate(u32 mem, x86Reg32Type Rn) {
	if (((mem & 0xe0000000) != 0xe0000000) &&
		(((mem & 0xe0000000) == 0x00000000) ||
		 ((mem & 0x40000000) != 0x00000000))) {
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, mem);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		u8 *p = (u8*)memAccess[mem >> 16];

		if ((int)p > 0x7f) {
			MOV32MtoR(Rn, (u32)(p + (mem & 0xffff)));
		} else {
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, mem);

			switch ((int)p) {
			case 1:
				CALL32((u32) & ReadCacheRam32);
				break;
			case 2:
				CALL32((u32) & ReadAsic32);
				break;
			case 4:
				CALL32((u32) & ReadAica32);
				break;
			case 5:
				CALL32((u32) & ReadRTS32);
				break;
			case 6:
				CALL32((u32) & ReadGaps32);
				break;
			case 8:
				CALL32((u32) & ReadOnChip32);
				break;
			case 11:
				CALL32((u32) & ReadItlbAddr32);
				break;
			case 12:
				CALL32((u32) & ReadItlbData32);
				break;
			case 13:
				XOR32RtoR(EAX, EAX);
				break;
			case 15:
				CALL32((u32) & ReadUtlbAddr32);
				break;
			case 16:
				CALL32((u32) & ReadUtlbData32);
				break;
			default:
				XOR32RtoR(EAX, EAX);
				break;
			}

			if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

			MOV32RtoR(Rn, EAX);
		}
	}
}

static void MemRead64Translate(u32 mem, u32 out) {
	if (((mem & 0xe0000000) != 0xe0000000) &&
		(((mem & 0xe0000000) == 0x00000000) ||
		 ((mem & 0x40000000) != 0x00000000))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, mem);
		MOV32ItoR(EDX, out);
		CALL32((u32) & memRead64);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		u8 *p = (u8*)memAccess[mem >> 16];

		if ((int)p > 0x7f) {
			if (cpuSSE) {
				MOVLPS64MtoXMM(XMM0, (u32)(p + (mem & 0xffff)));
				MOVLPS64XMMtoM(out, XMM0);
			} else {
				MOV64MtoMMX(MM0, (u32)(p + (mem & 0xffff)));
				MOV64MMXtoM(out, MM0);

				if (cpu3DNOW) FEMMS();
				else EMMS();
			}
		} else {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, mem);
			MOV32ItoR(EDX, out);

			switch ((int)p) {
			case 1:
				CALL32((u32) & ReadCacheRam64);
				break;
			default:
				MOV32ItoR((u32) & out + 0, 0);
				MOV32ItoR((u32) & out + 4, 0);
				break;
			}

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	}
}

static void INLINE FASTCALL NI(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVI(u32 code) {
	u32 i = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	MapConst(n, (u32)(s32)(s8)i);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWI(u32 code) {
	u32 d = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	u32 mem = block_info.pc + (d << 1) + 4;

	invalidConst(n);

	MOVSX32M16toR(Rn, (u32)((memAccess[mem >> 16]) + (mem & 0xffff)));

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLI(u32 code) {
	u32 d = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	u32 mem = (block_info.pc & 0xfffffffc) + (d << 2) + 4;

	invalidConst(n);

	MOV32MtoR(Rn, (u32)((memAccess[mem >> 16]) + (mem & 0xffff)));

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOV(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead8Translate(ConstantMap[m].constant, Rn);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead16Translate(ConstantMap[m].constant, Rn);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead32Translate(ConstantMap[m].constant, Rn);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 1);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 1);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		DEC32R(Rn);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant - 1);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 1);
	} else if (n != m) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		DEC32R(Rn);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				DEC32R(ECX);                break;
			case EDX:   DEC32R(ECX);                break;
			default:    DEC32R(ECX);
				MOV32RtoR(EDX, Rm);         break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				DEC32R(ECX);                break;
			case EDX:   MOV32RtoR(ECX, EDX);
				DEC32R(ECX);                break;
			default:    LEA32IRtoR(ECX, EDX, -1);
				MOV32RtoR(EDX, Rm);        break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, -1);        break;
			case EDX:   LEA32IRtoR(ECX, Rn, -1);        break;
			default:    LEA32IRtoR(ECX, Rn, -1);
				MOV32RtoR(EDX, Rm);            break;
			}
			break;
		}

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		DEC32R(Rn);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 2);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 2);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant - 2);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 2);
	} else if (n != m) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		SUB32ItoR(Rn, 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				SUB32ItoR(ECX, 2);          break;
			case EDX:   SUB32ItoR(ECX, 2);          break;
			default:    SUB32ItoR(ECX, 2);
				MOV32RtoR(EDX, Rm);         break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				SUB32ItoR(ECX, 2);         break;
			case EDX:   MOV32RtoR(ECX, EDX);
				SUB32ItoR(ECX, 2);         break;
			default:    LEA32IRtoR(ECX, EDX, -2);
				MOV32RtoR(EDX, Rm);        break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, -2);        break;
			case EDX:   LEA32IRtoR(ECX, Rn, -2);        break;
			default:    LEA32IRtoR(ECX, Rn, -2);
				MOV32RtoR(EDX, Rm);            break;
			}
			break;
		}

		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		SUB32ItoR(Rn, 2);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLM(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else if (n != m) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, Rm);     break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, Rm);    break;
			}
			break;
		}

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				SUB32ItoR(ECX, 4);          break;
			case EDX:   SUB32ItoR(ECX, 4);          break;
			default:    SUB32ItoR(ECX, 4);
				MOV32RtoR(EDX, Rm);         break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				SUB32ItoR(ECX, 4);         break;
			case EDX:   MOV32RtoR(ECX, EDX);
				SUB32ItoR(ECX, 4);         break;
			default:    LEA32IRtoR(ECX, EDX, -4);
				MOV32RtoR(EDX, Rm);        break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, -4);        break;
			case EDX:   LEA32IRtoR(ECX, Rn, -4);        break;
			default:    LEA32IRtoR(ECX, Rn, -4);
				MOV32RtoR(EDX, Rm);            break;
			}
			break;
		}

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		SUB32ItoR(Rn, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead8Translate(ConstantMap[m].constant, Rn);

		if (m != n) MapConst(m, ConstantMap[m].constant + 1);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);

		if (m != n) INC32R(Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead16Translate(ConstantMap[m].constant, Rn);

		if (m != n) MapConst(m, ConstantMap[m].constant + 2);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);

		if (m != n) ADD32ItoR(Rm, 2);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLP(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead32Translate(ConstantMap[m].constant, Rn);

		if (m != n) MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);

		if (m != n) ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 n = ((code >> 4) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 0));
		MOV32ItoR(EDX, ConstantMap[0].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		if (ECX != Rn) LEA32IRtoR(ECX, Rn, (d << 0));
		else ADD32ItoR(ECX, (d << 0));
		MOV32ItoR(EDX, ConstantMap[0].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 0));
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 0));       break;
			case EDX:   ADD32ItoR(ECX, (d << 0));       break;
			default:    ADD32ItoR(ECX, (d << 0));
				MOV32RtoR(EDX, R0);             break;
			}
			break;

		case EDX:
			switch (R0) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 0));      break;
			case EDX:   MOV32RtoR(ECX, EDX);
				ADD32ItoR(ECX, (d << 0));      break;
			default:    LEA32IRtoR(ECX, EDX, (d << 0));
				MOV32RtoR(EDX, R0);            break;
			}
			break;

		default:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, (d << 0));  break;
			case EDX:   LEA32IRtoR(ECX, Rn, (d << 0));  break;
			default:    LEA32IRtoR(ECX, Rn, (d << 0));
				MOV32RtoR(EDX, R0);            break;
			}
			break;
		}

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 n = ((code >> 4) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 1));
		MOV32ItoR(EDX, ConstantMap[0].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		if (ECX != Rn) LEA32IRtoR(ECX, Rn, (d << 1));
		else ADD32ItoR(ECX, (d << 1));
		MOV32ItoR(EDX, ConstantMap[0].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 1));
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 1));       break;
			case EDX:   ADD32ItoR(ECX, (d << 1));       break;
			default:    ADD32ItoR(ECX, (d << 1));
				MOV32RtoR(EDX, R0);             break;
			}
			break;

		case EDX:
			switch (R0) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 1));      break;
			case EDX:   MOV32RtoR(ECX, EDX);
				ADD32ItoR(ECX, (d << 1));      break;
			default:    LEA32IRtoR(ECX, EDX, (d << 1));
				MOV32RtoR(EDX, R0);            break;
			}
			break;

		default:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, (d << 1));  break;
			case EDX:   LEA32IRtoR(ECX, Rn, (d << 1));  break;
			default:    LEA32IRtoR(ECX, Rn, (d << 1));
				MOV32RtoR(EDX, R0);            break;
			}
			break;
		}

		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLS4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 2));
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		if (ECX != Rn) LEA32IRtoR(ECX, Rn, (d << 2));
		else ADD32ItoR(ECX, (d << 2));
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant + (d << 2));
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 2));       break;
			case EDX:   ADD32ItoR(ECX, (d << 2));       break;
			default:    ADD32ItoR(ECX, (d << 2));
				MOV32RtoR(EDX, Rm);             break;
			}
			break;

		case EDX:
			switch (Rm) {
			case ECX:   XCHG32RtoR(EDX, ECX);
				ADD32ItoR(ECX, (d << 2));      break;
			case EDX:   MOV32RtoR(ECX, EDX);
				ADD32ItoR(ECX, (d << 2));      break;
			default:    LEA32IRtoR(ECX, EDX, (d << 2));
				MOV32RtoR(EDX, Rm);            break;
			}
			break;

		default:
			switch (Rm) {
			case ECX:   MOV32RtoR(EDX, ECX);
				LEA32IRtoR(ECX, Rn, (d << 2));  break;
			case EDX:   LEA32IRtoR(ECX, Rn, (d << 2));  break;
			default:    LEA32IRtoR(ECX, Rn, (d << 2));
				MOV32RtoR(EDX, Rm);            break;
			}
			break;
		}

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type R0 = GetX86reg(0, ModeWrite);

		invalidConst(0);

		MemRead8Translate(ConstantMap[m].constant + (d << 0), R0);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeWrite);

		invalidConst(0);

		if ((isMapX86Reg(EDX)) && (R0 != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (R0 != ECX)) PUSH32R(ECX);

		if (ECX != Rm) LEA32IRtoR(ECX, Rm, (d << 0));
		else ADD32ItoR(ECX, (d << 0));
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (R0 != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (R0 != EDX)) POP32R(EDX);

		MOV32RtoR(R0, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type R0 = GetX86reg(0, ModeWrite);

		invalidConst(0);

		MemRead16Translate(ConstantMap[m].constant + (d << 1), R0);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeWrite);

		invalidConst(0);

		if ((isMapX86Reg(EDX)) && (R0 != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (R0 != ECX)) PUSH32R(ECX);

		if (ECX != Rm) LEA32IRtoR(ECX, Rm, (d << 1));
		else ADD32ItoR(ECX, (d << 1));
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (R0 != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (R0 != EDX)) POP32R(EDX);

		MOV32RtoR(R0, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLL4(u32 code) {
	u32 d = ((code >> 0) & 0x0f);
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead32Translate(ConstantMap[m].constant + (d << 2), Rn);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		if (ECX != Rm) LEA32IRtoR(ECX, Rm, (d << 2));
		else ADD32ItoR(ECX, (d << 2));
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[m].constant);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(0))) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(0)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rn, R0);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, R0, ConstantMap[n].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, Rn, ConstantMap[0].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(EAX, Rn, R0);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[m].constant);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(0))) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(0)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rn, R0);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, R0, ConstantMap[n].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, Rn, ConstantMap[0].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(EAX, Rn, R0);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLS0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[m].constant);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(0))) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, Rm);
		MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if ((isMappedConst(0)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rn, R0);
		MOV32ItoR(EDX, ConstantMap[m].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, R0, ConstantMap[n].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32IRtoR(EAX, Rn, ConstantMap[0].constant);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		LEA32RRtoR(EAX, Rn, R0);
		MOV32RtoR(EDX, Rm);
		MOV32RtoR(ECX, EAX);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(m)) && (isMappedConst(0))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead8Translate(ConstantMap[m].constant + ConstantMap[0].constant, Rn);
	} else if (isMappedConst(m)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rm, R0);
		CALL32((u32) & memRead8);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(m)) && (isMappedConst(0))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead16Translate(ConstantMap[m].constant + ConstantMap[0].constant, Rn);
	} else if (isMappedConst(m)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rm, R0);
		CALL32((u32) & memRead16);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLL0(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(m)) && (isMappedConst(0))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MemRead32Translate(ConstantMap[m].constant + ConstantMap[0].constant, Rn);
	} else if (isMappedConst(m)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if ((isMapX86Reg(EDX)) && (Rn != EDX)) PUSH32R(EDX);
		if ((isMapX86Reg(ECX)) && (Rn != ECX)) PUSH32R(ECX);

		LEA32RRtoR(ECX, Rm, R0);
		CALL32((u32) & memRead32);

		if ((isMapX86Reg(ECX)) && (Rn != ECX)) POP32R(ECX);
		if ((isMapX86Reg(EDX)) && (Rn != EDX)) POP32R(EDX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[0].constant);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 0));

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 0));

		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[0].constant);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 1));
		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 1));

		CALL32((u32) & memWrite16);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLSG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EDX, ConstantMap[0].constant);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 2));
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, (d << 2));

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVBLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	x86Reg32Type R0 = GetX86reg(0, ModeWrite);

	invalidConst(0);

	if ((isMapX86Reg(EDX)) && (R0 != EDX)) PUSH32R(EDX);
	if ((isMapX86Reg(ECX)) && (R0 != ECX)) PUSH32R(ECX);

	MOV32MtoR(ECX, (u32) & sh4.gbr);
	ADD32ItoR(ECX, (d << 0));
	CALL32((u32) & memRead8);

	if ((isMapX86Reg(ECX)) && (R0 != ECX)) POP32R(ECX);
	if ((isMapX86Reg(EDX)) && (R0 != EDX)) POP32R(EDX);

	MOV32RtoR(R0, EAX);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVWLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	x86Reg32Type R0 = GetX86reg(0, ModeWrite);

	invalidConst(0);

	if ((isMapX86Reg(EDX)) && (R0 != EDX)) PUSH32R(EDX);
	if ((isMapX86Reg(ECX)) && (R0 != ECX)) PUSH32R(ECX);

	MOV32MtoR(ECX, (u32) & sh4.gbr);
	ADD32ItoR(ECX, (d << 1));
	CALL32((u32) & memRead16);

	if ((isMapX86Reg(ECX)) && (R0 != ECX)) POP32R(ECX);
	if ((isMapX86Reg(EDX)) && (R0 != EDX)) POP32R(EDX);

	MOV32RtoR(R0, EAX);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVLLG(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	x86Reg32Type R0 = GetX86reg(0, ModeWrite);

	invalidConst(0);

	if ((isMapX86Reg(EDX)) && (R0 != EDX)) PUSH32R(EDX);
	if ((isMapX86Reg(ECX)) && (R0 != ECX)) PUSH32R(ECX);

	MOV32MtoR(ECX, (u32) & sh4.gbr);
	ADD32ItoR(ECX, (d << 2));
	CALL32((u32) & memRead32);

	if ((isMapX86Reg(ECX)) && (R0 != ECX)) POP32R(ECX);
	if ((isMapX86Reg(EDX)) && (R0 != EDX)) POP32R(EDX);

	MOV32RtoR(R0, EAX);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVCAL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(0))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);
		MOV32ItoR(EDX, ConstantMap[0].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(n)) {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EDX, R0);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else if (isMappedConst(0)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32ItoR(EDX, ConstantMap[n].constant);
		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		switch (Rn) {
		case ECX:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);    break;
			case EDX:                           break;
			default:    MOV32RtoR(EDX, R0);     break;
			}
			break;

		case EDX:
			switch (R0) {
			case ECX:   XCHG32RtoR(EDX, ECX);   break;
			case EDX:   MOV32RtoR(ECX, EDX);   break;
			default:    MOV32RtoR(ECX, EDX);
				MOV32RtoR(EDX, R0);    break;
			}
			break;

		default:
			switch (R0) {
			case ECX:   MOV32RtoR(EDX, ECX);
				MOV32RtoR(ECX, Rn);    break;
			case EDX:   MOV32RtoR(ECX, Rn);    break;

			default:    MOV32RtoR(ECX, Rn);
				MOV32RtoR(EDX, R0);    break;
			}
			break;
		}

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVA(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	MapConst(0, (block_info.pc & 0xfffffffc) + (d << 2) + 4);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MOVT(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.sr.all);
	AND32ItoR(Rn, 0x00000001);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SWAPB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, ((ConstantMap[m].constant & 0xffff0000) << 0) |
				 ((ConstantMap[m].constant & 0x000000ff) << 8) |
				 ((ConstantMap[m].constant & 0x0000ff00) >> 8));
	} else {
		invalidConst(n);

		if (n == m) {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			ROL16ItoR(Rm, 8);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			x86Reg32Type Rn = GetX86reg(n, ModeWrite);

			MOV32RtoR(Rn, Rm);
			ROL16ItoR(Rn, 8);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SWAPW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, ((ConstantMap[m].constant & 0xffff0000) << 16) |
				 ((ConstantMap[m].constant & 0x0000ffff) >> 16));
	} else {
		invalidConst(n);

		if (n == m) {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			ROL32ItoR(Rm, 16);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			x86Reg32Type Rn = GetX86reg(n, ModeWrite);

			MOV32RtoR(Rn, Rm);
			ROL32ItoR(Rn, 16);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL XTRCT(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(m)) && (isMappedConst(n))) {
		MapConst(n, ((ConstantMap[n].constant & 0xffff0000) >> 16) |
				 ((ConstantMap[m].constant & 0x0000ffff) << 16));
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHR32ItoR(Rn, 16);
		OR32ItoR(Rn, (ConstantMap[m].constant & 0x0000ffff) << 16);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		MOV16RtoR(Rn, Rm);
		ROL32ItoR(Rn, 16);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		invalidConst(n);

		MOV16RtoR(Rn, Rm);
		ROL32ItoR(Rn, 16);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ADD(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ConstantMap[n].constant + ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		ADD32ItoR(Rn, ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		LEA32IRtoR(Rn, Rm, ConstantMap[n].constant);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		ADD32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ADDI(u32 code) {
	u32 i = ((code >> 0) & 0xff);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant + (u32)(s32)(s8)i);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		ADD32ItoR(Rn, (u32)(s32)(s8)i);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ADDC(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		MOV32ItoR(EAX, ConstantMap[m].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		ADC32RtoR(Rn, EAX);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		invalidConst(n);

		MOV32ItoR(EAX, ConstantMap[m].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		ADC32RtoR(Rn, EAX);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		ADC32RtoR(Rn, Rm);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		BT32ItoM((u32) & sh4.sr.all, 0);
		ADC32RtoR(Rn, Rm);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ADDV(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		ADD32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		invalidConst(n);

		ADD32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		ADD32RtoR(Rn, Rm);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		ADD32RtoR(Rn, Rm);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPIM(u32 code) {
	u8 *link[2];

	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if ((u32)ConstantMap[0].constant == (u32)(s32)(s8)i) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		CMP32ItoR(R0, (u32)(s32)(s8)i);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPEQ(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((u32)ConstantMap[n].constant == (u32)ConstantMap[m].constant) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		CMP32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		CMP32RtoR(Rn, Rm);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		CMP32RtoR(Rn, Rm);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPHS(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((u32)ConstantMap[n].constant >= (u32)ConstantMap[m].constant) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		CMP32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JAE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		CMP32RtoR(Rn, Rm);

		link[0] = JAE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		CMP32RtoR(Rn, Rm);

		link[0] = JAE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPGE(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((s32)ConstantMap[n].constant >= (s32)ConstantMap[m].constant) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		CMP32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JGE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		CMP32RtoR(Rn, Rm);

		link[0] = JGE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		CMP32RtoR(Rn, Rm);

		link[0] = JGE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPHI(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((u32)ConstantMap[n].constant > (u32)ConstantMap[m].constant) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		CMP32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JA8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		CMP32RtoR(Rn, Rm);

		link[0] = JA8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		CMP32RtoR(Rn, Rm);

		link[0] = JA8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPGT(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((s32)ConstantMap[n].constant > (s32)ConstantMap[m].constant) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		CMP32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JG8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		CMP32RtoR(Rn, Rm);

		link[0] = JG8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		CMP32RtoR(Rn, Rm);

		link[0] = JG8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPPZ(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if ((s32)ConstantMap[n].constant >= 0) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		TEST32RtoR(Rn, Rn);

		link[0] = JGE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPPL(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if ((s32)ConstantMap[n].constant > 0) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		TEST32RtoR(Rn, Rn);

		link[0] = JG8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CMPSTR(u32 code) {
	u8 *link[5];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		u32 tmp = ConstantMap[n].constant ^ ConstantMap[m].constant;

		u32 HH = (tmp >> 24) & 0x000000FF;
		u32 HL = (tmp >> 16) & 0x000000FF;
		u32 LH = (tmp >> 8) & 0x000000FF;
		u32 LL = (tmp >> 0) & 0x000000FF;

		if ((HH && HL && LH && LL) == 0) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32ItoR(EAX, ConstantMap[n].constant);
		XOR32RtoR(EAX, Rm);
		TEST32ItoR(EAX, 0xff000000);
		link[0] = JE8(0);
		TEST32ItoR(EAX, 0x00ff0000);
		link[1] = JE8(0);
		TEST32ItoR(EAX, 0x0000ff00);
		link[2] = JE8(0);
		TEST32ItoR(EAX, 0x000000ff);
		link[3] = JNE8(0);
		x86link8(link[0]);
		x86link8(link[1]);
		x86link8(link[2]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		link[4] = JMP8(0);
		x86link8(link[3]);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		x86link8(link[4]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		MOV32ItoR(EAX, ConstantMap[m].constant);
		XOR32RtoR(EAX, Rn);
		TEST32ItoR(EAX, 0xff000000);
		link[0] = JE8(0);
		TEST32ItoR(EAX, 0x00ff0000);
		link[1] = JE8(0);
		TEST32ItoR(EAX, 0x0000ff00);
		link[2] = JE8(0);
		TEST32ItoR(EAX, 0x000000ff);
		link[3] = JNE8(0);
		x86link8(link[0]);
		x86link8(link[1]);
		x86link8(link[2]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		link[4] = JMP8(0);
		x86link8(link[3]);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		x86link8(link[4]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoR(EAX, Rn);
		XOR32RtoR(EAX, Rm);
		TEST32ItoR(EAX, 0xff000000);
		link[0] = JE8(0);
		TEST32ItoR(EAX, 0x00ff0000);
		link[1] = JE8(0);
		TEST32ItoR(EAX, 0x0000ff00);
		link[2] = JE8(0);
		TEST32ItoR(EAX, 0x000000ff);
		link[3] = JNE8(0);
		x86link8(link[0]);
		x86link8(link[1]);
		x86link8(link[2]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		link[4] = JMP8(0);
		x86link8(link[3]);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		x86link8(link[4]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL DIV(u32 code) {
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
}

static void INLINE FASTCALL DIV1(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	Flushall();
	MOV32ItoR(ECX, (u32)code);
	CALL32((u32) & DIV);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL DIV0S(u32 code) {
	u8  *link;
	u16 *opcode;
	u32 count;

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (((block_info.pc + 2) & 0x1fffffff) < 0x00200000)
		opcode = (u16*)&prom[(block_info.pc + 2) & 0x001fffff];
	else
		opcode = (u16*)&pram[(block_info.pc + 2) & 0x00ffffff];

	count = 0;

	while (((opcode[count * 2 + 0] & 0xf0ff) == 0x4024) &&	//ROTCL
		   ((opcode[count * 2 + 1] & 0xf00f) == 0x3004)) {	//DIV1
		count++;
	}

	if (count == 32) {
		u8 *link[2];

		x86Reg32Type Rdivided;
		x86Reg32Type Rremainder;
		x86Reg32Type Rdivider;

		u32 divider = ((opcode[0] >> 8) & 0x0f);
		u32 remainder = ((opcode[1] >> 8) & 0x0f);
		u32 divided = ((opcode[1] >> 4) & 0x0f);

		if ((isMappedConst(divided)) && (isMappedConst(divider)) && (isMappedConst(remainder))) {
			Rdivider = GetX86reg(divider, ModeWrite);
			Rremainder = GetX86reg(remainder, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);
			invalidConst(remainder);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divided)) && (isMappedConst(divider))) {
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32RtoR(EDX, Rremainder);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divided)) && (isMappedConst(remainder))) {
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(remainder);

			MOV32RtoR(EAX, Rdivider);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divider)) && (isMappedConst(remainder))) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);
			Rremainder = GetX86reg(remainder, ModeWrite);

			invalidConst(divider);
			invalidConst(remainder);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			MOV32RtoR(ECX, Rdivided);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(divided)) {
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);

			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(EDX, Rremainder);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(divider)) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);

			invalidConst(divider);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32RtoR(EDX, Rremainder);
			MOV32RtoR(ECX, Rdivided);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(remainder)) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeWrite);

			invalidConst(remainder);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(ECX, Rdivided);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeRead);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(ECX, Rdivided);
			MOV32RtoR(EDX, Rremainder);
			IDIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SAR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		}

		block_info.pc += 2 + 32 * 2 + 32 * 2;
		block_info.cycle += 1 + 32 + 32;

		return;
	}

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		u32 m, q, t;

		AND32ItoM((u32) & sh4.sr.all, 0xfffffcfe);

		q = (ConstantMap[n].constant >> 31) & 1;
		m = (ConstantMap[m].constant >> 31) & 1;
		t = (q == m);

		OR32ItoM((u32) & sh4.sr.all, (m << 9) | (q << 8) | (t << 0));
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		AND32ItoM((u32) & sh4.sr.all, 0xfffffcfe);

		if (ConstantMap[m].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000200);
		}

		OR32RtoR(Rn, Rn);
		MOV32RtoR(EAX, Rn);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000100);
		x86link8(link);

		XOR32ItoR(EAX, ConstantMap[m].constant);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		AND32ItoM((u32) & sh4.sr.all, 0xfffffcfe);

		if (ConstantMap[n].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000100);
		}

		OR32RtoR(Rm, Rm);
		MOV32RtoR(EAX, Rm);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000200);
		x86link8(link);

		XOR32ItoR(EAX, ConstantMap[m].constant);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		AND32ItoM((u32) & sh4.sr.all, 0xfffffcfe);

		OR32RtoR(Rn, Rn);
		MOV32RtoR(EAX, Rn);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000100);
		x86link8(link);

		OR32RtoR(Rm, Rm);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000200);
		x86link8(link);

		XOR32RtoR(EAX, Rm);
		link = JNS8(0);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL DIV0U(u32 code) {
	u16 *opcode;
	u32 count;

	if (((block_info.pc + 2) & 0x1fffffff) < 0x00200000)
		opcode = (u16*)&prom[(block_info.pc + 2) & 0x001fffff];
	else
		opcode = (u16*)&pram[(block_info.pc + 2) & 0x00ffffff];

	count = 0;

	while (((opcode[count * 2 + 0] & 0xf0ff) == 0x4024) &&	//ROTCL
		   ((opcode[count * 2 + 1] & 0xf00f) == 0x3004)) {	//DIV1
		count++;
	}

	if (count == 32) {
		u8 *link[2];

		x86Reg32Type Rdivided;
		x86Reg32Type Rremainder;
		x86Reg32Type Rdivider;

		u32 divider = ((opcode[0] >> 8) & 0x0f);
		u32 remainder = ((opcode[1] >> 8) & 0x0f);
		u32 divided = ((opcode[1] >> 4) & 0x0f);

		if ((isMappedConst(divided)) && (isMappedConst(divider)) && (isMappedConst(remainder))) {
			Rdivider = GetX86reg(divider, ModeWrite);
			Rremainder = GetX86reg(remainder, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);
			invalidConst(remainder);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divided)) && (isMappedConst(divider))) {
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32RtoR(EDX, Rremainder);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divided)) && (isMappedConst(remainder))) {
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeWrite);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(remainder);

			MOV32RtoR(EAX, Rdivider);
			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if ((isMappedConst(divider)) && (isMappedConst(remainder))) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);
			Rremainder = GetX86reg(remainder, ModeWrite);

			invalidConst(divider);
			invalidConst(remainder);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			MOV32RtoR(ECX, Rdivided);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(divided)) {
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			invalidConst(divider);

			MOV32ItoR(ECX, ConstantMap[divided].constant);
			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(EDX, Rremainder);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32ItoR(Rremainder, ConstantMap[divided].constant);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(divider)) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rremainder = GetX86reg(remainder, ModeRead);
			Rdivider = GetX86reg(divider, ModeWrite);

			invalidConst(divider);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EAX, ConstantMap[divider].constant);
			MOV32RtoR(EDX, Rremainder);
			MOV32RtoR(ECX, Rdivided);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else if (isMappedConst(remainder)) {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeWrite);

			invalidConst(remainder);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32ItoR(EDX, ConstantMap[remainder].constant);
			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(ECX, Rdivided);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else {
			Rdivided = GetX86reg(divided, ModeRead);
			Rdivider = GetX86reg(divider, ModeRead);
			Rremainder = GetX86reg(remainder, ModeRead);

			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) PUSH32R(EDX);
			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) PUSH32R(ECX);

			MOV32RtoR(EAX, Rdivider);
			MOV32RtoR(ECX, Rdivided);
			MOV32RtoR(EDX, Rremainder);
			DIV32R(ECX);

			MOV32RtoR(Rremainder, EDX);
			MOV32RtoR(Rdivider, EAX);
			SHR32ItoR(Rdivider, 1);

			if ((isMapX86Reg(ECX)) && (ECX != Rdivider) && (ECX != Rremainder)) POP32R(ECX);
			if ((isMapX86Reg(EDX)) && (EDX != Rdivider) && (EDX != Rremainder)) POP32R(EDX);

			link[0] = JB8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			SUB32RtoR(Rremainder, Rdivided);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		}

		block_info.pc += 2 + 32 * 2 + 32 * 2;
		block_info.cycle += 1 + 32 + 32;

		return;
	}

	AND32ItoM((u32) & sh4.sr.all, 0xfffffcfe);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL DMULS(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		s64 mult = (s64)(s32)ConstantMap[n].constant * (s64)(s32)ConstantMap[m].constant;

		MOV32ItoM((u32) & sh4.macl, (u32)((mult >> 0) & 0xffffffff));
		MOV32ItoM((u32) & sh4.mach, (u32)((mult >> 32) & 0xffffffff));
	} else if (isMappedConst(m)) {
		if (ConstantMap[m].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
			MOV32ItoM((u32) & sh4.mach, 0);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[m].constant);
			IMUL32R(Rn);
			MOV32RtoM((u32) & sh4.macl, EAX);
			MOV32RtoM((u32) & sh4.mach, EDX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else if (isMappedConst(n)) {
		if (ConstantMap[n].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
			MOV32ItoM((u32) & sh4.mach, 0);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[n].constant);
			IMUL32R(Rm);
			MOV32RtoM((u32) & sh4.macl, EAX);
			MOV32RtoM((u32) & sh4.mach, EDX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);

		MOV32RtoR(EAX, Rn);
		IMUL32R(Rm);
		MOV32RtoM((u32) & sh4.macl, EAX);
		MOV32RtoM((u32) & sh4.mach, EDX);

		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL DMULU(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		u64 mult = (u64)(u32)ConstantMap[n].constant * (u64)(u32)ConstantMap[m].constant;

		MOV32ItoM((u32) & sh4.macl, (u32)((mult >> 0) & 0xffffffff));
		MOV32ItoM((u32) & sh4.mach, (u32)((mult >> 32) & 0xffffffff));
	} else if (isMappedConst(m)) {
		if (ConstantMap[m].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
			MOV32ItoM((u32) & sh4.mach, 0);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[m].constant);
			MUL32R(Rn);
			MOV32RtoM((u32) & sh4.macl, EAX);
			MOV32RtoM((u32) & sh4.mach, EDX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else if (isMappedConst(n)) {
		if (ConstantMap[n].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
			MOV32ItoM((u32) & sh4.mach, 0);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[n].constant);
			MUL32R(Rm);
			MOV32RtoM((u32) & sh4.macl, EAX);
			MOV32RtoM((u32) & sh4.mach, EDX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);

		MOV32RtoR(EAX, Rn);
		MUL32R(Rm);
		MOV32RtoM((u32) & sh4.macl, EAX);
		MOV32RtoM((u32) & sh4.mach, EDX);

		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL DT(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant - 1);

		if (ConstantMap[n].constant == 0) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		DEC32R(Rn);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL EXTSW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, (u32)(s32)(s16)ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOVSX32R16toR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL EXTSB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, (u32)(s32)(s8)ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		switch (Rm) {
		case 1:
			MOVSX32R8toR(Rn, CL);
			break;

		case 2:
			MOVSX32R8toR(Rn, DL);
			break;

		case 3:
			MOVSX32R8toR(Rn, BL);
			break;

		default:
			MOV32RtoR(EAX, Rm);
			MOVSX32R8toR(Rn, AL);
			break;
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL EXTUW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, (u32)(u16)ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOVZX32R16toR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL EXTUB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MapConst(n, (u32)(u8)ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		switch (Rm) {
		case 1:
			MOVZX32R8toR(Rn, CL);
			break;

		case 2:
			MOVZX32R8toR(Rn, DL);
			break;

		case 3:
			MOVZX32R8toR(Rn, BL);
			break;

		default:
			if (n != m) {
				MOV32RtoR(EAX, Rm);
				MOVZX32R8toR(Rn, AL);
			} else {
				AND32ItoR(Rn, 0xff);
			}
			break;
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL MACL(u32 code) {
	u8 *link[4];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn;
	x86Reg32Type Rm;

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memRead32);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		PUSH32R(EAX);
		MapConst(n, ConstantMap[n].constant + 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);
		MapConst(n, ConstantMap[m].constant + 4);
	} else if (isMappedConst(m)) {
		Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32RtoR(ECX, Rn);
		CALL32((u32) & memRead32);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		PUSH32R(EAX);
		ADD32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);
		MapConst(n, ConstantMap[m].constant + 4);
	} else if (isMappedConst(n)) {
		Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memRead32);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		PUSH32R(EAX);
		MapConst(n, ConstantMap[n].constant + 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);
	} else {
		Rn = GetX86reg(n, ModeRead);
		Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32RtoR(ECX, Rn);
		CALL32((u32) & memRead32);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		PUSH32R(EAX);
		ADD32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);
	}

	POP32R(EDX);
	IMUL32R(EDX);

	TEST32ItoM((u32) & sh4.sr.all, 0x00000002);
	link[0] = JNZ8(0);
	ADD32RtoM((u32) & sh4.macl, EAX);
	ADC32RtoM((u32) & sh4.mach, EDX);
	link[1] = JMP8(0);

	x86link8(link[0]);
	MOV32MtoR(ECX, (u32) & sh4.mach);
	ADD32RtoM((u32) & sh4.macl, EAX);
	ADC32RtoR(ECX, EDX);
	CMP32ItoR(ECX, 0xffff8000);
	link[0] = JGE8(0);
	MOV32ItoM((u32) & sh4.macl, 0xffff8000);
	MOV32ItoM((u32) & sh4.mach, 0x00000000);
	link[2] = JMP8(0);

	x86link8(link[0]);
	CMP32ItoR(ECX, 0x00007fff);
	link[0] = JLE8(0);
	MOV32ItoM((u32) & sh4.macl, 0x00007fff);
	MOV32ItoM((u32) & sh4.mach, 0xffffffff);
	link[3] = JMP8(0);
	x86link8(link[0]);
	MOV32RtoM((u32) & sh4.mach, ECX);

	x86link8(link[1]);
	x86link8(link[2]);
	x86link8(link[3]);

	if (isMapX86Reg(ECX)) POP32R(ECX);
	if (isMapX86Reg(EDX)) POP32R(EDX);

	if (!isMappedConst(m)) ADD32ItoR(Rm, 4);

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL MACW(u32 code) {
	u8 *link[4];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn;
	x86Reg32Type Rm;

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memRead16);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		MOVSX32R16toR(EAX, AX);
		PUSH32R(EAX);
		MapConst(n, ConstantMap[n].constant + 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead16);
		MOVSX32R16toR(EAX, AX);
		MapConst(n, ConstantMap[m].constant + 2);
	} else if (isMappedConst(m)) {
		Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32RtoR(ECX, Rn);
		CALL32((u32) & memRead16);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		MOVSX32R16toR(EAX, AX);
		PUSH32R(EAX);
		ADD32ItoR(Rn, 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead16);
		MOVSX32R16toR(EAX, AX);
		MapConst(n, ConstantMap[m].constant + 2);
	} else if (isMappedConst(n)) {
		Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memRead16);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		MOVSX32R16toR(EAX, AX);
		PUSH32R(EAX);
		MapConst(n, ConstantMap[n].constant + 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead16);
		MOVSX32R16toR(EAX, AX);
	} else {
		Rn = GetX86reg(n, ModeRead);
		Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);
		MOV32RtoR(ECX, Rn);
		CALL32((u32) & memRead16);
		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
		MOVSX32R16toR(EAX, AX);
		PUSH32R(EAX);
		ADD32ItoR(Rn, 2);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead16);
		MOVSX32R16toR(EAX, AX);
	}

	POP32R(EDX);
	IMUL32R(EDX);

	TEST32ItoM((u32) & sh4.sr.all, 0x00000002);
	link[0] = JNZ8(0);
	ADD32RtoM((u32) & sh4.macl, EAX);
	ADC32RtoM((u32) & sh4.mach, EDX);
	link[1] = JMP8(0);

	x86link8(link[0]);
	MOV32MtoR(ECX, (u32) & sh4.mach);
	ADD32RtoM((u32) & sh4.macl, EAX);
	ADC32RtoR(ECX, EDX);
	CMP32ItoR(ECX, 0xffffffff);
	link[0] = JGE8(0);
	MOV32ItoM((u32) & sh4.macl, 0xffffffff);
	MOV32ItoM((u32) & sh4.mach, 0x80000000);
	link[2] = JMP8(0);

	x86link8(link[0]);
	CMP32ItoR(ECX, 0x00000000);
	link[0] = JLE8(0);
	MOV32ItoM((u32) & sh4.macl, 0x00000000);
	MOV32ItoM((u32) & sh4.mach, 0xffffffff);
	link[3] = JMP8(0);
	x86link8(link[0]);
	MOV32RtoM((u32) & sh4.mach, ECX);

	x86link8(link[1]);
	x86link8(link[2]);
	x86link8(link[3]);

	if (isMapX86Reg(ECX)) POP32R(ECX);
	if (isMapX86Reg(EDX)) POP32R(EDX);

	if (!isMappedConst(m)) ADD32ItoR(Rm, 2);

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL MULL(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MOV32ItoM((u32) & sh4.macl, (s32)ConstantMap[n].constant * (s32)ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		if (ConstantMap[m].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[m].constant);
			IMUL32R(Rn);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else if (isMappedConst(n)) {
		if (ConstantMap[n].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, ConstantMap[n].constant);
			IMUL32R(Rm);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);

		MOV32RtoR(EAX, Rn);
		IMUL32R(Rm);
		MOV32RtoM((u32) & sh4.macl, EAX);

		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL MULSW(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MOV32ItoM((u32) & sh4.macl, (s32)(s16)ConstantMap[n].constant * (s32)(s16)ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		if (ConstantMap[m].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, (u32)(s32)(s16)ConstantMap[m].constant);
			MOVSX32R16toR(EDX, Rn);
			IMUL32R(EDX);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else if (isMappedConst(n)) {
		if (ConstantMap[n].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, (u32)(s32)(s16)ConstantMap[n].constant);
			MOVSX32R16toR(EDX, Rm);
			IMUL32R(EDX);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);

		MOVSX32R16toR(EAX, Rn);
		MOVSX32R16toR(EDX, Rm);
		IMUL32R(EDX);
		MOV32RtoM((u32) & sh4.macl, EAX);

		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL MULSU(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MOV32ItoM((u32) & sh4.macl, (u32)(u16)ConstantMap[n].constant * (u32)(u16)ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		if (ConstantMap[m].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, (u32)(u16)ConstantMap[m].constant);
			MOVZX32R16toR(EDX, Rn);
			MUL32R(EDX);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else if (isMappedConst(n)) {
		if (ConstantMap[n].constant == 0) {
			MOV32ItoM((u32) & sh4.macl, 0);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);

			MOV32ItoR(EAX, (u32)(u16)ConstantMap[n].constant);
			MOVZX32R16toR(EDX, Rm);
			MUL32R(EDX);
			MOV32RtoM((u32) & sh4.macl, EAX);

			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);

		MOVZX32R16toR(EAX, Rn);
		MOVZX32R16toR(EDX, Rm);
		MUL32R(EDX);
		MOV32RtoM((u32) & sh4.macl, EAX);

		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL NEG(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, 0 - ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		MOV32ItoR(Rn, 0 - ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32RtoR(Rn, Rm);
		NEG32R(Rn);
	} else {
		if (n == m) {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			NEG32R(Rm);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			x86Reg32Type Rn = GetX86reg(n, ModeWrite);

			MOV32RtoR(Rn, Rm);
			NEG32R(Rn);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL NEGC(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(EAX, ConstantMap[m].constant);
		XOR32ItoR(Rn, Rn);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, EAX);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		invalidConst(n);

		MOV32ItoR(EAX, ConstantMap[m].constant);
		XOR32ItoR(Rn, Rn);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, EAX);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		XOR32ItoR(Rn, Rn);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, Rm);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		if (n != m) {
			XOR32RtoR(Rn, Rn);
			BT32ItoM((u32) & sh4.sr.all, 0);
			SBB32RtoR(Rn, Rm);
		} else {
			XOR32RtoR(EAX, EAX);
			BT32ItoM((u32) & sh4.sr.all, 0);
			SBB32RtoR(EAX, Rm);
			MOV32RtoR(Rn, EAX);
		}

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SUB(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (n == m) {
		MapConst(n, 0);
	} else if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ConstantMap[n].constant - ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		SUB32RtoR(Rn, Rm);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		SUB32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SUBC(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		MOV32ItoR(EAX, ConstantMap[m].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, EAX);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		invalidConst(n);

		MOV32ItoR(EAX, ConstantMap[m].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, EAX);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, Rm);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		BT32ItoM((u32) & sh4.sr.all, 0);
		SBB32RtoR(Rn, Rm);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SUBV(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		SUB32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		invalidConst(n);

		SUB32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		SUB32RtoR(Rn, Rm);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		SUB32RtoR(Rn, Rm);

		link[0] = JO8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL AND(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ConstantMap[n].constant & ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		AND32ItoR(Rn, ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		AND32RtoR(Rn, Rm);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		AND32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ANDI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (i == 0) {
		MapConst(0, 0);
	} else if (isMappedConst(0)) {
		MapConst(0, ConstantMap[0].constant & (u32)(u8)i);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		AND32ItoR(R0, (u32)(u8)i);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ANDM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		CALL32((u32) & memRead8);

		AND32ItoR(EAX, (u32)i);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32RtoR(ECX, R0);
		PUSH32R(ECX);
		CALL32((u32) & memRead8);

		AND32ItoR(EAX, (u32)i);
		POP32R(ECX);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL NOT(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ~ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		MOV32ItoR(Rn, ~ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32RtoR(Rn, Rm);
		NOT32R(Rn);
	} else {
		if (n == m) {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			NOT32R(Rm);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			x86Reg32Type Rn = GetX86reg(n, ModeWrite);

			MOV32RtoR(Rn, Rm);
			NOT32R(Rn);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL OR(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ConstantMap[n].constant | ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		OR32ItoR(Rn, ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32RtoR(Rn, Rm);
		OR32ItoR(Rn, ConstantMap[n].constant);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		OR32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ORI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		MapConst(0, ConstantMap[0].constant | (u32)(u8)i);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		OR32ItoR(R0, (u32)(u8)i);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ORM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		CALL32((u32) & memRead8);

		OR32ItoR(EAX, (u32)i);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32RtoR(ECX, R0);
		PUSH32R(ECX);
		CALL32((u32) & memRead8);

		OR32ItoR(EAX, (u32)i);
		POP32R(ECX);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL TAS(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);

		CALL32((u32) & memRead8);

		TEST32RtoR(EAX, EAX);
		MOV32RtoR(EDX, EAX);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x00000001);
		x86link8(link[1]);

		OR32ItoR(EDX, 0x80);
		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);

		PUSH32R(ECX);
		CALL32((u32) & memRead8);

		TEST32RtoR(EAX, EAX);
		MOV32RtoR(EDX, EAX);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x00000001);
		x86link8(link[1]);

		OR32ItoR(EDX, 0x80);
		POP32R(ECX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL TST(u32 code) {
	u8 *link[2];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if (ConstantMap[n].constant & ConstantMap[m].constant) {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		} else {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		TEST32ItoR(Rn, ConstantMap[m].constant);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		TEST32ItoR(Rm, ConstantMap[n].constant);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		TEST32RtoR(Rn, Rm);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL TSTI(u32 code) {
	u8 *link[2];

	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (ConstantMap[0].constant & (u32)(u8)i) {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		} else {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		}
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (i == 0) TEST32RtoR(R0, R0);
		else TEST32ItoR(R0, (u32)(u8)i);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL TSTM(u32 code) {
	u8 *link[2];

	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		CALL32((u32) & memRead8);

		TEST32RtoR(EAX, EAX);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32RtoR(ECX, R0);
		CALL32((u32) & memRead8);

		TEST32RtoR(EAX, EAX);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL XOR(u32 code) {
	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if (n == m) {
		MapConst(n, 0);
	} else if ((isMappedConst(n)) && (isMappedConst(m))) {
		MapConst(n, ConstantMap[n].constant ^ ConstantMap[m].constant);
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		XOR32ItoR(Rn, ConstantMap[m].constant);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32RtoR(Rn, Rm);
		XOR32ItoR(Rn, ConstantMap[n].constant);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		XOR32RtoR(Rn, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL XORI(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		MapConst(0, ConstantMap[0].constant ^ (u32)(u8)i);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		XOR32ItoR(R0, (u32)(u8)i);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL XORM(u32 code) {
	u32 i = ((code >> 0) & 0xff);

	if (isMappedConst(0)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		CALL32((u32) & memRead8);

		XOR32ItoR(EAX, (u32)i);
		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32ItoR(ECX, ConstantMap[0].constant);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type R0 = GetX86reg(0, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32MtoR(ECX, (u32) & sh4.gbr);
		ADD32RtoR(ECX, R0);
		PUSH32R(ECX);
		CALL32((u32) & memRead8);

		XOR32ItoR(EAX, (u32)i);
		POP32R(ECX);
		MOV32RtoR(EDX, EAX);
		CALL32((u32) & memWrite8);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 5;
}

static void INLINE FASTCALL ROTR(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 0x00000001) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, ((ConstantMap[n].constant & 0x00000001) << 31) |
				 ((ConstantMap[n].constant & 0xfffffffe) >> 1));
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		ROR32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ROTCR(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		RCR32ItoR(Rn, 1);

		if (ConstantMap[n].constant & 0x00000001) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		BT32ItoM((u32) & sh4.sr.all, 0);
		RCR32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ROTL(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, ((ConstantMap[n].constant & 0x80000000) >> 31) |
				 ((ConstantMap[n].constant & 0x7fffffff) << 1));
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		ROL32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL ROTCL(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);
		BT32ItoM((u32) & sh4.sr.all, 0);
		RCL32ItoR(Rn, 1);

		if (ConstantMap[n].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		BT32ItoM((u32) & sh4.sr.all, 0);
		RCL32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHAD(u32 code) {
	u8 *link[4];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((ConstantMap[m].constant & 0x80000000) == 0) {
			MapConst(n, ConstantMap[n].constant << (ConstantMap[m].constant & 0x1f));
		} else if ((ConstantMap[m].constant & 0x1f) == 0) {
			MapConst(n, (s32)ConstantMap[n].constant >> 31);
		} else {
			MapConst(n, (s32)ConstantMap[n].constant >> ((~ConstantMap[m].constant & 0x1f) + 1));
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EAX, Rn);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(0);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		SAR32ItoR(EAX, 31);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SAR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(EAX, ConstantMap[n].constant);
		MOV32RtoR(ECX, Rm);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(0);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		SAR32ItoR(EAX, 31);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SAR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EAX, Rn);
		MOV32RtoR(ECX, Rm);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(0);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		SAR32ItoR(EAX, 31);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SAR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHAR(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 1) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, (s32)ConstantMap[n].constant >> 1);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SAR32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLD(u32 code) {
	u8 *link[4];

	u32 m = ((code >> 4) & 0x0f);
	u32 n = ((code >> 8) & 0x0f);

	if ((isMappedConst(n)) && (isMappedConst(m))) {
		if ((ConstantMap[m].constant & 0x80000000) == 0) {
			MapConst(n, ConstantMap[n].constant << (ConstantMap[m].constant & 0x1f));
		} else if ((ConstantMap[m].constant & 0x1f) == 0) {
			MapConst(n, 0);
		} else {
			MapConst(n, ConstantMap[n].constant >> ((~ConstantMap[m].constant & 0x1f) + 1));
		}
	} else if (isMappedConst(m)) {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EAX, Rn);
		MOV32ItoR(ECX, ConstantMap[m].constant);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(1);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		XOR32RtoR(EAX, EAX);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SHR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	} else if (isMappedConst(n)) {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);
		x86Reg32Type Rn = GetX86reg(n, ModeWrite);

		invalidConst(n);

		MOV32ItoR(Rn, ConstantMap[n].constant);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EAX, Rn);
		MOV32RtoR(ECX, Rm);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(1);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		XOR32RtoR(EAX, EAX);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SHR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(EAX, Rn);
		MOV32RtoR(ECX, Rm);
		TEST32RtoR(ECX, ECX);
		link[0] = JS8(0);
		AND32ItoR(ECX, 0x1f);
		SHL32CLtoR(EAX);
		link[1] = JMP8(1);
		x86link8(link[0]);
		TEST32ItoR(ECX, 0x1f);
		link[2] = JNE8(0);
		XOR32RtoR(EAX, EAX);
		link[3] = JMP8(0);
		x86link8(link[2]);
		NOT32R(ECX);
		AND32ItoR(ECX, 0x1f);
		INC32R(ECX);
		SHR32CLtoR(EAX);
		x86link8(link[1]);
		x86link8(link[3]);

		if (isMapX86Reg(ECX)) POP32R(ECX);

		MOV32RtoR(Rn, EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHAL(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, (u32)ConstantMap[n].constant << 1);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHL32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLL(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 0x80000000) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, (u32)ConstantMap[n].constant << 1);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHL32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLR(u32 code) {
	u8 *link[2];

	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (ConstantMap[n].constant & 1) {
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		} else {
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		}

		MapConst(n, (u32)ConstantMap[n].constant >> 1);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHR32ItoR(Rn, 1);

		link[0] = JB8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLL2(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant << 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHL32ItoR(Rn, 2);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLR2(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant >> 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHR32ItoR(Rn, 2);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLL8(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant << 8);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHL32ItoR(Rn, 8);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLR8(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant >> 8);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHR32ItoR(Rn, 8);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLL16(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant << 16);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHL32ItoR(Rn, 16);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SHLR16(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		MapConst(n, ConstantMap[n].constant >> 16);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SHR32ItoR(Rn, 16);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL BF(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	Flushall();
	TEST32ItoM((u32) & sh4.sr.all, 1);
	block_info.link = JNZ8(0);
	block_info.need_linking_block1 = 1;
	block_info.block_pc1 = block_info.pc + (((s32)(s8)d) << 1) + 4;
	block_info.need_linking_block2 = 1;
	block_info.block_pc2 = block_info.pc + 2;

	block_info.cycle += 2;
	block_info.compile = 1;
}

static void INLINE FASTCALL BFS(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	TEST32ItoM((u32) & sh4.sr.all, 1);
	JNZ8(0x0c);
	MOV32ItoM((u32) & sh4.pc, block_info.pc + (((s32)(s8)d) << 1) + 4);
	JMP8(0x0a);
	MOV32ItoM((u32) & sh4.pc, block_info.pc + 4);

	block_info.cycle += 2;
	block_info.compile = 1;

	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL BT(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	Flushall();
	TEST32ItoM((u32) & sh4.sr.all, 1);
	block_info.link = JZ8(0);
	block_info.need_linking_block1 = 1;
	block_info.block_pc1 = block_info.pc + (((s32)(s8)d) << 1) + 4;
	block_info.need_linking_block2 = 1;
	block_info.block_pc2 = block_info.pc + 2;

	block_info.cycle += 2;
	block_info.compile = 1;
}

static void INLINE FASTCALL BTS(u32 code) {
	u32 d = ((code >> 0) & 0xff);

	TEST32ItoM((u32) & sh4.sr.all, 1);
	JZ8(0x0c);
	MOV32ItoM((u32) & sh4.pc, block_info.pc + (((s32)(s8)d) << 1) + 4);
	JMP8(0x0a);
	MOV32ItoM((u32) & sh4.pc, block_info.pc + 4);

	block_info.cycle += 2;
	block_info.compile = 1;

	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL BRA(u32 code) {
	u32 d = ((s32)(code << 20) >> 20);

	block_info.need_linking_block1 = 1;
	block_info.block_pc1 = block_info.pc + (d << 1) + 4;
	block_info.cycle += 2;
	block_info.compile = 1;

	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL BSR(u32 code) {
	u32 d = ((s32)(code << 20) >> 20);

	block_info.need_linking_block1 = 1;
	block_info.block_pc1 = block_info.pc + (d << 1) + 4;
	block_info.cycle += 2;
	block_info.compile = 1;

	MOV32ItoM((u32) & sh4.pr, block_info.pc + 4);
	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL BRAF(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = ConstantMap[n].constant + block_info.pc + 4;
		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		LEA32IRtoR(EAX, Rn, block_info.pc + 4);
		MOV32RtoM((u32) & sh4.pc, EAX);

		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	}
}

static void INLINE FASTCALL BSRF(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = ConstantMap[n].constant + block_info.pc + 4;
		block_info.cycle += 2;
		block_info.compile = 1;

		MOV32ItoM((u32) & sh4.pr, block_info.pc + 4);
		recStep(block_info.pc + 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		MOV32ItoM((u32) & sh4.pr, block_info.pc + 4);
		LEA32IRtoR(EAX, Rn, block_info.pc + 4);
		MOV32RtoM((u32) & sh4.pc, EAX);

		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	}
}

static void INLINE FASTCALL JMP(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = ConstantMap[n].constant;
		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		MOV32RtoM((u32) & sh4.pc, Rn);

		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	}
}

static void INLINE FASTCALL JSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = ConstantMap[n].constant;
		block_info.cycle += 2;
		block_info.compile = 1;

		MOV32ItoM((u32) & sh4.pr, block_info.pc + 4);
		recStep(block_info.pc + 2);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		MOV32ItoM((u32) & sh4.pr, block_info.pc + 4);
		MOV32RtoM((u32) & sh4.pc, Rn);

		block_info.cycle += 2;
		block_info.compile = 1;

		recStep(block_info.pc + 2);
	}
}

static void INLINE FASTCALL RTS(u32 code) {
	MOV32MtoR(EAX, (u32) & sh4.pr);
	MOV32RtoM((u32) & sh4.pc, EAX);

	block_info.cycle += 2;
	block_info.compile = 1;

	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL RTE(u32 code) {
	MOV32MtoR(EAX, (u32) & sh4.spc);
	MOV32RtoM((u32) & sh4.pc, EAX);

	MOV32MtoR(EAX, (u32) & sh4.ssr);
	AND32ItoR(EAX, 0x700083f3);

	Flushall();

	TEST32ItoM((u32) & sh4.sr.all, 0x20000000);
	SETE8R(CL);
	MOV32RtoM((u32) & sh4.sr.all, EAX);
	TEST32ItoM((u32) & sh4.sr.all, 0x20000000);
	SETE8R(CH);

	CMP8RtoR(CL, CH);

	if (cpuSSE) {
		u8 *link = JE8(0);

		MOVAPS128MtoXMM(XMM0, (u32) & sh4.r[0 + 0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.r[0 + 4]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.r[16 + 0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.r[16 + 4]);

		MOVAPS128XMMtoM((u32) & sh4.r[0 + 0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.r[0 + 4], XMM3);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 4], XMM1);

		x86link8(link);
	} else {
		u8 *link = JE8(0);

		MOV64MtoMMX(MM0, (u32) & sh4.r[0 + 0]);
		MOV64MtoMMX(MM1, (u32) & sh4.r[0 + 2]);
		MOV64MtoMMX(MM2, (u32) & sh4.r[0 + 4]);
		MOV64MtoMMX(MM3, (u32) & sh4.r[0 + 6]);
		MOV64MtoMMX(MM4, (u32) & sh4.r[16 + 0]);
		MOV64MtoMMX(MM5, (u32) & sh4.r[16 + 2]);
		MOV64MtoMMX(MM6, (u32) & sh4.r[16 + 4]);
		MOV64MtoMMX(MM7, (u32) & sh4.r[16 + 6]);

		MOV64MMXtoM((u32) & sh4.r[0 + 0], MM4);
		MOV64MMXtoM((u32) & sh4.r[0 + 2], MM5);
		MOV64MMXtoM((u32) & sh4.r[0 + 4], MM6);
		MOV64MMXtoM((u32) & sh4.r[0 + 6], MM7);
		MOV64MMXtoM((u32) & sh4.r[16 + 0], MM0);
		MOV64MMXtoM((u32) & sh4.r[16 + 2], MM1);
		MOV64MMXtoM((u32) & sh4.r[16 + 4], MM2);
		MOV64MMXtoM((u32) & sh4.r[16 + 6], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();

		x86link8(link);
	}

	block_info.cycle += 5;
	block_info.compile = 1;

	recStep(block_info.pc + 2);
}

static void INLINE FASTCALL CLRMAC(u32 code) {
	MOV32ItoM((u32) & sh4.macl, 0);
	MOV32ItoM((u32) & sh4.mach, 0);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CLRS(u32 code) {
	AND32ItoM((u32) & sh4.sr.all, 0xfffffffd);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL CLRT(u32 code) {
	AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32RtoR(EAX, ConstantMap[m].constant & 0x700083f3);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoR(EAX, Rm);
		AND32ItoR(EAX, 0x700083f3);
	}

	Flushall();

	MOV32MtoR(ECX, (u32) & sh4.sr.all);
	AND32ItoR(ECX, 0x20000000);
	MOV32RtoM((u32) & sh4.sr.all, EAX);
	AND32ItoR(EAX, 0x20000000);

	CMP32RtoR(EAX, ECX);

	if (cpuSSE) {
		u8 *link = JE8(0);

		MOVAPS128MtoXMM(XMM0, (u32) & sh4.r[0 + 0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.r[0 + 4]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.r[16 + 0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.r[16 + 4]);

		MOVAPS128XMMtoM((u32) & sh4.r[0 + 0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.r[0 + 4], XMM3);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 4], XMM1);

		x86link8(link);
	} else {
		u8 *link = JE8(0);

		MOV64MtoMMX(MM0, (u32) & sh4.r[0 + 0]);
		MOV64MtoMMX(MM1, (u32) & sh4.r[0 + 2]);
		MOV64MtoMMX(MM2, (u32) & sh4.r[0 + 4]);
		MOV64MtoMMX(MM3, (u32) & sh4.r[0 + 6]);
		MOV64MtoMMX(MM4, (u32) & sh4.r[16 + 0]);
		MOV64MtoMMX(MM5, (u32) & sh4.r[16 + 2]);
		MOV64MtoMMX(MM6, (u32) & sh4.r[16 + 4]);
		MOV64MtoMMX(MM7, (u32) & sh4.r[16 + 6]);

		MOV64MMXtoM((u32) & sh4.r[0 + 0], MM4);
		MOV64MMXtoM((u32) & sh4.r[0 + 2], MM5);
		MOV64MMXtoM((u32) & sh4.r[0 + 4], MM6);
		MOV64MMXtoM((u32) & sh4.r[0 + 6], MM7);
		MOV64MMXtoM((u32) & sh4.r[16 + 0], MM0);
		MOV64MMXtoM((u32) & sh4.r[16 + 2], MM1);
		MOV64MMXtoM((u32) & sh4.r[16 + 4], MM2);
		MOV64MMXtoM((u32) & sh4.r[16 + 6], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();

		x86link8(link);
	}

	block_info.pc += 2;
	block_info.cycle += 4;
}

static void INLINE FASTCALL LDCGBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.gbr, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.gbr, Rm);
	}

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL LDCVBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.vbr, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.vbr, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCSSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.ssr, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.ssr, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCSPC(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.spc, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.spc, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCDBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.dbr, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.dbr, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.r[b], ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.r[b], Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCMSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	AND32ItoR(EAX, 0x700083f3);

	Flushall();

	MOV32MtoR(ECX, (u32) & sh4.sr.all);
	AND32ItoR(ECX, 0x20000000);
	MOV32RtoM((u32) & sh4.sr.all, EAX);
	AND32ItoR(EAX, 0x20000000);

	CMP32RtoR(EAX, ECX);

	if (cpuSSE) {
		u8 *link = JE8(0);

		MOVAPS128MtoXMM(XMM0, (u32) & sh4.r[0 + 0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.r[0 + 4]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.r[16 + 0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.r[16 + 4]);

		MOVAPS128XMMtoM((u32) & sh4.r[0 + 0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.r[0 + 4], XMM3);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.r[16 + 4], XMM1);

		x86link8(link);
	} else {
		u8 *link = JE8(0);

		MOV64MtoMMX(MM0, (u32) & sh4.r[0 + 0]);
		MOV64MtoMMX(MM1, (u32) & sh4.r[0 + 2]);
		MOV64MtoMMX(MM2, (u32) & sh4.r[0 + 4]);
		MOV64MtoMMX(MM3, (u32) & sh4.r[0 + 6]);
		MOV64MtoMMX(MM4, (u32) & sh4.r[16 + 0]);
		MOV64MtoMMX(MM5, (u32) & sh4.r[16 + 2]);
		MOV64MtoMMX(MM6, (u32) & sh4.r[16 + 4]);
		MOV64MtoMMX(MM7, (u32) & sh4.r[16 + 6]);

		MOV64MMXtoM((u32) & sh4.r[0 + 0], MM4);
		MOV64MMXtoM((u32) & sh4.r[0 + 2], MM5);
		MOV64MMXtoM((u32) & sh4.r[0 + 4], MM6);
		MOV64MMXtoM((u32) & sh4.r[0 + 6], MM7);
		MOV64MMXtoM((u32) & sh4.r[16 + 0], MM0);
		MOV64MMXtoM((u32) & sh4.r[16 + 2], MM1);
		MOV64MMXtoM((u32) & sh4.r[16 + 4], MM2);
		MOV64MMXtoM((u32) & sh4.r[16 + 6], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();

		x86link8(link);
	}

	ADD32ItoM((u32) & sh4.r[m], 4);

	block_info.pc += 2;
	block_info.cycle += 4;
}

static void INLINE FASTCALL LDCMGBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.gbr, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.gbr, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL LDCMVBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.vbr, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.vbr, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCMSSR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.ssr, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.ssr, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCMSPC(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.spc, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.spc, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCMDBR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.dbr, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.dbr, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDCMRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.r[b], EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.r[b], EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMACH(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.mach, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.mach, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMACL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.macl, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.macl, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSPR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.pr, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.pr, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMMACH(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.mach, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.mach, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMMACL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.macl, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.macl, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMPR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.pr, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.pr, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDTLB(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL NOP(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL OCBI(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL OCBP(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL OCBWB(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void FASTCALL PREFETCH(u32 Rn) {
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
}

static void INLINE FASTCALL PREF(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant);
		CALL32((u32) & PREFETCH);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		CALL32((u32) & PREFETCH);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SETS(u32 code) {
	OR32ItoM((u32) & sh4.sr.all, 0x000000002);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SETT(u32 code) {
	OR32ItoM((u32) & sh4.sr.all, 0x000000001);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL SLEEP(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STCSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.sr.all);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCGBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.gbr);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCVBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.vbr);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCSSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.ssr);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCSPC(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.spc);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCSGR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.sgr);

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL STCDBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.dbr);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.r[b]);

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.sr.all);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.sr.all);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMGBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.gbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.gbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMVBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.vbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.vbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMSSR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.ssr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.ssr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMSPC(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.spc);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.spc);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMSGR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.sgr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.sgr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 3;
}

static void INLINE FASTCALL STCMDBR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.dbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.dbr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STCMRBANK(u32 code) {
	u32 b = ((code >> 4) & 0x07) + 16;
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.r[b]);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.r[b]);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle += 2;
}

static void INLINE FASTCALL STSMACH(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.mach);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMACL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.macl);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSPR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.pr);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMMACH(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.mach);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.mach);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMMACL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.macl);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.macl);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMPR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.pr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.pr);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL TRAPA(u32 code) {
	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSFPSCR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoR(EAX, ConstantMap[m].constant & 0x003fffff);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoR(EAX, Rm);
		AND32ItoR(EAX, 0x003fffff);
	}

	if (isMapX86Reg(ECX)) PUSH32R(ECX);

	MOV32MtoR(ECX, (u32) & sh4.fpscr.all);
	AND32ItoR(ECX, 0x00200000);
	MOV32RtoM((u32) & sh4.fpscr.all, EAX);
	AND32ItoR(EAX, 0x00200000);

	CMP32RtoR(EAX, ECX);

	if (isMapX86Reg(ECX)) POP32R(ECX);

	if (cpuSSE) {
		u8 *link = JE8(0);

		MOVAPS128MtoXMM(XMM0, (u32) & sh4.vec[0][0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.vec[1][0]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.vec[2][0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.vec[3][0]);
		MOVAPS128MtoXMM(XMM4, (u32) & sh4.vec[4][0]);
		MOVAPS128MtoXMM(XMM5, (u32) & sh4.vec[5][0]);
		MOVAPS128MtoXMM(XMM6, (u32) & sh4.vec[6][0]);
		MOVAPS128MtoXMM(XMM7, (u32) & sh4.vec[7][0]);

		MOVAPS128XMMtoM((u32) & sh4.vec[0][0], XMM4);
		MOVAPS128XMMtoM((u32) & sh4.vec[1][0], XMM5);
		MOVAPS128XMMtoM((u32) & sh4.vec[2][0], XMM6);
		MOVAPS128XMMtoM((u32) & sh4.vec[3][0], XMM7);
		MOVAPS128XMMtoM((u32) & sh4.vec[4][0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.vec[5][0], XMM1);
		MOVAPS128XMMtoM((u32) & sh4.vec[6][0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.vec[7][0], XMM3);

		x86link8(link);
	} else {
		u32 *link = JE32(0);

		MOV64MtoMMX(MM0, (u32) & sh4.dbl[0]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[1]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[2]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[3]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[8]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[9]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[10]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[11]);
		MOV64MMXtoM((u32) & sh4.dbl[0], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[1], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[2], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[3], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[8], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[9], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[10], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[11], MM3);

		MOV64MtoMMX(MM0, (u32) & sh4.dbl[4]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[5]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[6]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[7]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[12]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[13]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[14]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[15]);
		MOV64MMXtoM((u32) & sh4.dbl[4], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[5], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[6], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[7], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[12], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[13], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[14], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[15], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();

		x86link32(link);
	}

	block_info.pc += 2;
	block_info.cycle++;

	if (!block_info.compile) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = block_info.pc;
		block_info.compile = 1;
	}
}

static void INLINE FASTCALL LDSFPUL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		MOV32ItoM((u32) & sh4.fpul, ConstantMap[m].constant);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		MOV32RtoM((u32) & sh4.fpul, Rm);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL LDSMFPSCR(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		ADD32ItoR(Rm, 4);
	}

	AND32ItoR(EAX, 0x003fffff);

	if (isMapX86Reg(ECX)) PUSH32R(ECX);

	MOV32MtoR(ECX, (u32) & sh4.fpscr.all);
	AND32ItoR(ECX, 0x00200000);
	MOV32RtoM((u32) & sh4.fpscr.all, EAX);
	AND32ItoR(EAX, 0x00200000);

	CMP32RtoR(EAX, ECX);

	if (isMapX86Reg(ECX)) POP32R(ECX);

	if (cpuSSE) {
		u8 *link = JE8(0);

		MOVAPS128MtoXMM(XMM0, (u32) & sh4.vec[0][0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.vec[1][0]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.vec[2][0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.vec[3][0]);
		MOVAPS128MtoXMM(XMM4, (u32) & sh4.vec[4][0]);
		MOVAPS128MtoXMM(XMM5, (u32) & sh4.vec[5][0]);
		MOVAPS128MtoXMM(XMM6, (u32) & sh4.vec[6][0]);
		MOVAPS128MtoXMM(XMM7, (u32) & sh4.vec[7][0]);

		MOVAPS128XMMtoM((u32) & sh4.vec[0][0], XMM4);
		MOVAPS128XMMtoM((u32) & sh4.vec[1][0], XMM5);
		MOVAPS128XMMtoM((u32) & sh4.vec[2][0], XMM6);
		MOVAPS128XMMtoM((u32) & sh4.vec[3][0], XMM7);
		MOVAPS128XMMtoM((u32) & sh4.vec[4][0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.vec[5][0], XMM1);
		MOVAPS128XMMtoM((u32) & sh4.vec[6][0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.vec[7][0], XMM3);

		x86link8(link);
	} else {
		u32 *link = JE32(0);

		MOV64MtoMMX(MM0, (u32) & sh4.dbl[0]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[1]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[2]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[3]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[8]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[9]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[10]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[11]);
		MOV64MMXtoM((u32) & sh4.dbl[0], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[1], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[2], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[3], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[8], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[9], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[10], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[11], MM3);

		MOV64MtoMMX(MM0, (u32) & sh4.dbl[4]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[5]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[6]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[7]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[12]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[13]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[14]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[15]);
		MOV64MMXtoM((u32) & sh4.dbl[4], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[5], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[6], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[7], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[12], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[13], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[14], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[15], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();

		x86link32(link);
	}

	block_info.pc += 2;
	block_info.cycle++;

	if (!block_info.compile) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = block_info.pc;
		block_info.compile = 1;
	}
}

static void INLINE FASTCALL LDSMFPUL(u32 code) {
	u32 m = ((code >> 8) & 0x0f);

	if (isMappedConst(m)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[m].constant);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.fpul, EAX);

		MapConst(m, ConstantMap[m].constant + 4);
	} else {
		x86Reg32Type Rm = GetX86reg(m, ModeRead);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rm);
		CALL32((u32) & memRead32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MOV32RtoM((u32) & sh4.fpul, EAX);
		ADD32ItoR(Rm, 4);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSFPSCR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.fpscr.all);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSFPUL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	x86Reg32Type Rn = GetX86reg(n, ModeWrite);

	invalidConst(n);

	MOV32MtoR(Rn, (u32) & sh4.fpul);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMFPSCR(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.fpscr.all);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.fpscr.all);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL STSMFPUL(u32 code) {
	u32 n = ((code >> 8) & 0x0f);

	if (isMappedConst(n)) {
		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32ItoR(ECX, ConstantMap[n].constant - 4);
		MOV32MtoR(EDX, (u32) & sh4.fpul);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);

		MapConst(n, ConstantMap[n].constant - 4);
	} else {
		x86Reg32Type Rn = GetX86reg(n, ModeRead);

		SUB32ItoR(Rn, 4);

		if (isMapX86Reg(EDX)) PUSH32R(EDX);
		if (isMapX86Reg(ECX)) PUSH32R(ECX);

		MOV32RtoR(ECX, Rn);
		MOV32MtoR(EDX, (u32) & sh4.fpul);

		CALL32((u32) & memWrite32);

		if (isMapX86Reg(ECX)) POP32R(ECX);
		if (isMapX86Reg(EDX)) POP32R(EDX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FLDI0(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	MOV32ItoM((u32) & sh4.flt[n], 0x00000000);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FLDI1(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	MOV32ItoM((u32) & sh4.flt[n], 0x3f800000);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV(u32 code) {
	if (sh4.fpscr.sz) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		switch (code & 0x00000110) {
		case 0x00000000:
			if (cpuSSE) {
				MOVLPS64MtoXMM(XMM0, (u32) & sh4.dbli[m + 0]);
				MOVLPS64XMMtoM((u32) & sh4.dbli[n + 0], XMM0);
			} else {
				MOV64MtoMMX(MM0, (u32) & sh4.dbl[m + 0]);
				MOV64MMXtoM((u32) & sh4.dbl[n + 0], MM0);

				if (cpu3DNOW) FEMMS();
				else EMMS();
			}
			break;

		case 0x00000010:
			if (cpuSSE) {
				MOVLPS64MtoXMM(XMM0, (u32) & sh4.dbli[m + 8]);
				MOVLPS64XMMtoM((u32) & sh4.dbli[n + 0], XMM0);
			} else {
				MOV64MtoMMX(MM0, (u32) & sh4.dbl[m + 8]);
				MOV64MMXtoM((u32) & sh4.dbl[n + 0], MM0);

				if (cpu3DNOW) FEMMS();
				else EMMS();
			}
			break;

		case 0x00000100:
			if (cpuSSE) {
				MOVLPS64MtoXMM(XMM0, (u32) & sh4.dbli[m + 0]);
				MOVLPS64XMMtoM((u32) & sh4.dbli[n + 8], XMM0);
			} else {
				MOV64MtoMMX(MM0, (u32) & sh4.dbl[m + 0]);
				MOV64MMXtoM((u32) & sh4.dbl[n + 8], MM0);

				if (cpu3DNOW) FEMMS();
				else EMMS();
			}
			break;

		case 0x00000110:
			if (cpuSSE) {
				MOVLPS64MtoXMM(XMM0, (u32) & sh4.dbli[m + 8]);
				MOVLPS64XMMtoM((u32) & sh4.dbli[n + 8], XMM0);
			} else {
				MOV64MtoMMX(MM0, (u32) & sh4.dbl[m + 8]);
				MOV64MMXtoM((u32) & sh4.dbl[n + 8], MM0);

				if (cpu3DNOW) FEMMS();
				else EMMS();
			}
			break;
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		MOV32MtoR(EAX, (u32) & sh4.flti[m]);
		MOV32RtoM((u32) & sh4.flti[n], EAX);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_LOAD(u32 code) {
	if (sh4.fpscr.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			if (isMappedConst(m)) {
				MemRead64Translate(ConstantMap[m].constant, (u32) & sh4.dbli[n]);
			} else {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rm);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			if (isMappedConst(m)) {
				MemRead64Translate(ConstantMap[m].constant, (u32) & sh4.dbli[n]);
			} else {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rm);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (isMappedConst(m)) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[m].constant);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32RtoR(ECX, Rm);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_INDEX_LOAD(u32 code) {
	if (block_info.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			if ((isMappedConst(m)) && (isMappedConst(0))) {
				MemRead64Translate(ConstantMap[m].constant + ConstantMap[0].constant, (u32) & sh4.dbli[n]);
			} else if (isMappedConst(m)) {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(0)) {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32RRtoR(ECX, Rm, R0);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			if ((isMappedConst(m)) && (isMappedConst(0))) {
				MemRead64Translate(ConstantMap[m].constant + ConstantMap[0].constant, (u32) & sh4.dbli[n]);
			} else if (isMappedConst(m)) {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(0)) {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32RRtoR(ECX, Rm, R0);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if ((isMappedConst(m)) && (isMappedConst(0))) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[m].constant + ConstantMap[0].constant);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		} else if (isMappedConst(m)) {
			x86Reg32Type R0 = GetX86reg(0, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32IRtoR(ECX, R0, ConstantMap[m].constant);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		} else if (isMappedConst(0)) {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32IRtoR(ECX, Rm, ConstantMap[0].constant);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);
			x86Reg32Type R0 = GetX86reg(0, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32RRtoR(ECX, Rm, R0);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_RESTORE(u32 code) {
	if (block_info.sz) {
		if ((code & 0x00000100) == 0) {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 0;

			if (isMappedConst(m)) {
				MemRead64Translate(ConstantMap[m].constant, (u32) & sh4.dbli[n]);

				MapConst(m, ConstantMap[m].constant + 8);
			} else {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rm);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);

				ADD32ItoR(Rm, 8);
			}
		} else {
			u32 m = ((code >> 4) & 0xf);
			u32 n = ((code >> 9) & 0x7) + 8;

			if (isMappedConst(m)) {
				MemRead64Translate(ConstantMap[m].constant, (u32) & sh4.dbli[n]);

				MapConst(m, ConstantMap[m].constant + 8);
			} else {
				x86Reg32Type Rm = GetX86reg(m, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rm);
				MOV32ItoR(EDX, (u32) & sh4.dbli[n]);

				CALL32((u32) & memRead64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);

				ADD32ItoR(Rm, 8);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (isMappedConst(m)) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[m].constant);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);

			MapConst(m, ConstantMap[m].constant + 4);
		} else {
			x86Reg32Type Rm = GetX86reg(m, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32RtoR(ECX, Rm);
			CALL32((u32) & memRead32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MOV32RtoM((u32) & sh4.flti[n], EAX);

			ADD32ItoR(Rm, 4);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_SAVE(u32 code) {
	if (block_info.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			if (isMappedConst(n)) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant - 8);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);

				MapConst(n, ConstantMap[n].constant - 8);
			} else {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);

				SUB32ItoR(Rn, 8);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rn);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			if (isMappedConst(n)) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant - 8);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);

				MapConst(n, ConstantMap[n].constant - 8);
			} else {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);

				SUB32ItoR(Rn, 8);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rn);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (isMappedConst(n)) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[n].constant - 4);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);

			MapConst(n, ConstantMap[n].constant - 4);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			SUB32ItoR(Rn, 4);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32RtoR(ECX, Rn);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_STORE(u32 code) {
	if (block_info.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			if (isMappedConst(n)) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rn);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMappedConst(n)) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32RtoR(ECX, Rn);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (isMappedConst(n)) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[n].constant);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32RtoR(ECX, Rn);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FMOV_INDEX_STORE(u32 code) {
	if (block_info.sz) {
		if ((code & 0x00000010) == 0) {
			u32 m = ((code >> 5) & 0x7) + 0;
			u32 n = ((code >> 8) & 0xf);

			if ((isMappedConst(n)) && (isMappedConst(0))) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(n)) {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(0)) {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32RRtoR(ECX, Rn, R0);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		} else {
			u32 m = ((code >> 5) & 0x7) + 8;
			u32 n = ((code >> 8) & 0xf);

			if ((isMappedConst(n)) && (isMappedConst(0))) {
				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(n)) {
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else if (isMappedConst(0)) {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			} else {
				x86Reg32Type Rn = GetX86reg(n, ModeRead);
				x86Reg32Type R0 = GetX86reg(0, ModeRead);

				if (isMapX86Reg(EDX)) PUSH32R(EDX);
				if (isMapX86Reg(ECX)) PUSH32R(ECX);

				LEA32RRtoR(ECX, Rn, R0);
				MOV32ItoR(EDX, (u32) & sh4.dbli[m]);

				CALL32((u32) & memWrite64);

				if (isMapX86Reg(ECX)) POP32R(ECX);
				if (isMapX86Reg(EDX)) POP32R(EDX);
			}
		}
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if ((isMappedConst(n)) && (isMappedConst(0))) {
			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			MOV32ItoR(ECX, ConstantMap[n].constant + ConstantMap[0].constant);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		} else if (isMappedConst(n)) {
			x86Reg32Type R0 = GetX86reg(0, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32IRtoR(ECX, R0, ConstantMap[n].constant);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		} else if (isMappedConst(0)) {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32IRtoR(ECX, Rn, ConstantMap[0].constant);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		} else {
			x86Reg32Type Rn = GetX86reg(n, ModeRead);
			x86Reg32Type R0 = GetX86reg(0, ModeRead);

			if (isMapX86Reg(EDX)) PUSH32R(EDX);
			if (isMapX86Reg(ECX)) PUSH32R(ECX);

			LEA32RRtoR(ECX, Rn, R0);
			MOV32MtoR(EDX, (u32) & sh4.flti[m]);

			CALL32((u32) & memWrite32);

			if (isMapX86Reg(ECX)) POP32R(ECX);
			if (isMapX86Reg(EDX)) POP32R(EDX);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FLDS(u32 code) {
	u32 m = ((code >> 8) & 0xf);

	MOV32MtoR(EAX, (u32) & sh4.flti[m]);
	MOV32RtoM((u32) & sh4.fpul, EAX);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FSTS(u32 code) {
	u32 n = ((code >> 8) & 0xf);

	MOV32MtoR(EAX, (u32) & sh4.fpul);
	MOV32RtoM((u32) & sh4.flt[n], EAX);

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FABS(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		AND32ItoM((u32) & sh4.dbli[n] + 4, 0x7fffffff);

		block_info.pc += 2;
		block_info.cycle++;
	} else {
		u32 n = ((code >> 8) & 0xf);

		AND32ItoM((u32) & sh4.flti[n], 0x7fffffff);

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FNEG(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		XOR32ItoM((u32) & sh4.dbli[n] + 4, 0x80000000);

		block_info.pc += 2;
		block_info.cycle++;
	} else {
		u32 n = ((code >> 8) & 0xf);

		XOR32ItoM((u32) & sh4.flti[n], 0x80000000);

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FCNVDS(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[m]);
		FSTP32M((u32) & sh4.fpul);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FCNVSD(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		FLD32M((u32) & sh4.fpul);
		FSTP64M((u32) & sh4.dbl[n]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FTRC(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[m]);
		FISTP32M((u32) & sh4.fpul);

		block_info.pc += 2;
		block_info.cycle += 2;
	} else {
		u32 m = ((code >> 8) & 0xf);

		if (cpuSSE) {
			CVTTSS2SI32MtoR(EAX, (u32) & sh4.flt[m]);
			MOV32RtoM((u32) & sh4.fpul, EAX);
		} else {
			FLD32M((u32) & sh4.flt[m]);
			FISTP32M((u32) & sh4.fpul);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FLOAT1(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		FILD32M((u32) & sh4.fpul);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 2;
	} else {
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			CVTTSI2SS32MtoXMM(XMM0, (u32) & sh4.fpul);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FILD32M((u32) & sh4.fpul);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FCMPEQ(u32 code) {
	u8 *link[2];

	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[m]);
		FLD64M((u32) & sh4.dbl[n]);
		FCOMIP(st1);
		FSTP32R(st0);

		link[0] = JE8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);

		block_info.pc += 2;
		block_info.cycle += 2;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			UCOMISS32MtoXMM(XMM0, (u32) & sh4.flt[m]);

			link[0] = JE8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else {
			FLD32M((u32) & sh4.flt[m]);
			FLD32M((u32) & sh4.flt[n]);
			FCOMIP(st1);
			FSTP32R(st0);

			link[0] = JE8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FCMPGT(u32 code) {
	u8 *link[2];

	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[m]);
		FLD64M((u32) & sh4.dbl[n]);
		FCOMIP(st1);
		FSTP32R(st0);

		link[0] = JA8(0);
		AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
		link[1] = JMP8(0);
		x86link8(link[0]);
		OR32ItoM((u32) & sh4.sr.all, 0x000000001);
		x86link8(link[1]);

		block_info.pc += 2;
		block_info.cycle += 2;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			UCOMISS32MtoXMM(XMM0, (u32) & sh4.flt[m]);

			link[0] = JA8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		} else {
			FLD32M((u32) & sh4.flt[m]);
			FLD32M((u32) & sh4.flt[n]);
			FCOMIP(st1);
			FSTP32R(st0);

			link[0] = JA8(0);
			AND32ItoM((u32) & sh4.sr.all, 0xfffffffe);
			link[1] = JMP8(0);
			x86link8(link[0]);
			OR32ItoM((u32) & sh4.sr.all, 0x000000001);
			x86link8(link[1]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FMAC(u32 code) {
	if (block_info.pr == 0) {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[m]);
			MULSSM32toXMM(XMM0, (u32) & sh4.flt[0]);
			ADDSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[m]);
			FMUL32M((u32) & sh4.flt[0]);
			FADD32M((u32) & sh4.flt[n]);
			FSTP32M((u32) & sh4.flt[n]);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FDIV(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[n]);
		FDIV64M((u32) & sh4.dbl[m]);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 23;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			DIVSS32MtoXMM(XMM0, (u32) & sh4.flt[m]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[n]);
			FDIV32M((u32) & sh4.flt[m]);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle += 10;
	}
}

static void INLINE FASTCALL FMUL(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[n]);
		FMUL64M((u32) & sh4.dbl[m]);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			MULSSM32toXMM(XMM0, (u32) & sh4.flt[m]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[n]);
			FMUL32M((u32) & sh4.flt[m]);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FSQRT(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.flt[n]);
		FSQRT32();
		FSTP64M((u32) & sh4.flt[n]);

		block_info.pc += 2;
		block_info.cycle += 22;
	} else {
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			SQRTSSM32toXMM(XMM0, (u32) & sh4.flt[n]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[n]);
			FSQRT32();
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle += 9;
	}
}

static void INLINE FASTCALL FSRRA(u32 code) {
	if (block_info.pr) {
		u32 n = ((code >> 9) & 0x7);

		FLD1();
		FLD64M((u32) & sh4.dbl[n]);
		FSQRT32();
		FDIVP32RtoST0(st1);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 22;
	} else {
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			static float const FLT1 = 1.0f;

			MOVSS32MtoXMM(XMM1, (u32) & FLT1);
			SQRTSSM32toXMM(XMM0, (u32) & sh4.flt[n]);
			DIVSS32XMMtoXMM(XMM1, XMM0);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM1);
		} else {
			FLD1();
			FLD32M((u32) & sh4.flt[n]);
			FSQRT32();
			FDIVP32RtoST0(st1);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle += 9;
	}
}

static void INLINE FASTCALL FADD(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[n]);
		FADD64M((u32) & sh4.dbl[m]);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			ADDSS32MtoXMM(XMM0, (u32) & sh4.flt[m]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[n]);
			FADD32M((u32) & sh4.flt[m]);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FSUB(u32 code) {
	if (block_info.pr) {
		u32 m = ((code >> 5) & 0x7);
		u32 n = ((code >> 9) & 0x7);

		FLD64M((u32) & sh4.dbl[n]);
		FSUB64M((u32) & sh4.dbl[m]);
		FSTP64M((u32) & sh4.dbl[n]);

		block_info.pc += 2;
		block_info.cycle += 6;
	} else {
		u32 m = ((code >> 4) & 0xf);
		u32 n = ((code >> 8) & 0xf);

		if (cpuSSE) {
			MOVSS32MtoXMM(XMM0, (u32) & sh4.flt[n]);
			SUBSSM32toXMM(XMM0, (u32) & sh4.flt[m]);
			MOVSS32XMMtoM((u32) & sh4.flt[n], XMM0);
		} else {
			FLD32M((u32) & sh4.flt[n]);
			FSUB32M((u32) & sh4.flt[m]);
			FSTP32M((u32) & sh4.flt[n]);
		}

		block_info.pc += 2;
		block_info.cycle++;
	}
}

static void INLINE FASTCALL FIPR(u32 code) {
	if (block_info.pr == 0) {
		u32 m = ((code >> 8) & 0x3);
		u32 n = ((code >> 10) & 0x3);

		if (cpuSSE) {
			MOVAPS128MtoXMM(XMM0, (u32) & sh4.vec[n][0]);
			MULPS128MtoXMM(XMM0, (u32) & sh4.vec[m][0]);
			MOVHLPS128XMMtoXMM(XMM1, XMM0);
			ADDPS128XMMtoXMM(XMM0, XMM1);
			MOVAPS128XMMtoXMM(XMM1, XMM0);
			SHUFPS128XMMtoXMM(XMM1, XMM1, 1);
			ADDSS32XMMtoXMM(XMM0, XMM1);
			MOVSS32XMMtoM((u32) & sh4.vec[n][3], XMM0);
		} else {
			FLD32M((u32) & sh4.vec[n][0]);
			FLD32M((u32) & sh4.vec[n][1]);
			FLD32M((u32) & sh4.vec[n][2]);
			FLD32M((u32) & sh4.vec[n][3]);
			FLD32M((u32) & sh4.vec[m][0]);
			FMULP32RtoST0(st4);
			FLD32M((u32) & sh4.vec[m][1]);
			FMULP32RtoST0(st3);
			FLD32M((u32) & sh4.vec[m][2]);
			FMULP32RtoST0(st2);
			FLD32M((u32) & sh4.vec[m][3]);
			FMULP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FSTP32M((u32) & sh4.vec[n][3]);
		}
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FTRV(u32 code) {
	if (block_info.pr == 0) {
		u32 n = ((code >> 10) & 0x3);

		if (cpuSSE) {
			MOVAPS128MtoXMM(XMM0, (u32) & sh4.vec[n][0]);
			MOVAPS128XMMtoXMM(XMM3, XMM0);
			SHUFPS128XMMtoXMM(XMM0, XMM0, 0);
			MOVAPS128XMMtoXMM(XMM1, XMM3);
			SHUFPS128XMMtoXMM(XMM3, XMM3, 0xff);
			MOVAPS128XMMtoXMM(XMM2, XMM1);
			SHUFPS128XMMtoXMM(XMM1, XMM1, 0x55);
			SHUFPS128XMMtoXMM(XMM2, XMM2, 0xaa);
			MULPS128MtoXMM(XMM0, (u32) & sh4.vec[4][0]);
			MULPS128MtoXMM(XMM1, (u32) & sh4.vec[5][0]);
			MULPS128MtoXMM(XMM2, (u32) & sh4.vec[6][0]);
			MULPS128MtoXMM(XMM3, (u32) & sh4.vec[7][0]);
			ADDPS128XMMtoXMM(XMM0, XMM1);
			ADDPS128XMMtoXMM(XMM2, XMM3);
			ADDPS128XMMtoXMM(XMM0, XMM2);
			MOVAPS128XMMtoM((u32) & sh4.vec[n][0], XMM0);
		} else {
			FLD32M((u32) & sh4.vec[n][0]);
			FLD32M((u32) & sh4.vec[n][1]);
			FLD32M((u32) & sh4.vec[n][2]);
			FLD32M((u32) & sh4.vec[n][3]);

			FLD32M((u32) & sh4.vec[4][0]);
			FMUL32ST0toR(st4);
			FLD32M((u32) & sh4.vec[5][0]);
			FMUL32ST0toR(st4);
			FADDP32RtoST0(st1);
			FLD32M((u32) & sh4.vec[6][0]);
			FMUL32ST0toR(st3);
			FLD32M((u32) & sh4.vec[7][0]);
			FMUL32ST0toR(st3);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FSTP32M((u32) & sh4.vec[n][0]);

			FLD32M((u32) & sh4.vec[4][1]);
			FMUL32ST0toR(st4);
			FLD32M((u32) & sh4.vec[5][1]);
			FMUL32ST0toR(st4);
			FADDP32RtoST0(st1);
			FLD32M((u32) & sh4.vec[6][1]);
			FMUL32ST0toR(st3);
			FLD32M((u32) & sh4.vec[7][1]);
			FMUL32ST0toR(st3);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FSTP32M((u32) & sh4.vec[n][1]);

			FLD32M((u32) & sh4.vec[4][2]);
			FMUL32ST0toR(st4);
			FLD32M((u32) & sh4.vec[5][2]);
			FMUL32ST0toR(st4);
			FADDP32RtoST0(st1);
			FLD32M((u32) & sh4.vec[6][2]);
			FMUL32ST0toR(st3);
			FLD32M((u32) & sh4.vec[7][2]);
			FMUL32ST0toR(st3);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FSTP32M((u32) & sh4.vec[n][2]);

			FLD32M((u32) & sh4.vec[4][3]);
			FMULP32RtoST0(st4);
			FLD32M((u32) & sh4.vec[5][3]);
			FMULP32RtoST0(st3);
			FLD32M((u32) & sh4.vec[6][3]);
			FMULP32RtoST0(st2);
			FLD32M((u32) & sh4.vec[7][3]);
			FMULP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FADDP32RtoST0(st1);
			FSTP32M((u32) & sh4.vec[n][3]);
		}
	}

	block_info.pc += 2;
	block_info.cycle += 4;
}

static void INLINE FASTCALL FSCA(u32 code) {
	if (block_info.pr == 0) {
		u32 n = ((code >> 8) & 0x0e);
		static float const angle = (float)(2.0f * 3.1415926535f / 65536.0f);

		FILD16M((u32) & sh4.fpul);
		FMUL32M((u32) & angle);
		FSINCOS32();
		FSTP32M((u32) & sh4.flt[n + 1]);
		FSTP32M((u32) & sh4.flt[n + 0]);
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FRCHG(u32 code) {
	XOR32ItoM((u32) & sh4.fpscr.all, 0x00200000);

	if (cpuSSE) {
		MOVAPS128MtoXMM(XMM0, (u32) & sh4.vec[0][0]);
		MOVAPS128MtoXMM(XMM1, (u32) & sh4.vec[1][0]);
		MOVAPS128MtoXMM(XMM2, (u32) & sh4.vec[2][0]);
		MOVAPS128MtoXMM(XMM3, (u32) & sh4.vec[3][0]);
		MOVAPS128MtoXMM(XMM4, (u32) & sh4.vec[4][0]);
		MOVAPS128MtoXMM(XMM5, (u32) & sh4.vec[5][0]);
		MOVAPS128MtoXMM(XMM6, (u32) & sh4.vec[6][0]);
		MOVAPS128MtoXMM(XMM7, (u32) & sh4.vec[7][0]);

		MOVAPS128XMMtoM((u32) & sh4.vec[0][0], XMM4);
		MOVAPS128XMMtoM((u32) & sh4.vec[1][0], XMM5);
		MOVAPS128XMMtoM((u32) & sh4.vec[2][0], XMM6);
		MOVAPS128XMMtoM((u32) & sh4.vec[3][0], XMM7);
		MOVAPS128XMMtoM((u32) & sh4.vec[4][0], XMM0);
		MOVAPS128XMMtoM((u32) & sh4.vec[5][0], XMM1);
		MOVAPS128XMMtoM((u32) & sh4.vec[6][0], XMM2);
		MOVAPS128XMMtoM((u32) & sh4.vec[7][0], XMM3);
	} else {
		MOV64MtoMMX(MM0, (u32) & sh4.dbl[0]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[1]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[2]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[3]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[8]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[9]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[10]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[11]);
		MOV64MMXtoM((u32) & sh4.dbl[0], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[1], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[2], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[3], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[8], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[9], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[10], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[11], MM3);

		MOV64MtoMMX(MM0, (u32) & sh4.dbl[4]);
		MOV64MtoMMX(MM1, (u32) & sh4.dbl[5]);
		MOV64MtoMMX(MM2, (u32) & sh4.dbl[6]);
		MOV64MtoMMX(MM3, (u32) & sh4.dbl[7]);
		MOV64MtoMMX(MM4, (u32) & sh4.dbl[12]);
		MOV64MtoMMX(MM5, (u32) & sh4.dbl[13]);
		MOV64MtoMMX(MM6, (u32) & sh4.dbl[14]);
		MOV64MtoMMX(MM7, (u32) & sh4.dbl[15]);
		MOV64MMXtoM((u32) & sh4.dbl[4], MM4);
		MOV64MMXtoM((u32) & sh4.dbl[5], MM5);
		MOV64MMXtoM((u32) & sh4.dbl[6], MM6);
		MOV64MMXtoM((u32) & sh4.dbl[7], MM7);
		MOV64MMXtoM((u32) & sh4.dbl[12], MM0);
		MOV64MMXtoM((u32) & sh4.dbl[13], MM1);
		MOV64MMXtoM((u32) & sh4.dbl[14], MM2);
		MOV64MMXtoM((u32) & sh4.dbl[15], MM3);

		if (cpu3DNOW) FEMMS();
		else EMMS();
	}

	block_info.pc += 2;
	block_info.cycle++;
}

static void INLINE FASTCALL FSCHG(u32 code) {
	XOR32ItoM((u32) & sh4.fpscr.all, 0x00100000);

	block_info.pc += 2;
	block_info.cycle++;

	if (!block_info.compile) {
		block_info.need_linking_block1 = 1;
		block_info.block_pc1 = block_info.pc;
		block_info.compile = 1;
	}
}

int recOpen() {
	cpuid();

	if (!cpuMMX) return 0;

	x86ptr = (u8*)prec;

	return 1;
}

void recClose() {
}

void recCompileBlock(u32 pc) {
	block_info.pc = pc;
	block_info.cycle = 0;
	block_info.compile = 0;
	block_info.need_linking_block1 = 0;
	block_info.need_linking_block2 = 0;

	while (!block_info.compile)
		recStep(block_info.pc);

	Flushall();

	if (block_info.need_linking_block1) {
		u32 entry;

		if ((block_info.block_pc1 & 0x1fffffff) < 0x00200000)
			entry = (u32)precrom[(block_info.block_pc1 & 0x001fffff) >> 1];
		else
			entry = (u32)precram[(block_info.block_pc1 & 0x00ffffff) >> 1];

		if (entry) {
			u32 *link;

			SUB32ItoM((u32) & iount, (u32)block_info.cycle);
			link = JG32(0); *link = entry - (u32)link - 4;
			MOV32ItoM((u32) & sh4.pc, block_info.block_pc1);
			RET();
		} else {
			SUB32ItoM((u32) & iount, (u32)block_info.cycle);
			MOV32ItoM((u32) & sh4.pc, block_info.block_pc1);
			RET();
		}
	}

	if (block_info.need_linking_block2) {
		u32 entry;

		x86link8(block_info.link);

		if ((block_info.block_pc2 & 0x1fffffff) < 0x00200000)
			entry = (u32)precrom[(block_info.block_pc2 & 0x001fffff) >> 1];
		else
			entry = (u32)precram[(block_info.block_pc2 & 0x00ffffff) >> 1];

		if (entry) {
			u32 *link;

			SUB32ItoM((u32) & iount, (u32)block_info.cycle);
			link = JG32(0); *link = entry - (u32)link - 4;
			MOV32ItoM((u32) & sh4.pc, block_info.block_pc2);
			RET();
		} else {
			SUB32ItoM((u32) & iount, (u32)block_info.cycle);
			MOV32ItoM((u32) & sh4.pc, block_info.block_pc2);
			RET();
		}
	}

	if ((block_info.need_linking_block1 == 0) &&
		(block_info.need_linking_block2 == 0)) {
		SUB32ItoM((u32) & iount, (u32)block_info.cycle);
		RET();
	}
}

void recAnalyzeBlock(u32 pc) {
	u32 code;
	int EndBlock = 0;

	block_info.usefpu = 0;
	block_info.need_test_block = 0;

	if ((pc & 0x1fffffff) > 0x00200000) {
		block_info.need_test_block = 1;
		block_info.start_addr = pc;
		block_info.start_code = *(u16*)&pram[pc & 0x00ffffff];
	}

	while (!EndBlock) {
 next:   if ((pc & 0x1fffffff) < 0x00200000)
			code = *(u16*)&prom[pc & 0x001fffff];
		else
			code = *(u16*)&pram[pc & 0x00ffffff];

		if ((code & 0xf000) == 0xf000) {
			block_info.usefpu = 1;
			EndBlock = 1;

			if (block_info.need_test_block) {
				block_info.finish_addr = pc;
				block_info.finish_code = *(u16*)&pram[pc & 0x00ffffff];
			}
		} else if
		(
			((code & 0xffff) == 0x002b) ||	//RTE
			((code & 0xffff) == 0x000b) ||	//RTS
			((code & 0xf0ff) == 0x402b) ||	//JMP
			((code & 0xf0ff) == 0x400b) ||	//JSR
			((code & 0xf0ff) == 0x0023) ||	//BRAF
			((code & 0xf0ff) == 0x0003) ||	//BSRF
			((code & 0xff00) == 0x8f00) ||	//BFS
			((code & 0xff00) == 0x8d00) ||	//BTS
			((code & 0xf000) == 0xa000) ||	//BRA
			((code & 0xf000) == 0xb000)		//BSR
		) {
			EndBlock = 1;
			pc += 2;

			if (block_info.need_test_block) {
				block_info.finish_addr = pc;
				block_info.finish_code = *(u16*)&pram[pc & 0x00ffffff];
			}

			goto next;
		} else if
		(
			((code & 0xffff) == 0xf3fd) ||	//FSCHG
			((code & 0xff00) == 0x8b00) ||	//BF
			((code & 0xff00) == 0x8900) ||	//BT
			((code & 0xf0ff) == 0x4066) ||	//LDSMFPSCR
			((code & 0xf0ff) == 0x406a)		//LDSFPSCR
		) {
			EndBlock = 1;

			if (block_info.need_test_block) {
				block_info.finish_addr = pc;
				block_info.finish_code = (u32) * (u16*)&pram[pc & 0x00ffffff];
			}
		}

		pc += 2;
	}
}

void *recCompile() {
	void *entry;

	if (((u32)x86ptr - (u32)prec) >= (0x01000000 - 0x10000)) {
		memset((void*)precrom, 0, 0x00200000 >> 1);
		memset((void*)precram, 0, 0x01000000 >> 1);
		x86ptr = (u8*)prec;
	}

	x86ptr = (u8*)align_memory(x86ptr, 32);
	entry = (void*)x86ptr;

	if ((sh4.pc & 0x1fffffff) < 0x00200000)
		precrom[(sh4.pc & 0x001fffff) >> 1] = (int)x86ptr;
	else
		precram[(sh4.pc & 0x00ffffff) >> 1] = (int)x86ptr;

	recAnalyzeBlock(sh4.pc);

	if (block_info.need_test_block) {
		u8 *link[2];
		u8 *entry = x86ptr;

		CMP16ItoM((u32) & pram[block_info.start_addr & 0x00ffffff], block_info.start_code);
		link[0] = JNE8(0);
		CMP16ItoM((u32) & pram[block_info.finish_addr & 0x00ffffff], block_info.finish_code);
		link[1] = JE8(0);
		x86link8(link[0]);
		MOV8ItoM((u32)entry, 0xc3);
		MOV32ItoM((u32) & precram[((block_info.start_addr & 0x00ffffff) >> 1)], 0);
		RET();
		x86link8(link[1]);
	}

	if (block_info.usefpu) {
		u32 *link[2];

		MOV32MtoR(EAX, (u32) & sh4.fpscr.all);
		AND32ItoR(EAX, 0x00180000);
		JNZ8(2);
		JMP8(16);					//PR = 0; SZ = 0;
		TEST32ItoR(EAX, 0x00100000);
		link[0] = JNZ32(0);			//PR = 0; SZ = 1;
		link[1] = JMP32(0);			//PR = 1; SZ = 0;

		block_info.pr = 0;
		block_info.sz = 0;
		recCompileBlock(sh4.pc);

		x86link32(link[0]);
		block_info.pr = 0;
		block_info.sz = 1;
		recCompileBlock(sh4.pc);

		x86link32(link[1]);
		block_info.pr = 1;
		block_info.sz = 0;
		recCompileBlock(sh4.pc);
	} else {
		recCompileBlock(sh4.pc);
	}

	return (void*)entry;
}

void recExecute() {
	iount = 1024;

	do {
		void (*recExecuteCode)();

		if ((sh4.pc & 0x1fffffff) < 0x00200000)
			recExecuteCode = (void*)precrom[(sh4.pc & 0x001fffff) >> 1];
		else
			recExecuteCode = (void*)precram[(sh4.pc & 0x00ffffff) >> 1];

		if (recExecuteCode == NULL)
			recExecuteCode = (void*)recCompile();

#ifdef _DEBUG
		__asm pushad
#endif
		recExecuteCode();
#ifdef _DEBUG
		__asm popad
#endif
	} while (iount > 0);

	sh4.cycle += 1024 - iount;

	CpuTest();
}

void recStep(u32 pc) {
	u32 code;

	if ((pc & 0x1fffffff) < 0x00200000)
		code = *(u16*)&prom[pc & 0x001fffff];
	else
		code = *(u16*)&pram[pc & 0x00ffffff];

	if ((pc == 0x8c0548e4) && (code == 0x6022)) code = 0x7001;	// Psyvariar 2
	if ((pc == 0x0C14B2F2) && (code == 0x30E0)) code = 0x0009;	// Gigawing 2
	if ((pc == 0x8C033792) && (code == 0x30E0)) code = 0x0009;	// Jedi Power Battles

#ifdef STAT_ENABLE
	statUpdate(code);
#endif

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
			}

		case 9:
			switch ((code >> 4) & 0xf) {
			case 0:  NOP(code);      return;
			case 1:  DIV0U(code);    return;
			case 2:  MOVT(code);     return;
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
			}

		case 11:
			switch ((code >> 4) & 0xf) {
			case 0:  RTS(code);      return;
			case 1:  SLEEP(code);    return;
			case 2:  RTE(code);      return;
			}

		case 12: MOVBL0(code);  return;
		case 13: MOVWL0(code);  return;
		case 14: MOVLL0(code);  return;
		case 15: MACL(code);    return;
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
		}

	case 4:
		switch ((code >> 0) & 0xf) {
		case 0:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLL(code);     return;
			case 1:  DT(code);       return;
			case 2:  SHAL(code);     return;
			}

		case 1:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLR(code);     return;
			case 1:  CMPPZ(code);    return;
			case 2:  SHAR(code);     return;
			}

		case 2:
			switch ((code >> 4) & 0xf) {
			case 0:  STSMMACH(code); return;
			case 1:  STSMMACL(code); return;
			case 2:  STSMPR(code);   return;
			case 5:  STSMFPUL(code); return;
			case 6:  STSMFPSCR(code); return;
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
			}

		case 4:
			switch ((code >> 4) & 0xf) {
			case 0:  ROTL(code);     return;
			case 2:  ROTCL(code);    return;
			}

		case 5:
			switch ((code >> 4) & 0xf) {
			case 0:  ROTR(code);     return;
			case 1:  CMPPL(code);    return;
			case 2:  ROTCR(code);    return;
			}

		case 6:
			switch ((code >> 4) & 0xf) {
			case 0:  LDSMMACH(code); return;
			case 1:  LDSMMACL(code); return;
			case 2:  LDSMPR(code);   return;
			case 5:  LDSMFPUL(code); return;
			case 6:  LDSMFPSCR(code); return;
			case 15: LDCMDBR(code);  return;
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
			}

		case 8:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLL2(code);    return;
			case 1:  SHLL8(code);    return;
			case 2:  SHLL16(code);   return;
			}

		case 9:
			switch ((code >> 4) & 0xf) {
			case 0:  SHLR2(code);    return;
			case 1:  SHLR8(code);    return;
			case 2:  SHLR16(code);   return;
			}

		case 10:
			switch ((code >> 4) & 0xf) {
			case 0:  LDSMACH(code);  return;
			case 1:  LDSMACL(code);  return;
			case 2:  LDSPR(code);    return;
			case 5:  LDSFPUL(code);  return;
			case 6:  LDSFPSCR(code); return;
			case 15: LDCDBR(code);   return;
			}

		case 11:
			switch ((code >> 4) & 0xf) {
			case 0:  JSR(code);      return;
			case 1:  TAS(code);      return;
			case 2:  JMP(code);      return;
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
			}

		case 15: MACW(code);    return;
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
				}
			}

		case 14: FMAC(code);    return;
		}
	}
	NI(code);
}
