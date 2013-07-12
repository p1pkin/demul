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
#include <string.h>

#include "demul.h"
#include "plugins.h"
#include "counters.h"
#include "memory.h"
#include "aica.h"
#include "arm.h"

static s32 icount;

u8 INLINE FASTCALL READ8(u32 mem) {
	if (mem < 0x00200000) {
		return ARAM8_UNMASKED(mem);
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		return ReadAica8(mem);
	}

	Printf("Unknown arm memRead8  address = 0x%08lx\n", mem);
	return 0;
}

u16 INLINE FASTCALL READ16(u32 mem) {
	if (mem < 0x00200000) {
		return ARAM16_UNMASKED(mem);
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		return ReadAica16(mem);
	}

	Printf("Unknown arm memRead8  address = 0x%08lx\n", mem);
	return 0;
}

u32 INLINE FASTCALL READ32(u32 mem) {
	if (mem < 0x00200000) {
		return ARAM32_UNMASKED(mem);
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		return ReadAica32(mem);
	}

	Printf("Unknown arm memRead32 address = 0x%08lx\n", mem);
	return 0;
}

void INLINE FASTCALL WRITE8(u32 mem, u8 value) {
	if (mem < 0x00200000) {
		ARAM8_UNMASKED(mem) = value;
		return;
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		WriteAica8(mem, value);
		return;
	}

	Printf("Unknown arm memWrite8  address = 0x%08lx, value = 0x%08lx\n ", mem, value);
}

void INLINE FASTCALL WRITE16(u32 mem, u16 value) {
	if (mem < 0x00200000) {
		ARAM16_UNMASKED(mem) = value;
		return;
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		WriteAica16(mem, value);
		return;
	}

	Printf("Unknown arm memWrite8  address = 0x%08lx, value = 0x%08lx\n ", mem, value);
}

void INLINE FASTCALL WRITE32(u32 mem, u32 value) {
	if (mem < 0x00200000) {
		ARAM32_UNMASKED(mem) = value;
		return;
	}

	if ((mem >= 0x00800000) &&
		(mem < 0x00810000)) {
		WriteAica32(mem, value);
		return;
	}

	Printf("Unknown arm memWrite32 address = 0x%08lx, value = 0x%08lx\n ", mem, value);
}

static u8 cpuBitsSet[256];

u32 armOpen() {
	int i;

	for (i = 0; i < 256; i++) {
		int count = 0;
		int j;

		for (j = 0; j < 8; j++) if (i & (1 << j)) count++;

		cpuBitsSet[i] = count;
	}

	AICA32(0x2c00) |= 1;
	armReset();

	return 1;
}

void armReset() {
	memset(&arm, 0, sizeof(ARM));
	armSwitchMode(ARM_SVC, true);
	arm.cpsr |= ARM_F | ARM_I;
	arm.r[15] = 0x00 + 4;
	arm.pc = 0x00;
}

void armClose() {
}

void armFIQ() {
	if ((arm.cpsr & ARM_F) == 0) {
		AICA32(0x2d00) = 2;

		armSwitchMode(ARM_FIQ, true);
		arm.cpsr |= ARM_F | ARM_I;
		arm.r[14] = arm.r[15];
		arm.r[15] = 0x1c + 4;
		arm.pc = 0x1c;
	}
}

void armUpdateCPSR() {
	u32 CPSR = arm.cpsr & (ARM_I | ARM_F);
	if (N_FLAG) CPSR |= 0x80000000;
	if (Z_FLAG) CPSR |= 0x40000000;
	if (C_FLAG) CPSR |= 0x20000000;
	if (V_FLAG) CPSR |= 0x10000000;
	CPSR |= arm.mode;
	arm.cpsr = CPSR;
}

void armUpdateFlags() {
	u32 CPSR = arm.cpsr;

	N_FLAG = (CPSR & 0x80000000) ? true : false;
	Z_FLAG = (CPSR & 0x40000000) ? true : false;
	C_FLAG = (CPSR & 0x20000000) ? true : false;
	V_FLAG = (CPSR & 0x10000000) ? true : false;
}

void armSwitchMode(int mode, bool state) {
	u32 cpsr;
	u32 spsr;

	armUpdateCPSR();

	switch (arm.mode) {
	case 0x10:
	case 0x1F:
		arm.r13usr = arm.r[13];
		arm.r14usr = arm.r[14];
		arm.spsr = arm.cpsr;
		break;
	case 0x11:
	{
		u32 value;

		value = arm.r[8];
		arm.r[8] = arm.r8fiq;
		arm.r8fiq = value;

		value = arm.r[9];
		arm.r[9] = arm.r9fiq;
		arm.r9fiq = value;

		value = arm.r[10];
		arm.r[10] = arm.r10fiq;
		arm.r10fiq = value;

		value = arm.r[11];
		arm.r[11] = arm.r11fiq;
		arm.r11fiq = value;

		value = arm.r[12];
		arm.r[12] = arm.r12fiq;
		arm.r12fiq = value;

		arm.r13fiq = arm.r[13];
		arm.r14fiq = arm.r[14];
		arm.fiqspsr = arm.spsr;
		break;
	}
	case 0x12:
		arm.r13irq = arm.r[13];
		arm.r14irq = arm.r[14];
		arm.irqspsr = arm.spsr;
		break;
	case 0x13:
		arm.r13svc = arm.r[13];
		arm.r14svc = arm.r[14];
		arm.svcspsr = arm.spsr;
		break;
	case 0x17:
		arm.r13abt = arm.r[13];
		arm.r14abt = arm.r[14];
		arm.abtspsr = arm.spsr;
		break;
	case 0x1b:
		arm.r13und = arm.r[13];
		arm.r14und = arm.r[14];
		arm.undspsr = arm.spsr;
		break;
	}

	cpsr = arm.cpsr;
	spsr = arm.spsr;

	switch (mode) {
	case 0x10:
	case 0x1F:
		arm.r[13] = arm.r13usr;
		arm.r[14] = arm.r14usr;
		arm.cpsr = spsr;
		break;
	case 0x11:
	{
		u32 value;

		value = arm.r[8];
		arm.r[8] = arm.r8fiq;
		arm.r8fiq = value;

		value = arm.r[9];
		arm.r[9] = arm.r9fiq;
		arm.r9fiq = value;

		value = arm.r[10];
		arm.r[10] = arm.r10fiq;
		arm.r10fiq = value;

		value = arm.r[11];
		arm.r[11] = arm.r11fiq;
		arm.r11fiq = value;

		value = arm.r[12];
		arm.r[12] = arm.r12fiq;
		arm.r12fiq = value;

		arm.r[13] = arm.r13fiq;
		arm.r[14] = arm.r14fiq;
		if (state) arm.spsr = cpsr;
		else arm.spsr = arm.fiqspsr;
		break;
	}
	case 0x12:
		arm.r[13] = arm.r13irq;
		arm.r[14] = arm.r14irq;
		arm.cpsr = spsr;
		if (state) arm.spsr = cpsr;
		else arm.spsr = arm.irqspsr;
		break;
	case 0x13:
		arm.r[13] = arm.r13svc;
		arm.r[14] = arm.r14svc;
		arm.cpsr = spsr;
		if (state) arm.spsr = cpsr;
		else arm.spsr = arm.svcspsr;
		break;
	case 0x17:
		arm.r[13] = arm.r13abt;
		arm.r[14] = arm.r14abt;
		arm.cpsr = spsr;
		if (state) arm.spsr = cpsr;
		else arm.spsr = arm.abtspsr;
		break;
	case 0x1b:
		arm.r[13] = arm.r13und;
		arm.r[14] = arm.r14und;
		arm.cpsr = spsr;
		if (state) arm.spsr = cpsr;
		else arm.spsr = arm.undspsr;
		break;
	default:
		break;
	}

	arm.mode = mode;
	armUpdateFlags();
	armUpdateCPSR();
}

void armExecute(u32 cycle) {
	icount = cycle;

	do {
		bool cond;
		u32 opcode = ARAM32(arm.pc);

		arm.pc = arm.r[15];
		arm.r[15] += 4;

		switch (opcode >> 28) {
		case 0x00:	// EQ
			cond = Z_FLAG;
			break;
		case 0x01:	// NE
			cond = !Z_FLAG;
			break;
		case 0x02:	// CS
			cond = C_FLAG;
			break;
		case 0x03:	// CC
			cond = !C_FLAG;
			break;
		case 0x04:	// MI
			cond = N_FLAG;
			break;
		case 0x05:	// PL
			cond = !N_FLAG;
			break;
		case 0x06:	// VS
			cond = V_FLAG;
			break;
		case 0x07:	// VC
			cond = !V_FLAG;
			break;
		case 0x08:	// HI
			cond = C_FLAG && !Z_FLAG;
			break;
		case 0x09:	// LS
			cond = !C_FLAG || Z_FLAG;
			break;
		case 0x0A:	// GE
			cond = N_FLAG == V_FLAG;
			break;
		case 0x0B:	// LT
			cond = N_FLAG != V_FLAG;
			break;
		case 0x0C:	// GT
			cond = !Z_FLAG && (N_FLAG == V_FLAG);
			break;
		case 0x0D:	// LE
			cond = Z_FLAG || (N_FLAG != V_FLAG);
			break;
		case 0x0E:
			cond = true;
			break;
		case 0x0F:
			cond = false;
			break;
		}

		if (cond) {
			switch (((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0x0F)) {
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_AND, OP_AND, 0x000);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_ANDS, OP_AND, 0x010);
			case 0x009:
			{
				// MUL Rd, Rm, Rs
				int dest = (opcode >> 16) & 0x0F;
				int mult = (opcode & 0x0F);
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				arm.r[dest] = arm.r[mult] * rs;
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((rs & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
			case 0x019:
			{
				// MULS Rd, Rm, Rs
				int dest = (opcode >> 16) & 0x0F;
				int mult = (opcode & 0x0F);
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				arm.r[dest] = arm.r[mult] * rs;
				N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;
				Z_FLAG = (arm.r[dest]) ? false : true;
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((rs & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
			case 0x00b:
			case 0x02b:
			{
				// STRH Rd, [Rn], -Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				address -= offset;
				arm.r[base] = address;
			}
			break;
			case 0x04b:
			case 0x06b:
			{
				// STRH Rd, [Rn], #-offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				address -= offset;
				arm.r[base] = address;
			}
			break;
			case 0x08b:
			case 0x0ab:
			{
				// STRH Rd, [Rn], Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				address += offset;
				arm.r[base] = address;
			}
			break;
			case 0x0cb:
			case 0x0eb:
			{
				// STRH Rd, [Rn], #offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				address += offset;
				arm.r[base] = address;
			}
			break;
			case 0x10b:
			{
				// STRH Rd, [Rn, -Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
			}
			break;
			case 0x12b:
			{
				// STRH Rd, [Rn, -Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				arm.r[base] = address;
			}
			break;
			case 0x14b:
			{
				// STRH Rd, [Rn, -#offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 4;
				WRITE16(address, arm.r[dest]);
			}
			break;
			case 0x16b:
			{
				// STRH Rd, [Rn, -#offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				arm.r[base] = address;
			}
			break;
			case 0x18b:
			{
				// STRH Rd, [Rn, Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
			}
			break;
			case 0x1ab:
			{
				// STRH Rd, [Rn, Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				arm.r[base] = address;
			}
			break;
			case 0x1cb:
			{
				// STRH Rd, [Rn, #offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 4;
				WRITE16(address, arm.r[dest]);
			}
			break;
			case 0x1eb:
			{
				// STRH Rd, [Rn, #offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 4;
				WRITE16(address, arm.r[dest]);
				arm.r[base] = address;
			}
			break;
			case 0x01b:
			case 0x03b:
			{
				// LDRH Rd, [Rn], -Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x05b:
			case 0x07b:
			{
				// LDRH Rd, [Rn], #-offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x09b:
			case 0x0bb:
			{
				// LDRH Rd, [Rn], Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x0db:
			case 0x0fb:
			{
				// LDRH Rd, [Rn], #offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x11b:
			{
				// LDRH Rd, [Rn, -Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
			}
			break;
			case 0x13b:
			{
				// LDRH Rd, [Rn, -Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x15b:
			{
				// LDRH Rd, [Rn, -#offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = READ16(address);
			}
			break;
			case 0x17b:
			{
				// LDRH Rd, [Rn, -#offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x19b:
			{
				// LDRH Rd, [Rn, Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
			}
			break;
			case 0x1bb:
			{
				// LDRH Rd, [Rn, Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x1db:
			{
				// LDRH Rd, [Rn, #offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = READ16(address);
			}
			break;
			case 0x1fb:
			{
				// LDRH Rd, [Rn, #offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x01d:
			case 0x03d:
			{
				// LDRSB Rd, [Rn], -Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x05d:
			case 0x07d:
			{
				// LDRSB Rd, [Rn], #-offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x09d:
			case 0x0bd:
			{
				// LDRSB Rd, [Rn], Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x0dd:
			case 0x0fd:
			{
				// LDRSB Rd, [Rn], #offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x11d:
			{
				// LDRSB Rd, [Rn, -Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
			}
			break;
			case 0x13d:
			{
				// LDRSB Rd, [Rn, -Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x15d:
			{
				// LDRSB Rd, [Rn, -#offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
			}
			break;
			case 0x17d:
			{
				// LDRSB Rd, [Rn, -#offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x19d:
			{
				// LDRSB Rd, [Rn, Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
			}
			break;
			case 0x1bd:
			{
				// LDRSB Rd, [Rn, Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x1dd:
			{
				// LDRSB Rd, [Rn, #offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
			}
			break;
			case 0x1fd:
			{
				// LDRSB Rd, [Rn, #offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (s8)READ8(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x01f:
			case 0x03f:
			{
				// LDRSH Rd, [Rn], -Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x05f:
			case 0x07f:
			{
				// LDRSH Rd, [Rn], #-offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base) {
					address -= offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x09f:
			case 0x0bf:
			{
				// LDRSH Rd, [Rn], Rm
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x0df:
			case 0x0ff:
			{
				// LDRSH Rd, [Rn], #offset
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base];
				int offset = (opcode & 0x0F) | ((opcode >> 4) & 0xF0);
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base) {
					address += offset;
					arm.r[base] = address;
				}
			}
			break;
			case 0x11f:
			{
				// LDRSH Rd, [Rn, -Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
			}
			break;
			case 0x13f:
			{
				// LDRSH Rd, [Rn, -Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x15f:
			{
				// LDRSH Rd, [Rn, -#offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
			}
			break;
			case 0x17f:
			{
				// LDRSH Rd, [Rn, -#offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] - ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x19f:
			{
				// LDRSH Rd, [Rn, Rm]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
			}
			break;
			case 0x1bf:
			{
				// LDRSH Rd, [Rn, Rm]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + arm.r[opcode & 0x0F];
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
			case 0x1df:
			{
				// LDRSH Rd, [Rn, #offset]
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
			}
			break;
			case 0x1ff:
			{
				// LDRSH Rd, [Rn, #offset]!
				int base = (opcode >> 16) & 0x0F;
				int dest = (opcode >> 12) & 0x0F;
				u32 address = arm.r[base] + ((opcode & 0x0F) | ((opcode >> 4) & 0xF0));
				icount -= 3;
				arm.r[dest] = (u32)(s32)(s16)READ16(address);
				if (dest != base)
					arm.r[base] = address;
			}
			break;
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_EOR, OP_EOR, 0x020);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_EORS, OP_EOR, 0x030);
			case 0x029:
			{
				// MLA Rd, Rm, Rs, Rn
				int dest = (opcode >> 16) & 0x0F;
				int mult = (opcode & 0x0F);
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				arm.r[dest] = arm.r[mult] * rs + arm.r[(opcode >> 12) & 0x0f];
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((rs & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
			case 0x039:
			{
				// MLAS Rd, Rm, Rs, Rn
				int dest = (opcode >> 16) & 0x0F;
				int mult = (opcode & 0x0F);
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				arm.r[dest] = arm.r[mult] * rs + arm.r[(opcode >> 12) & 0x0f];
				N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;
				Z_FLAG = (arm.r[dest]) ? false : true;
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((rs & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
				ARITHMETIC_DATA_OPCODE(OP_SUB, OP_SUB, 0x040);
				ARITHMETIC_DATA_OPCODE(OP_SUBS, OP_SUB, 0x050);
				ARITHMETIC_DATA_OPCODE(OP_RSB, OP_RSB, 0x060);
				ARITHMETIC_DATA_OPCODE(OP_RSBS, OP_RSB, 0x070);
				ARITHMETIC_DATA_OPCODE(OP_ADD, OP_ADD, 0x080);
				ARITHMETIC_DATA_OPCODE(OP_ADDS, OP_ADD, 0x090);
			case 0x089:
			{
				// UMULL RdLo, RdHi, Rn, Rs
				u32 umult = arm.r[(opcode & 0x0F)];
				u32 usource = arm.r[(opcode >> 8) & 0x0F];
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u64 uTemp = ((u64)umult) * ((u64)usource);
				arm.r[destLo] = (u32)uTemp;
				arm.r[destHi] = (u32)(uTemp >> 32);
				if ((usource & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((usource & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((usource & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
			case 0x099:
			{
				// UMULLS RdLo, RdHi, Rn, Rs
				u32 umult = arm.r[(opcode & 0x0F)];
				u32 usource = arm.r[(opcode >> 8) & 0x0F];
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u64 uTemp = ((u64)umult) * ((u64)usource);
				arm.r[destLo] = (u32)uTemp;
				arm.r[destHi] = (u32)(uTemp >> 32);
				Z_FLAG = (uTemp) ? false : true;
				N_FLAG = (arm.r[destHi] & 0x80000000) ? true : false;
				if ((usource & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((usource & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((usource & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
				ARITHMETIC_DATA_OPCODE(OP_ADC, OP_ADC, 0x0a0);
				ARITHMETIC_DATA_OPCODE(OP_ADCS, OP_ADC, 0x0b0);
			case 0x0a9:
			{
				// UMLAL RdLo, RdHi, Rn, Rs
				u32 umult = arm.r[(opcode & 0x0F)];
				u32 usource = arm.r[(opcode >> 8) & 0x0F];
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u64 uTemp = (u64)arm.r[destHi];
				uTemp <<= 32;
				uTemp |= (u64)arm.r[destLo];
				uTemp += ((u64)umult) * ((u64)usource);
				arm.r[destLo] = (u32)uTemp;
				arm.r[destHi] = (u32)(uTemp >> 32);
				if ((usource & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((usource & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((usource & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
			case 0x0b9:
			{
				// UMLALS RdLo, RdHi, Rn, Rs
				u32 umult = arm.r[(opcode & 0x0F)];
				u32 usource = arm.r[(opcode >> 8) & 0x0F];
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u64 uTemp = (u64)arm.r[destHi];
				uTemp <<= 32;
				uTemp |= (u64)arm.r[destLo];
				uTemp += ((u64)umult) * ((u64)usource);
				arm.r[destLo] = (u32)uTemp;
				arm.r[destHi] = (u32)(uTemp >> 32);
				Z_FLAG = (uTemp) ? false : true;
				N_FLAG = (arm.r[destHi] & 0x80000000) ? true : false;
				if ((usource & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((usource & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((usource & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
				ARITHMETIC_DATA_OPCODE(OP_SBC, OP_SBC, 0x0c0);
				ARITHMETIC_DATA_OPCODE(OP_SBCS, OP_SBC, 0x0d0);
			case 0x0c9:
			{
				// SMULL RdLo, RdHi, Rm, Rs
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				s64 m = (s32)arm.r[(opcode & 0x0F)];
				s64 s = (s32)rs;
				s64 sTemp = m * s;
				arm.r[destLo] = (u32)sTemp;
				arm.r[destHi] = (u32)(sTemp >> 32);
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((rs & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
			case 0x0d9:
			{
				// SMULLS RdLo, RdHi, Rm, Rs
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				s64 m = (s32)arm.r[(opcode & 0x0F)];
				s64 s = (s32)rs;
				s64 sTemp = m * s;
				arm.r[destLo] = (u32)sTemp;
				arm.r[destHi] = (u32)(sTemp >> 32);
				Z_FLAG = (sTemp) ? false : true;
				N_FLAG = (sTemp < 0) ? true : false;
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 2;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 3;
				else if ((rs & 0xFF000000) == 0)
					icount -= 4;
				else
					icount -= 5;
			}
			break;
				ARITHMETIC_DATA_OPCODE(OP_RSC, OP_RSC, 0x0e0);
				ARITHMETIC_DATA_OPCODE(OP_RSCS, OP_RSC, 0x0f0);
			case 0x0e9:
			{
				// SMLAL RdLo, RdHi, Rm, Rs
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				s64 m = (s32)arm.r[(opcode & 0x0F)];
				s64 s = (s32)rs;
				s64 sTemp = (u64)arm.r[destHi];
				sTemp <<= 32;
				sTemp |= (u64)arm.r[destLo];
				sTemp += m * s;
				arm.r[destLo] = (u32)sTemp;
				arm.r[destHi] = (u32)(sTemp >> 32);
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((rs & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
			case 0x0f9:
			{
				// SMLALS RdLo, RdHi, Rm, Rs
				int destLo = (opcode >> 12) & 0x0F;
				int destHi = (opcode >> 16) & 0x0F;
				u32 rs = arm.r[(opcode >> 8) & 0x0F];
				s64 m = (s32)arm.r[(opcode & 0x0F)];
				s64 s = (s32)rs;
				s64 sTemp = (u64)arm.r[destHi];
				sTemp <<= 32;
				sTemp |= (u64)arm.r[destLo];
				sTemp += m * s;
				arm.r[destLo] = (u32)sTemp;
				arm.r[destHi] = (u32)(sTemp >> 32);
				Z_FLAG = (sTemp) ? false : true;
				N_FLAG = (sTemp < 0) ? true : false;
				if (((s32)rs) < 0)
					rs = ~rs;
				if ((rs & 0xFFFFFF00) == 0)
					icount -= 3;
				else if ((rs & 0xFFFF0000) == 0)
					icount -= 4;
				else if ((rs & 0xFF000000) == 0)
					icount -= 5;
				else
					icount -= 6;
			}
			break;
				LOGICAL_DATA_OPCODE(OP_TST, OP_TST, 0x110);
			case 0x100:
				// MRS Rd, CPSR
				// TODO: check if right instruction....
				armUpdateCPSR();
				arm.r[(opcode >> 12) & 0x0F] = arm.cpsr;
				break;
			case 0x109:
			{
				// SWP Rd, Rm, [Rn]
				u32 address = arm.r[(opcode >> 16) & 15];
				u32 temp = READ32(address);
				WRITE32(address, arm.r[opcode & 15]);
				arm.r[(opcode >> 12) & 15] = temp;
			}
			break;
				LOGICAL_DATA_OPCODE(OP_TEQ, OP_TEQ, 0x130);
			case 0x120:
			{
				// MSR CPSR_fields, Rm
				u32 value;
				u32 newValue;

				armUpdateCPSR();

				value = arm.r[opcode & 15];
				newValue = arm.cpsr;

				if (arm.mode > 0x10) {
					if (opcode & 0x00010000)
						newValue = (newValue & 0xFFFFFF00) | (value & 0x000000FF);
					if (opcode & 0x00020000)
						newValue = (newValue & 0xFFFF00FF) | (value & 0x0000FF00);
					if (opcode & 0x00040000)
						newValue = (newValue & 0xFF00FFFF) | (value & 0x00FF0000);
				}
				if (opcode & 0x00080000)
					newValue = (newValue & 0x00FFFFFF) | (value & 0xFF000000);
				newValue |= 0x10;
				armSwitchMode(newValue & 0x1f, false);
				arm.cpsr = newValue;
				armUpdateFlags();
			}
			break;

				ARITHMETIC_DATA_OPCODE(OP_CMP, OP_CMP, 0x150);
			case 0x140:
				// MRS Rd, SPSR
				// TODO: check if right instruction...
				arm.r[(opcode >> 12) & 0x0F] = arm.spsr;
				break;
			case 0x149:
			{
				// SWPB Rd, Rm, [Rn]
				u32 address = arm.r[(opcode >> 16) & 15];
				u32 temp = READ8(address);
				WRITE8(address, arm.r[opcode & 15]);
				arm.r[(opcode >> 12) & 15] = temp;
			}
			break;
				ARITHMETIC_DATA_OPCODE(OP_CMN, OP_CMN, 0x170);
			case 0x160:
			{
				// MSR SPSR_fields, Rm
				u32 value = arm.r[opcode & 15];
				if (arm.mode > 0x10 && arm.mode < 0x1f) {
					if (opcode & 0x00010000)
						arm.spsr = (arm.spsr & 0xFFFFFF00) | (value & 0x000000FF);
					if (opcode & 0x00020000)
						arm.spsr = (arm.spsr & 0xFFFF00FF) | (value & 0x0000FF00);
					if (opcode & 0x00040000)
						arm.spsr = (arm.spsr & 0xFF00FFFF) | (value & 0x00FF0000);
					if (opcode & 0x00080000)
						arm.spsr = (arm.spsr & 0x00FFFFFF) | (value & 0xFF000000);
				}
			}
			break;
				LOGICAL_DATA_OPCODE(OP_ORR, OP_ORR, 0x180);
				LOGICAL_DATA_OPCODE(OP_ORRS, OP_ORR, 0x190);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_MOV, OP_MOV, 0x1a0);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_MOVS, OP_MOV, 0x1b0);
				LOGICAL_DATA_OPCODE(OP_BIC, OP_BIC, 0x1c0);
				LOGICAL_DATA_OPCODE(OP_BICS, OP_BIC, 0x1d0);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_MVN, OP_MVN, 0x1e0);
				LOGICAL_DATA_OPCODE_WITHOUT_base(OP_MVNS, OP_MVN, 0x1f0);
			case 0x320:
			case 0x321:
			case 0x322:
			case 0x323:
			case 0x324:
			case 0x325:
			case 0x326:
			case 0x327:
			case 0x328:
			case 0x329:
			case 0x32a:
			case 0x32b:
			case 0x32c:
			case 0x32d:
			case 0x32e:
			case 0x32f:
			{
				// MSR CPSR_fields, #
				u32 newValue;
				u32 value;
				int shift;

				armUpdateCPSR();

				value = opcode & 0xFF;
				shift = (opcode & 0xF00) >> 7;

				if (shift) {
					ROR_IMM_MSR;
				}
				newValue = arm.cpsr;
				if (arm.mode > 0x10) {
					if (opcode & 0x00010000)
						newValue = (newValue & 0xFFFFFF00) | (value & 0x000000FF);
					if (opcode & 0x00020000)
						newValue = (newValue & 0xFFFF00FF) | (value & 0x0000FF00);
					if (opcode & 0x00040000)
						newValue = (newValue & 0xFF00FFFF) | (value & 0x00FF0000);
				}
				if (opcode & 0x00080000)
					newValue = (newValue & 0x00FFFFFF) | (value & 0xFF000000);

				newValue |= 0x10;

				armSwitchMode(newValue & 0x1f, false);
				arm.cpsr = newValue;
				armUpdateFlags();
			}
			break;
			case 0x360:
			case 0x361:
			case 0x362:
			case 0x363:
			case 0x364:
			case 0x365:
			case 0x366:
			case 0x367:
			case 0x368:
			case 0x369:
			case 0x36a:
			case 0x36b:
			case 0x36c:
			case 0x36d:
			case 0x36e:
			case 0x36f:
			{
				// MSR SPSR_fields, #
				if (arm.mode > 0x10 && arm.mode < 0x1f) {
					u32 value = opcode & 0xFF;
					int shift = (opcode & 0xF00) >> 7;
					if (shift) {
						ROR_IMM_MSR;
					}
					if (opcode & 0x00010000)
						arm.spsr = (arm.spsr & 0xFFFFFF00) | (value & 0x000000FF);
					if (opcode & 0x00020000)
						arm.spsr = (arm.spsr & 0xFFFF00FF) | (value & 0x0000FF00);
					if (opcode & 0x00040000)
						arm.spsr = (arm.spsr & 0xFF00FFFF) | (value & 0x00FF0000);
					if (opcode & 0x00080000)
						arm.spsr = (arm.spsr & 0x00FFFFFF) | (value & 0xFF000000);
				}
			}
			break;
				CASE_16(0x400)
				CASE_16(0x420) {
					// STR Rd, [Rn], -#
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					WRITE32(address, arm.r[dest]);
					arm.r[base] = address - offset;
					icount -= 2;
				}
				break;
				CASE_16(0x480)
				CASE_16(0x4a0) {
					// STR Rd, [Rn], #
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					WRITE32(address, arm.r[dest]);
					arm.r[base] = address + offset;
					icount -= 2;
				}
				break;
				CASE_16(0x500) {
					// STR Rd, [Rn, -#]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					WRITE32(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x520) {
					// STR Rd, [Rn, -#]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[base] = address;
					WRITE32(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x580) {
					// STR Rd, [Rn, #]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					WRITE32(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x5a0) {
					// STR Rd, [Rn, #]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[base] = address;
					WRITE32(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x410) {
					// LDR Rd, [Rn], -#
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] -= offset;
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x430) {
					// LDRT Rd, [Rn], -#
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] -= offset;
					icount -= 3;
				}
				break;
				CASE_16(0x490) {
					// LDR Rd, [Rn], #
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] += offset;
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x4b0) {
					// LDRT Rd, [Rn], #
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] += offset;
					icount -= 3;
				}
				break;
				CASE_16(0x510) {
					// LDR Rd, [Rn, -#]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[dest] = READ32(address);
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x530) {
					// LDR Rd, [Rn, -#]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] = address;
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x590) {
					// LDR Rd, [Rn, #]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[dest] = READ32(address);
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x5b0) {
					// LDR Rd, [Rn, #]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[dest] = READ32(address);
					if (dest != base)
						arm.r[base] = address;
					icount -= 3;
					if (dest == 15) {
						icount -= 2;
						arm.r[15] &= 0xFFFFFFFC;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x440)
				// T versions shouldn't be different on GBA
				CASE_16(0x460) {
					// STRB Rd, [Rn], -#
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					WRITE8(address, arm.r[dest]);
					arm.r[base] = address - offset;
					icount -= 2;
				}
				break;
				CASE_16(0x4c0)
				// T versions shouldn't be different on GBA
				CASE_16(0x4e0) {
					// STRB Rd, [Rn], #
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					WRITE8(address, arm.r[dest]);
					arm.r[base] = address + offset;
					icount -= 2;
				}
				break;
				CASE_16(0x540) {
					// STRB Rd, [Rn, -#]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					WRITE8(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x560) {
					// STRB Rd, [Rn, -#]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[base] = address;
					WRITE8(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x5c0) {
					// STRB Rd, [Rn, #]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					WRITE8(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x5e0) {
					// STRB Rd, [Rn, #]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[base] = address;
					WRITE8(address, arm.r[dest]);
					icount -= 2;
				}
				break;
				CASE_16(0x450)
				// T versions shouldn't be different
				CASE_16(0x470) {
					// LDRB Rd, [Rn], -#
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ8(address);
					if (dest != base)
						arm.r[base] -= offset;
					icount -= 3;
				}
				break;
				CASE_16(0x4d0)
				CASE_16(0x4f0) {// T versions should not be different
					// LDRB Rd, [Rn], #
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base];
					arm.r[dest] = READ8(address);
					if (dest != base)
						arm.r[base] += offset;
					icount -= 3;
				}
				break;
				CASE_16(0x550) {
					// LDRB Rd, [Rn, -#]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[dest] = READ8(address);
					icount -= 3;
				}
				break;
				CASE_16(0x570) {
					// LDRB Rd, [Rn, -#]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] - offset;
					arm.r[dest] = READ8(address);
					if (dest != base)
						arm.r[base] = address;
					icount -= 3;
				}
				break;
				CASE_16(0x5d0) {
					// LDRB Rd, [Rn, #]
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[dest] = READ8(address);
					icount -= 3;
				}
				break;
				CASE_16(0x5f0) {
					// LDRB Rd, [Rn, #]!
					int offset = opcode & 0xFFF;
					int dest = (opcode >> 12) & 15;
					int base = (opcode >> 16) & 15;
					u32 address = arm.r[base] + offset;
					arm.r[dest] = READ8(address);
					if (dest != base)
						arm.r[base] = address;
					icount -= 3;
				}
				break;
			case 0x600:
			case 0x608:
			// T versions are the same
			case 0x620:
			case 0x628:
			{
				// STR Rd, [Rn], -Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x602:
			case 0x60a:
			// T versions are the same
			case 0x622:
			case 0x62a:
			{
				// STR Rd, [Rn], -Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x604:
			case 0x60c:
			// T versions are the same
			case 0x624:
			case 0x62c:
			{
				// STR Rd, [Rn], -Rm, ASR #
				int offset;
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;

				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x606:
			case 0x60e:
			// T versions are the same
			case 0x626:
			case 0x62e:
			{
				// STR Rd, [Rn], -Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address - value;
				icount -= 2;
			}
			break;
			case 0x680:
			case 0x688:
			// T versions are the same
			case 0x6a0:
			case 0x6a8:
			{
				// STR Rd, [Rn], Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x682:
			case 0x68a:
			// T versions are the same
			case 0x6a2:
			case 0x6aa:
			{
				// STR Rd, [Rn], Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x684:
			case 0x68c:
			// T versions are the same
			case 0x6a4:
			case 0x6ac:
			{
				// STR Rd, [Rn], Rm, ASR #
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x686:
			case 0x68e:
			// T versions are the same
			case 0x6a6:
			case 0x6ae:
			{
				// STR Rd, [Rn], Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE32(address, arm.r[dest]);
				arm.r[base] = address + value;
				icount -= 2;
			}
			break;
			case 0x700:
			case 0x708:
			{
				// STR Rd, [Rn, -Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x702:
			case 0x70a:
			{
				// STR Rd, [Rn, -Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x704:
			case 0x70c:
			{
				// STR Rd, [Rn, -Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x706:
			case 0x70e:
			{
				// STR Rd, [Rn, -Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x720:
			case 0x728:
			{
				// STR Rd, [Rn, -Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x722:
			case 0x72a:
			{
				// STR Rd, [Rn, -Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x724:
			case 0x72c:
			{
				// STR Rd, [Rn, -Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x726:
			case 0x72e:
			{
				// STR Rd, [Rn, -Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x780:
			case 0x788:
			{
				// STR Rd, [Rn, Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x782:
			case 0x78a:
			{
				// STR Rd, [Rn, Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x784:
			case 0x78c:
			{
				// STR Rd, [Rn, Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x786:
			case 0x78e:
			{
				// STR Rd, [Rn, Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7a0:
			case 0x7a8:
			{
				// STR Rd, [Rn, Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7a2:
			case 0x7aa:
			{
				// STR Rd, [Rn, Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7a4:
			case 0x7ac:
			{
				// STR Rd, [Rn, Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7a6:
			case 0x7ae:
			{
				// STR Rd, [Rn, Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[base] = address;
				WRITE32(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x610:
			case 0x618:
			// T versions are the same
			case 0x630:
			case 0x638:
			{
				// LDR Rd, [Rn], -Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x612:
			case 0x61a:
			// T versions are the same
			case 0x632:
			case 0x63a:
			{
				// LDR Rd, [Rn], -Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x614:
			case 0x61c:
			// T versions are the same
			case 0x634:
			case 0x63c:
			{
				// LDR Rd, [Rn], -Rm, ASR #
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x616:
			case 0x61e:
			// T versions are the same
			case 0x636:
			case 0x63e:
			{
				// LDR Rd, [Rn], -Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address - value;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x690:
			case 0x698:
			// T versions are the same
			case 0x6b0:
			case 0x6b8:
			{
				// LDR Rd, [Rn], Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x692:
			case 0x69a:
			// T versions are the same
			case 0x6b2:
			case 0x6ba:
			{
				// LDR Rd, [Rn], Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x694:
			case 0x69c:
			// T versions are the same
			case 0x6b4:
			case 0x6bc:
			{
				// LDR Rd, [Rn], Rm, ASR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x696:
			case 0x69e:
			// T versions are the same
			case 0x6b6:
			case 0x6be:
			{
				// LDR Rd, [Rn], Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address + value;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x710:
			case 0x718:
			{
				// LDR Rd, [Rn, -Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x712:
			case 0x71a:
			{
				// LDR Rd, [Rn, -Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x714:
			case 0x71c:
			{
				// LDR Rd, [Rn, -Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x716:
			case 0x71e:
			{
				// LDR Rd, [Rn, -Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x730:
			case 0x738:
			{
				// LDR Rd, [Rn, -Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x732:
			case 0x73a:
			{
				// LDR Rd, [Rn, -Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x734:
			case 0x73c:
			{
				// LDR Rd, [Rn, -Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x736:
			case 0x73e:
			{
				// LDR Rd, [Rn, -Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x790:
			case 0x798:
			{
				// LDR Rd, [Rn, Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x792:
			case 0x79a:
			{
				// LDR Rd, [Rn, Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x794:
			case 0x79c:
			{
				// LDR Rd, [Rn, Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x796:
			case 0x79e:
			{
				// LDR Rd, [Rn, Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[dest] = READ32(address);
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x7b0:
			case 0x7b8:
			{
				// LDR Rd, [Rn, Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x7b2:
			case 0x7ba:
			{
				// LDR Rd, [Rn, Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x7b4:
			case 0x7bc:
			{
				// LDR Rd, [Rn, Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x7b6:
			case 0x7be:
			{
				// LDR Rd, [Rn, Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[dest] = READ32(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
				if (dest == 15) {
					icount -= 2;
					arm.r[15] &= 0xFFFFFFFC;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
				}
			}
			break;
			case 0x640:
			case 0x648:
			// T versions are the same
			case 0x660:
			case 0x668:
			{
				// STRB Rd, [Rn], -Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x642:
			case 0x64a:
			// T versions are the same
			case 0x662:
			case 0x66a:
			{
				// STRB Rd, [Rn], -Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x644:
			case 0x64c:
			// T versions are the same
			case 0x664:
			case 0x66c:
			{
				// STRB Rd, [Rn], -Rm, ASR #
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address - offset;
				icount -= 2;
			}
			break;
			case 0x646:
			case 0x64e:
			// T versions are the same
			case 0x666:
			case 0x66e:
			{
				// STRB Rd, [Rn], -Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address - value;
				icount -= 2;
			}
			break;
			case 0x6c0:
			case 0x6c8:
			// T versions are the same
			case 0x6e0:
			case 0x6e8:
			{
				// STRB Rd, [Rn], Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x6c2:
			case 0x6ca:
			// T versions are the same
			case 0x6e2:
			case 0x6ea:
			{
				// STRB Rd, [Rn], Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x6c4:
			case 0x6cc:
			// T versions are the same
			case 0x6e4:
			case 0x6ec:
			{
				// STR Rd, [Rn], Rm, ASR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address + offset;
				icount -= 2;
			}
			break;
			case 0x6c6:
			case 0x6ce:
			// T versions are the same
			case 0x6e6:
			case 0x6ee:
			{
				// STRB Rd, [Rn], Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				WRITE8(address, arm.r[dest]);
				arm.r[base] = address + value;
				icount -= 2;
			}
			break;
			case 0x740:
			case 0x748:
			{
				// STRB Rd, [Rn, -Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x742:
			case 0x74a:
			{
				// STRB Rd, [Rn, -Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x744:
			case 0x74c:
			{
				// STRB Rd, [Rn, -Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x746:
			case 0x74e:
			{
				// STRB Rd, [Rn, -Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x760:
			case 0x768:
			{
				// STRB Rd, [Rn, -Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x762:
			case 0x76a:
			{
				// STRB Rd, [Rn, -Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x764:
			case 0x76c:
			{
				// STRB Rd, [Rn, -Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x766:
			case 0x76e:
			{
				// STRB Rd, [Rn, -Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7c0:
			case 0x7c8:
			{
				// STRB Rd, [Rn, Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7c2:
			case 0x7ca:
			{
				// STRB Rd, [Rn, Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7c4:
			case 0x7cc:
			{
				// STRB Rd, [Rn, Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7c6:
			case 0x7ce:
			{
				// STRB Rd, [Rn, Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7e0:
			case 0x7e8:
			{
				// STRB Rd, [Rn, Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7e2:
			case 0x7ea:
			{
				// STRB Rd, [Rn, Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7e4:
			case 0x7ec:
			{
				// STRB Rd, [Rn, Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x7e6:
			case 0x7ee:
			{
				// STRB Rd, [Rn, Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[base] = address;
				WRITE8(address, arm.r[dest]);
				icount -= 2;
			}
			break;
			case 0x650:
			case 0x658:
			// T versions are the same
			case 0x670:
			case 0x678:
			{
				// LDRB Rd, [Rn], -Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
			}
			break;
			case 0x652:
			case 0x65a:
			// T versions are the same
			case 0x672:
			case 0x67a:
			{
				// LDRB Rd, [Rn], -Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
			}
			break;
			case 0x654:
			case 0x65c:
			// T versions are the same
			case 0x674:
			case 0x67c:
			{
				// LDRB Rd, [Rn], -Rm, ASR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address - offset;
				icount -= 3;
			}
			break;
			case 0x656:
			case 0x65e:
			case 0x676:
			case 0x67e:
			{
				// LDRB Rd, [Rn], -Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address - value;
				icount -= 3;
			}
			break;
			case 0x6d0:
			case 0x6d8:
			// T versions are the same
			case 0x6f0:
			case 0x6f8:
			{
				// LDRB Rd, [Rn], Rm, LSL #
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
			}
			break;
			case 0x6d2:
			case 0x6da:
			// T versions are the same
			case 0x6f2:
			case 0x6fa:
			{
				// LDRB Rd, [Rn], Rm, LSR #
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
			}
			break;
			case 0x6d4:
			case 0x6dc:
			// T versions are the same
			case 0x6f4:
			case 0x6fc:
			{
				// LDRB Rd, [Rn], Rm, ASR #
				int dest;
				int base;
				u32 address;
				int offset;
				int shift = (opcode >> 7) & 31;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address + offset;
				icount -= 3;
			}
			break;
			case 0x6d6:
			case 0x6de:
			// T versions are the same
			case 0x6f6:
			case 0x6fe:
			{
				// LDRB Rd, [Rn], Rm, ROR #
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base];
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address + value;
				icount -= 3;
			}
			break;
			case 0x750:
			case 0x758:
			{
				// LDRB Rd, [Rn, -Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x752:
			case 0x75a:
			{
				// LDRB Rd, [Rn, -Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x754:
			case 0x75c:
			{
				// LDRB Rd, [Rn, -Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x756:
			case 0x75e:
			{
				// LDRB Rd, [Rn, -Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x770:
			case 0x778:
			{
				// LDRB Rd, [Rn, -Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x772:
			case 0x77a:
			{
				// LDRB Rd, [Rn, -Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x774:
			case 0x77c:
			{
				// LDRB Rd, [Rn, -Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x776:
			case 0x77e:
			{
				// LDRB Rd, [Rn, -Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] - value;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x7d0:
			case 0x7d8:
			{
				// LDRB Rd, [Rn, Rm, LSL #]
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x7d2:
			case 0x7da:
			{
				// LDRB Rd, [Rn, Rm, LSR #]
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x7d4:
			case 0x7dc:
			{
				// LDRB Rd, [Rn, Rm, ASR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x7d6:
			case 0x7de:
			{
				// LDRB Rd, [Rn, Rm, ROR #]
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[dest] = READ8(address);
				icount -= 3;
			}
			break;
			case 0x7f0:
			case 0x7f8:
			{
				// LDRB Rd, [Rn, Rm, LSL #]!
				int offset = arm.r[opcode & 15] << ((opcode >> 7) & 31);
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x7f2:
			case 0x7fa:
			{
				// LDRB Rd, [Rn, Rm, LSR #]!
				int shift = (opcode >> 7) & 31;
				int offset = shift ? arm.r[opcode & 15] >> shift : 0;
				int dest = (opcode >> 12) & 15;
				int base = (opcode >> 16) & 15;
				u32 address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x7f4:
			case 0x7fc:
			{
				// LDRB Rd, [Rn, Rm, ASR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				int offset;
				if (shift)
					offset = (int)((s32)arm.r[opcode & 15] >> shift);
				else if (arm.r[opcode & 15] & 0x80000000)
					offset = 0xFFFFFFFF;
				else
					offset = 0;
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + offset;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			case 0x7f6:
			case 0x7fe:
			{
				// LDRB Rd, [Rn, Rm, ROR #]!
				int dest;
				int base;
				u32 address;
				int shift = (opcode >> 7) & 31;
				u32 value = arm.r[opcode & 15];
				if (shift) {
					ROR_VALUE;
				} else {
					RCR_VALUE;
				}
				dest = (opcode >> 12) & 15;
				base = (opcode >> 16) & 15;
				address = arm.r[base] + value;
				arm.r[dest] = READ8(address);
				if (dest != base)
					arm.r[base] = address;
				icount -= 3;
			}
			break;
			#define STMW(val, reg) \
	if (opcode & (val)) { \
		WRITE32(address, (*(u32*)&reg)); \
		if (!offset) { \
			arm.r[base] = temp;	\
			icount -= 1; \
			offset = 1;	\
		} \
		else { \
			icount -= 1; \
		} \
		address += 4; \
	}
			#define STM(val, reg) \
	if (opcode & (val)) { \
		WRITE32(address, (*(u32*)&reg)); \
		if (!offset) { \
			icount -= 1; \
			offset = 1;	\
		} else { \
			icount -= 1; \
		} \
		address += 4; \
	}

				CASE_16(0x800) {
					// STMDA Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					STM(256, arm.r[8]);
					STM(512, arm.r[9]);
					STM(1024, arm.r[10]);
					STM(2048, arm.r[11]);
					STM(4096, arm.r[12]);
					STM(8192, arm.r[13]);
					STM(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x820) {
					// STMDA Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					STMW(256, arm.r[8]);
					STMW(512, arm.r[9]);
					STMW(1024, arm.r[10]);
					STMW(2048, arm.r[11]);
					STMW(4096, arm.r[12]);
					STMW(8192, arm.r[13]);
					STMW(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
						arm.r[base] = temp;
					}
				}
				break;
				CASE_16(0x840) {
					// STMDA Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);

					if (arm.mode == 0x11) {
						STM(256, arm.r8fiq);
						STM(512, arm.r9fiq);
						STM(1024, arm.r10fiq);
						STM(2048, arm.r11fiq);
						STM(4096, arm.r12fiq);
					} else {
						STM(256, arm.r[8]);
						STM(512, arm.r[9]);
						STM(1024, arm.r[10]);
						STM(2048, arm.r[11]);
						STM(4096, arm.r[12]);
					}

					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STM(8192, arm.r13usr);
						STM(16384, arm.r14usr);
					} else {
						STM(8192, arm.r[13]);
						STM(16384, arm.r[14]);
					}

					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x860) {
					// STMDA Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);

					if (arm.mode == 0x11) {
						STMW(256, arm.r8fiq);
						STMW(512, arm.r9fiq);
						STMW(1024, arm.r10fiq);
						STMW(2048, arm.r11fiq);
						STMW(4096, arm.r12fiq);
					} else {
						STMW(256, arm.r[8]);
						STMW(512, arm.r[9]);
						STMW(1024, arm.r[10]);
						STMW(2048, arm.r[11]);
						STMW(4096, arm.r[12]);
					}

					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STMW(8192, arm.r13usr);
						STMW(16384, arm.r14usr);
					} else {
						STMW(8192, arm.r[13]);
						STMW(16384, arm.r[14]);
					}

					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
						arm.r[base] = temp;
					}
				}
				break;

				CASE_16(0x880) {
					// STMIA Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					STM(256, arm.r[8]);
					STM(512, arm.r[9]);
					STM(1024, arm.r[10]);
					STM(2048, arm.r[11]);
					STM(4096, arm.r[12]);
					STM(8192, arm.r[13]);
					STM(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x8a0) {
					// STMIA Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					u32 temp = arm.r[base] + 4 * (cpuBitsSet[opcode & 0xFF] +
												  cpuBitsSet[(opcode >> 8) & 255]);
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					STMW(256, arm.r[8]);
					STMW(512, arm.r[9]);
					STMW(1024, arm.r[10]);
					STMW(2048, arm.r[11]);
					STMW(4096, arm.r[12]);
					STMW(8192, arm.r[13]);
					STMW(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset) {
							arm.r[base] = temp;
							icount -= 1;
						} else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x8c0) {
					// STMIA Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					if (arm.mode == 0x11) {
						STM(256, arm.r8fiq);
						STM(512, arm.r9fiq);
						STM(1024, arm.r10fiq);
						STM(2048, arm.r11fiq);
						STM(4096, arm.r12fiq);
					} else {
						STM(256, arm.r[8]);
						STM(512, arm.r[9]);
						STM(1024, arm.r[10]);
						STM(2048, arm.r[11]);
						STM(4096, arm.r[12]);
					}
					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STM(8192, arm.r13usr);
						STM(16384, arm.r14usr);
					} else {
						STM(8192, arm.r[13]);
						STM(16384, arm.r[14]);
					}
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x8e0) {
					// STMIA Rn!, {Rlist}^
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					int offset = 0;
					u32 temp = arm.r[base] + 4 * (cpuBitsSet[opcode & 0xFF] +
												  cpuBitsSet[(opcode >> 8) & 255]);
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					if (arm.mode == 0x11) {
						STMW(256, arm.r8fiq);
						STMW(512, arm.r9fiq);
						STMW(1024, arm.r10fiq);
						STMW(2048, arm.r11fiq);
						STMW(4096, arm.r12fiq);
					} else {
						STMW(256, arm.r[8]);
						STMW(512, arm.r[9]);
						STMW(1024, arm.r[10]);
						STMW(2048, arm.r[11]);
						STMW(4096, arm.r[12]);
					}
					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STMW(8192, arm.r13usr);
						STMW(16384, arm.r14usr);
					} else {
						STMW(8192, arm.r[13]);
						STMW(16384, arm.r[14]);
					}
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset) {
							arm.r[base] = temp;
							icount -= 1;
						} else
							icount -= 1;
					}
				}
				break;

				CASE_16(0x900) {
					// STMDB Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					STM(256, arm.r[8]);
					STM(512, arm.r[9]);
					STM(1024, arm.r[10]);
					STM(2048, arm.r[11]);
					STM(4096, arm.r[12]);
					STM(8192, arm.r[13]);
					STM(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x920) {
					// STMDB Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					STMW(256, arm.r[8]);
					STMW(512, arm.r[9]);
					STMW(1024, arm.r[10]);
					STMW(2048, arm.r[11]);
					STMW(4096, arm.r[12]);
					STMW(8192, arm.r[13]);
					STMW(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
						arm.r[base] = temp;
					}
				}
				break;
				CASE_16(0x940) {
					// STMDB Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);

					if (arm.mode == 0x11) {
						STM(256, arm.r8fiq);
						STM(512, arm.r9fiq);
						STM(1024, arm.r10fiq);
						STM(2048, arm.r11fiq);
						STM(4096, arm.r12fiq);
					} else {
						STM(256, arm.r[8]);
						STM(512, arm.r[9]);
						STM(1024, arm.r[10]);
						STM(2048, arm.r[11]);
						STM(4096, arm.r[12]);
					}

					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STM(8192, arm.r13usr);
						STM(16384, arm.r14usr);
					} else {
						STM(8192, arm.r[13]);
						STM(16384, arm.r[14]);
					}

					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x960) {
					// STMDB Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);

					if (arm.mode == 0x11) {
						STMW(256, arm.r8fiq);
						STMW(512, arm.r9fiq);
						STMW(1024, arm.r10fiq);
						STMW(2048, arm.r11fiq);
						STMW(4096, arm.r12fiq);
					} else {
						STMW(256, arm.r[8]);
						STMW(512, arm.r[9]);
						STMW(1024, arm.r[10]);
						STMW(2048, arm.r[11]);
						STMW(4096, arm.r[12]);
					}

					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STMW(8192, arm.r13usr);
						STMW(16384, arm.r14usr);
					} else {
						STMW(8192, arm.r[13]);
						STMW(16384, arm.r[14]);
					}

					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
						arm.r[base] = temp;
					}
				}
				break;

				CASE_16(0x980) {
					// STMIB Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					STM(256, arm.r[8]);
					STM(512, arm.r[9]);
					STM(1024, arm.r[10]);
					STM(2048, arm.r[11]);
					STM(4096, arm.r[12]);
					STM(8192, arm.r[13]);
					STM(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x9a0) {
					// STMIB Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					u32 temp = arm.r[base] + 4 * (cpuBitsSet[opcode & 0xFF] +
												  cpuBitsSet[(opcode >> 8) & 255]);
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					STMW(256, arm.r[8]);
					STMW(512, arm.r[9]);
					STMW(1024, arm.r[10]);
					STMW(2048, arm.r[11]);
					STMW(4096, arm.r[12]);
					STMW(8192, arm.r[13]);
					STMW(16384, arm.r[14]);
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset) {
							arm.r[base] = temp;
							icount -= 1;
						} else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x9c0) {
					// STMIB Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					STM(1, arm.r[0]);
					STM(2, arm.r[1]);
					STM(4, arm.r[2]);
					STM(8, arm.r[3]);
					STM(16, arm.r[4]);
					STM(32, arm.r[5]);
					STM(64, arm.r[6]);
					STM(128, arm.r[7]);
					if (arm.mode == 0x11) {
						STM(256, arm.r8fiq);
						STM(512, arm.r9fiq);
						STM(1024, arm.r10fiq);
						STM(2048, arm.r11fiq);
						STM(4096, arm.r12fiq);
					} else {
						STM(256, arm.r[8]);
						STM(512, arm.r[9]);
						STM(1024, arm.r[10]);
						STM(2048, arm.r[11]);
						STM(4096, arm.r[12]);
					}
					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STM(8192, arm.r13usr);
						STM(16384, arm.r14usr);
					} else {
						STM(8192, arm.r[13]);
						STM(16384, arm.r[14]);
					}
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset)
							icount -= 1;
						else
							icount -= 1;
					}
				}
				break;
				CASE_16(0x9e0) {
					// STMIB Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					u32 temp = arm.r[base] + 4 * (cpuBitsSet[opcode & 0xFF] +
												  cpuBitsSet[(opcode >> 8) & 255]);
					icount -= 2;

					STMW(1, arm.r[0]);
					STMW(2, arm.r[1]);
					STMW(4, arm.r[2]);
					STMW(8, arm.r[3]);
					STMW(16, arm.r[4]);
					STMW(32, arm.r[5]);
					STMW(64, arm.r[6]);
					STMW(128, arm.r[7]);
					if (arm.mode == 0x11) {
						STMW(256, arm.r8fiq);
						STMW(512, arm.r9fiq);
						STMW(1024, arm.r10fiq);
						STMW(2048, arm.r11fiq);
						STMW(4096, arm.r12fiq);
					} else {
						STMW(256, arm.r[8]);
						STMW(512, arm.r[9]);
						STMW(1024, arm.r[10]);
						STMW(2048, arm.r[11]);
						STMW(4096, arm.r[12]);
					}
					if (arm.mode != 0x10 && arm.mode != 0x1f) {
						STMW(8192, arm.r13usr);
						STMW(16384, arm.r14usr);
					} else {
						STMW(8192, arm.r[13]);
						STMW(16384, arm.r[14]);
					}
					if (opcode & 32768) {
						WRITE32(address, arm.r[15] + 4);
						if (!offset) {
							arm.r[base] = temp;
							icount -= 1;
						} else
							icount -= 1;
					}
				}
				break;

			#define LDM(val, reg) \
	if (opcode & (val)) { \
		*(u32*)&reg = READ32(address); \
		if (offset)	\
			icount -= 1; \
		else { \
			icount -= 1; \
			offset = 1;	\
		} \
		address += 4; \
	}

				CASE_16(0x810) {
					// LDMDA Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x830) {
					// LDMDA Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
					if (!(opcode & (1 << base)))
						arm.r[base] = temp;
				}
				break;
				CASE_16(0x850) {
					// LDMDA Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}
					}
				}
				break;
				CASE_16(0x870) {
					// LDMDA Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (temp + 4) & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r13usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;
					}
				}
				break;

				CASE_16(0x890) {
					// LDMIA Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x8b0) {
					// LDMIA Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] +
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = arm.r[base] & 0xFFFFFFFC;

					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
					if (!(opcode & (1 << base)))
						arm.r[base] = temp;
				}
				break;
				CASE_16(0x8d0) {
					// LDMIA Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = arm.r[base] & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}
					}
				}
				break;
				CASE_16(0x8f0) {
					// LDMIA Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] +
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = arm.r[base] & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;
					}
				}
				break;

				CASE_16(0x910) {
					// LDMDB Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x930) {
					// LDMDB Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
					if (!(opcode & (1 << base)))
						arm.r[base] = temp;
				}
				break;
				CASE_16(0x950) {
					// LDMDB Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}
					}
				}
				break;
				CASE_16(0x970) {
					// LDMDB Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] -
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = temp & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;
					}
				}
				break;

				CASE_16(0x990) {
					// LDMIB Rn, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
				}
				break;
				CASE_16(0x9b0) {
					// LDMIB Rn!, {Rlist}
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] +
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					LDM(1, arm.r[0]);
					LDM(2, arm.r[1]);
					LDM(4, arm.r[2]);
					LDM(8, arm.r[3]);
					LDM(16, arm.r[4]);
					LDM(32, arm.r[5]);
					LDM(64, arm.r[6]);
					LDM(128, arm.r[7]);
					LDM(256, arm.r[8]);
					LDM(512, arm.r[9]);
					LDM(1024, arm.r[10]);
					LDM(2048, arm.r[11]);
					LDM(4096, arm.r[12]);
					LDM(8192, arm.r[13]);
					LDM(16384, arm.r[14]);
					if (opcode & 32768) {
						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;
						arm.pc = arm.r[15];
						arm.r[15] += 4;
					}
					if (!(opcode & (1 << base)))
						arm.r[base] = temp;
				}
				break;
				CASE_16(0x9d0) {
					// LDMIB Rn, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}
					}
				}
				break;
				CASE_16(0x9f0) {
					// LDMIB Rn!, {Rlist}^
					int offset = 0;
					int base = (opcode & 0x000F0000) >> 16;
					u32 temp = arm.r[base] +
							   4 * (cpuBitsSet[opcode & 255] + cpuBitsSet[(opcode >> 8) & 255]);
					u32 address = (arm.r[base] + 4) & 0xFFFFFFFC;
					icount -= 2;

					if (opcode & 0x8000) {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);
						LDM(256, arm.r[8]);
						LDM(512, arm.r[9]);
						LDM(1024, arm.r[10]);
						LDM(2048, arm.r[11]);
						LDM(4096, arm.r[12]);
						LDM(8192, arm.r[13]);
						LDM(16384, arm.r[14]);

						arm.r[15] = READ32(address);
						if (!offset)
							icount -= 2;
						else
							icount -= 2;

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;

						armSwitchMode(arm.spsr & 0x1f, false);

						arm.pc = arm.r[15] & 0xFFFFFFFC;
						arm.r[15] = arm.pc + 4;
					} else {
						LDM(1, arm.r[0]);
						LDM(2, arm.r[1]);
						LDM(4, arm.r[2]);
						LDM(8, arm.r[3]);
						LDM(16, arm.r[4]);
						LDM(32, arm.r[5]);
						LDM(64, arm.r[6]);
						LDM(128, arm.r[7]);

						if (arm.mode == 0x11) {
							LDM(256, arm.r8fiq);
							LDM(512, arm.r9fiq);
							LDM(1024, arm.r10fiq);
							LDM(2048, arm.r11fiq);
							LDM(4096, arm.r12fiq);
						} else {
							LDM(256, arm.r[8]);
							LDM(512, arm.r[9]);
							LDM(1024, arm.r[10]);
							LDM(2048, arm.r[11]);
							LDM(4096, arm.r[12]);
						}

						if (arm.mode != 0x10 && arm.mode != 0x1f) {
							LDM(8192, arm.r13usr);
							LDM(16384, arm.r14usr);
						} else {
							LDM(8192, arm.r[13]);
							LDM(16384, arm.r[14]);
						}

						if (!(opcode & (1 << base)))
							arm.r[base] = temp;
					}
				}
				break;
				CASE_256(0xa00) {
					// B <offset>
					int offset = opcode & 0x00FFFFFF;
					if (offset & 0x00800000) {
						offset |= 0xFF000000;
					}
					offset <<= 2;
					arm.r[15] += offset;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
					icount -= 3;
				}
				break;
				CASE_256(0xb00) {
					// BL <offset>
					int offset = opcode & 0x00FFFFFF;
					if (offset & 0x00800000) {
						offset |= 0xFF000000;
					}
					offset <<= 2;
					arm.r[14] = arm.r[15] - 4;
					arm.r[15] += offset;
					arm.pc = arm.r[15];
					arm.r[15] += 4;
					icount -= 3;
				}
				break;
				CASE_256(0xf00)
				break;
			default:
				break;
			}
		}
	} while (icount > 0);
}
