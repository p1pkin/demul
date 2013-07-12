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

#ifndef __ARM_H__
#define __ARM_H__

#define Z_FLAG arm.zero
#define C_FLAG arm.carry
#define N_FLAG arm.negative
#define V_FLAG arm.overflow

#define CASE_16(BASE) \
case BASE: \
case BASE + 1: \
case BASE + 2: \
case BASE + 3: \
case BASE + 4: \
case BASE + 5: \
case BASE + 6: \
case BASE + 7: \
case BASE + 8: \
case BASE + 9: \
case BASE + 10:	\
case BASE + 11:	\
case BASE + 12:	\
case BASE + 13:	\
case BASE + 14:	\
case BASE + 15:

#define CASE_256(BASE) \
	CASE_16(BASE) \
	CASE_16(BASE + 0x10) \
	CASE_16(BASE + 0x20) \
	CASE_16(BASE + 0x30) \
	CASE_16(BASE + 0x40) \
	CASE_16(BASE + 0x50) \
	CASE_16(BASE + 0x60) \
	CASE_16(BASE + 0x70) \
	CASE_16(BASE + 0x80) \
	CASE_16(BASE + 0x90) \
	CASE_16(BASE + 0xa0) \
	CASE_16(BASE + 0xb0) \
	CASE_16(BASE + 0xc0) \
	CASE_16(BASE + 0xd0) \
	CASE_16(BASE + 0xe0) \
	CASE_16(BASE + 0xf0)

#define OP_AND \
	arm.r[dest] = arm.r[(opcode >> 16) & 15] & value;

#define OP_ANDS	\
	arm.r[dest] = arm.r[(opcode >> 16) & 15] & value; \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define OP_EOR \
	arm.r[dest] = arm.r[(opcode >> 16) & 15] ^ value;

#define OP_EORS	\
	arm.r[dest] = arm.r[(opcode >> 16) & 15] ^ value; \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define NEG(i) ((i) >> 31)
#define POS(i) ((~(i)) >> 31)
#define ADDCARRY(a, b, c) \
	C_FLAG = ((NEG(a) & NEG(b)) | \
			  (NEG(a) & POS(c)) | \
			  (NEG(b) & POS(c))) ? true : false;
#define ADDOVERFLOW(a, b, c) \
	V_FLAG = ((NEG(a) & NEG(b) & POS(c)) | \
			  (POS(a) & POS(b) & NEG(c))) ? true : false;
#define SUBCARRY(a, b, c) \
	C_FLAG = ((NEG(a) & POS(b)) | \
			  (NEG(a) & POS(c)) | \
			  (POS(b) & POS(c))) ? true : false;
#define SUBOVERFLOW(a, b, c) \
	V_FLAG = ((NEG(a) & POS(b) & POS(c)) | \
			  (POS(a) & NEG(b) & NEG(c))) ? true : false;
#define OP_SUB \
	{ \
		arm.r[dest] = arm.r[base] - value; \
	}
#define OP_SUBS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs - rhs; \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		SUBCARRY(lhs, rhs, res); \
		SUBOVERFLOW(lhs, rhs, res);	\
	}
#define OP_RSB \
	{ \
		arm.r[dest] = value - arm.r[base]; \
	}
#define OP_RSBS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = rhs - lhs; \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		SUBCARRY(rhs, lhs, res); \
		SUBOVERFLOW(rhs, lhs, res);	\
	}
#define OP_ADD \
	{ \
		arm.r[dest] = arm.r[base] + value; \
	}
#define OP_ADDS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs + rhs; \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		ADDCARRY(lhs, rhs, res); \
		ADDOVERFLOW(lhs, rhs, res);	\
	}
#define OP_ADC \
	{ \
		arm.r[dest] = arm.r[base] + value + (u32)C_FLAG; \
	}
#define OP_ADCS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs + rhs + (u32)C_FLAG; \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		ADDCARRY(lhs, rhs, res); \
		ADDOVERFLOW(lhs, rhs, res);	\
	}
#define OP_SBC \
	{ \
		arm.r[dest] = arm.r[base] - value - !((u32)C_FLAG);	\
	}
#define OP_SBCS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs - rhs - !((u32)C_FLAG); \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		SUBCARRY(lhs, rhs, res); \
		SUBOVERFLOW(lhs, rhs, res);	\
	}
#define OP_RSC \
	{ \
		arm.r[dest] = value - arm.r[base] - !((u32)C_FLAG);	\
	}
#define OP_RSCS	\
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = rhs - lhs - !((u32)C_FLAG); \
		arm.r[dest] = res; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		SUBCARRY(rhs, lhs, res); \
		SUBOVERFLOW(rhs, lhs, res);	\
	}
#define OP_CMP \
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs - rhs; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		SUBCARRY(lhs, rhs, res); \
		SUBOVERFLOW(lhs, rhs, res);	\
	}
#define OP_CMN \
	{ \
		u32 lhs = arm.r[base]; \
		u32 rhs = value; \
		u32 res = lhs + rhs; \
		Z_FLAG = (res == 0) ? true : false;	\
		N_FLAG = NEG(res) ? true : false; \
		ADDCARRY(lhs, rhs, res); \
		ADDOVERFLOW(lhs, rhs, res);	\
	}

#define OP_TST \
	u32 res = arm.r[base] & value; \
	N_FLAG = (res & 0x80000000) ? true : false;	\
	Z_FLAG = (res) ? false : true; \
	C_FLAG = C_OUT;

#define OP_TEQ \
	u32 res = arm.r[base] ^ value; \
	N_FLAG = (res & 0x80000000) ? true : false;	\
	Z_FLAG = (res) ? false : true; \
	C_FLAG = C_OUT;

#define OP_ORR \
	arm.r[dest] = arm.r[base] | value;

#define OP_ORRS	\
	arm.r[dest] = arm.r[base] | value; \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define OP_MOV \
	arm.r[dest] = value;

#define OP_MOVS	\
	arm.r[dest] = value; \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define OP_BIC \
	arm.r[dest] = arm.r[base] & (~value);

#define OP_BICS	\
	arm.r[dest] = arm.r[base] & (~value); \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define OP_MVN \
	arm.r[dest] = ~value;

#define OP_MVNS	\
	arm.r[dest] = ~value; \
	N_FLAG = (arm.r[dest] & 0x80000000) ? true : false;	\
	Z_FLAG = (arm.r[dest]) ? false : true; \
	C_FLAG = C_OUT;

#define LOGICAL_LSL_REG	\
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		C_OUT = (v >> (32 - shift)) & 1 ? true : false;	\
		value = v << shift;	\
	}
#define LOGICAL_LSR_REG	\
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		C_OUT = (v >> (shift - 1)) & 1 ? true : false; \
		value = v >> shift;	\
	}
#define LOGICAL_ASR_REG	\
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		C_OUT = ((s32)v >> (int)(shift - 1)) & 1 ? true : false; \
		value = (s32)v >> (int)shift; \
	}
#define LOGICAL_ROR_REG	\
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		C_OUT = (v >> (shift - 1)) & 1 ? true : false; \
		value = ((v << (32 - shift)) | \
				 (v >> shift));	\
	}
#define LOGICAL_RRX_REG	\
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		shift = (int)C_FLAG; \
		C_OUT = (v & 1) ? true : false;	\
		value = ((v >> 1) |	\
				 (shift << 31)); \
	}
#define LOGICAL_ROR_IMM	\
	{ \
		u32 v = opcode & 0xff; \
		C_OUT = (v >> (shift - 1)) & 1 ? true : false; \
		value = ((v << (32 - shift)) | \
				 (v >> shift));	\
	}
#define ARITHMETIC_LSL_REG \
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		value = v << shift;	\
	}
#define ARITHMETIC_LSR_REG \
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		value = v >> shift;	\
	}
#define ARITHMETIC_ASR_REG \
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		value = (s32)v >> (int)shift; \
	}
#define ARITHMETIC_ROR_REG \
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		value = ((v << (32 - shift)) | \
				 (v >> shift));	\
	}
#define ARITHMETIC_RRX_REG \
	{ \
		u32 v = arm.r[opcode & 0x0f]; \
		shift = (int)C_FLAG; \
		value = ((v >> 1) |	\
				 (shift << 31)); \
	}
#define ARITHMETIC_ROR_IMM \
	{ \
		u32 v = opcode & 0xff; \
		value = ((v << (32 - shift)) | \
				 (v >> shift));	\
	}
#define ROR_IMM_MSR	\
	{ \
		u32 v = opcode & 0xff; \
		value = ((v << (32 - shift)) | \
				 (v >> shift));	\
	}
#define ROR_VALUE \
	{ \
		value = ((value << (32 - shift)) | \
				 (value >> shift));	\
	}
#define RCR_VALUE \
	{ \
		shift = (int)C_FLAG; \
		value = ((value >> 1) |	\
				 (shift << 31)); \
	}

#define LOGICAL_DATA_OPCODE(OPCODE, OPCODE2, BASE) \
case BASE: \
case BASE + 8: \
{ \
	/* OP Rd,Rb,Rm LSL # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	  \
	if (shift) { \
		LOGICAL_LSL_REG	\
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 2: \
case BASE + 10:	\
{ \
	/* OP Rd,Rb,Rm LSR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_LSR_REG	\
	} else { \
		value = 0; \
		C_OUT = (arm.r[opcode & 0x0F] & 0x80000000) ? true : false;	\
	} \
	  \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 4: \
case BASE + 12:	\
{ \
	/* OP Rd,Rb,Rm ASR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ASR_REG	\
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
			C_OUT = true; \
		} else { \
			value = 0; \
			C_OUT = false; \
		}					\
	} \
	  \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 6: \
case BASE + 14:	\
{ \
	/* OP Rd,Rb,Rm ROR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ROR_REG	\
	} else { \
		LOGICAL_RRX_REG	\
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 1: \
{ \
	/* OP Rd,Rb,Rm LSL Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
			C_OUT = (arm.r[opcode & 0x0F] & 1 ? true : false); \
		} else if (shift < 32) { \
			LOGICAL_LSL_REG	\
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 3: \
{ \
	/* OP Rd,Rb,Rm LSR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
			C_OUT = (arm.r[opcode & 0x0F] & 0x80000000 ? true : false);	\
		} else if (shift < 32) { \
			LOGICAL_LSR_REG	\
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 5: \
{ \
	/* OP Rd,Rb,Rm ASR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift < 32) { \
		if (shift) { \
			LOGICAL_ASR_REG	\
		} else { \
			value = arm.r[opcode & 0x0F]; \
		} \
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
			C_OUT = true; \
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 7: \
{ \
	/* OP Rd,Rb,Rm ROR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		shift &= 0x1f; \
		if (shift) { \
			LOGICAL_ROR_REG	\
		} else { \
			value = arm.r[opcode & 0x0F]; \
			C_OUT = (value & 0x80000000 ? true : false); \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
		C_OUT = (value & 0x80000000 ? true : false); \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 0x200: \
case BASE + 0x201: \
case BASE + 0x202: \
case BASE + 0x203: \
case BASE + 0x204: \
case BASE + 0x205: \
case BASE + 0x206: \
case BASE + 0x207: \
case BASE + 0x208: \
case BASE + 0x209: \
case BASE + 0x20a: \
case BASE + 0x20b: \
case BASE + 0x20c: \
case BASE + 0x20d: \
case BASE + 0x20e: \
case BASE + 0x20f: \
{ \
	int shift = (opcode & 0xF00) >> 7; \
	int base = (opcode >> 16) & 0x0F; \
	int dest = (opcode >> 12) & 0x0F; \
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ROR_IMM	\
	} else { \
		value = opcode & 0xff; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break;

#define LOGICAL_DATA_OPCODE_WITHOUT_base(OPCODE, OPCODE2, BASE)	\
case BASE: \
case BASE + 8: \
{ \
	/* OP Rd,Rb,Rm LSL # */	\
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	  \
	if (shift) { \
		LOGICAL_LSL_REG	\
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 2: \
case BASE + 10:	\
{ \
	/* OP Rd,Rb,Rm LSR # */	\
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_LSR_REG	\
	} else { \
		value = 0; \
		C_OUT = (arm.r[opcode & 0x0F] & 0x80000000) ? true : false;	\
	} \
	  \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 4: \
case BASE + 12:	\
{ \
	/* OP Rd,Rb,Rm ASR # */	\
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ASR_REG	\
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
			C_OUT = true; \
		} else { \
			value = 0; \
			C_OUT = false; \
		}					\
	} \
	  \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 6: \
case BASE + 14:	\
{ \
	/* OP Rd,Rb,Rm ROR # */	\
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ROR_REG	\
	} else { \
		LOGICAL_RRX_REG	\
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 1: \
{ \
	/* OP Rd,Rb,Rm LSL Rs */ \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
			C_OUT = (arm.r[opcode & 0x0F] & 1 ? true : false); \
		} else if (shift < 32) { \
			LOGICAL_LSL_REG	\
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 3: \
{ \
	/* OP Rd,Rb,Rm LSR Rs */ \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
			C_OUT = (arm.r[opcode & 0x0F] & 0x80000000 ? true : false);	\
		} else if (shift < 32) { \
			LOGICAL_LSR_REG	\
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 5: \
{ \
	/* OP Rd,Rb,Rm ASR Rs */ \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift < 32) { \
		if (shift) { \
			LOGICAL_ASR_REG	\
		} else { \
			value = arm.r[opcode & 0x0F]; \
		} \
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
			C_OUT = true; \
		} else { \
			value = 0; \
			C_OUT = false; \
		} \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 7: \
{ \
	/* OP Rd,Rb,Rm ROR Rs */ \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	bool C_OUT = C_FLAG; \
	u32 value; \
	icount--; \
	if (shift) { \
		shift &= 0x1f; \
		if (shift) { \
			LOGICAL_ROR_REG	\
		} else { \
			value = arm.r[opcode & 0x0F]; \
			C_OUT = (value & 0x80000000 ? true : false); \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
		C_OUT = (value & 0x80000000 ? true : false); \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 0x200: \
case BASE + 0x201: \
case BASE + 0x202: \
case BASE + 0x203: \
case BASE + 0x204: \
case BASE + 0x205: \
case BASE + 0x206: \
case BASE + 0x207: \
case BASE + 0x208: \
case BASE + 0x209: \
case BASE + 0x20a: \
case BASE + 0x20b: \
case BASE + 0x20c: \
case BASE + 0x20d: \
case BASE + 0x20e: \
case BASE + 0x20f: \
{ \
	int shift = (opcode & 0xF00) >> 7; \
	int dest = (opcode >> 12) & 0x0F; \
	bool C_OUT = C_FLAG; \
	u32 value; \
	if (shift) { \
		LOGICAL_ROR_IMM	\
	} else { \
		value = opcode & 0xff; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break;

#define ARITHMETIC_DATA_OPCODE(OPCODE, OPCODE2, BASE) \
case BASE: \
case BASE + 8: \
{ \
	/* OP Rd,Rb,Rm LSL # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	if (shift) { \
		ARITHMETIC_LSL_REG \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 2: \
case BASE + 10:	\
{ \
	/* OP Rd,Rb,Rm LSR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	if (shift) { \
		ARITHMETIC_LSR_REG \
	} else { \
		value = 0; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 4: \
case BASE + 12:	\
{ \
	/* OP Rd,Rb,Rm ASR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	if (shift) { \
		ARITHMETIC_ASR_REG \
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
		} else value = 0; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 6: \
case BASE + 14:	\
{ \
	/* OP Rd,Rb,Rm ROR # */	\
	int base = (opcode >> 16) & 0x0F; \
	int shift = (opcode >> 7) & 0x1F; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	if (shift) { \
		ARITHMETIC_ROR_REG \
	} else { \
		ARITHMETIC_RRX_REG \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 1: \
{ \
	/* OP Rd,Rb,Rm LSL Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
		} else if (shift < 32) { \
			ARITHMETIC_LSL_REG \
		} else value = 0; \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 3: \
{ \
	/* OP Rd,Rb,Rm LSR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	icount--; \
	if (shift) { \
		if (shift == 32) { \
			value = 0; \
		} else if (shift < 32) { \
			ARITHMETIC_LSR_REG \
		} else value = 0; \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 5: \
{ \
	/* OP Rd,Rb,Rm ASR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	icount--; \
	if (shift < 32) { \
		if (shift) { \
			ARITHMETIC_ASR_REG \
		} else { \
			value = arm.r[opcode & 0x0F]; \
		} \
	} else { \
		if (arm.r[opcode & 0x0F] & 0x80000000) { \
			value = 0xFFFFFFFF;	\
		} else value = 0; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 7: \
{ \
	/* OP Rd,Rb,Rm ROR Rs */ \
	int base = (opcode >> 16) & 0x0F; \
	int shift = arm.r[(opcode >> 8) & 15]; \
	int dest = (opcode >> 12) & 15;	\
	u32 value; \
	icount--; \
	if (shift) { \
		shift &= 0x1f; \
		if (shift) { \
			ARITHMETIC_ROR_REG \
		} else { \
			value = arm.r[opcode & 0x0F]; \
		} \
	} else { \
		value = arm.r[opcode & 0x0F]; \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break; \
case BASE + 0x200: \
case BASE + 0x201: \
case BASE + 0x202: \
case BASE + 0x203: \
case BASE + 0x204: \
case BASE + 0x205: \
case BASE + 0x206: \
case BASE + 0x207: \
case BASE + 0x208: \
case BASE + 0x209: \
case BASE + 0x20a: \
case BASE + 0x20b: \
case BASE + 0x20c: \
case BASE + 0x20d: \
case BASE + 0x20e: \
case BASE + 0x20f: \
{ \
	int shift = (opcode & 0xF00) >> 7; \
	int base = (opcode >> 16) & 0x0F; \
	int dest = (opcode >> 12) & 0x0F; \
	u32 value; \
	{ \
		ARITHMETIC_ROR_IMM \
	} \
	if (dest == 15) { \
		OPCODE2	\
		/* todo */ \
		if (opcode & 0x00100000) { \
			icount--; \
			armSwitchMode(arm.spsr & 0x1f, false); \
		} \
		arm.r[15] &= 0xFFFFFFFC; \
		arm.pc = arm.r[15];	\
		arm.r[15] += 4;	\
	} else { \
		OPCODE \
	} \
} \
break;

#define ARM_USR 0x00000010			// User mode
#define ARM_FIQ 0x00000011			// Fast Interrupt mode
#define ARM_IRQ 0x00000012			// Interrupt mode
#define ARM_SVC 0x00000013			// Supervisor mode
#define ARM_ABT 0x00000017			// Abort mode
#define ARM_UND 0x0000001B			// Undefined-instruction mode
#define ARM_SYS 0x0000001F			// System mode

#define ARM_N   0x80000000			// Negative
#define ARM_Z   0x40000000			// Zero
#define ARM_C   0x20000000			// Carry
#define ARM_V   0x10000000			// Overflow
#define ARM_Q   0x08000000			// DSP Overflow/Saturated
#define ARM_I   0x00000080			// Interrupts Disabled
#define ARM_F   0x00000040			// Fast Interrupts Disabled
#define ARM_T   0x00000020			// Thumb Mode
#define ARM_M   0x0000001F			// CPSR Mode Mask

#include "types.h"

typedef struct {
	u32 r[16];

	u32 r13usr;
	u32 r14usr;

	u32 r8fiq;
	u32 r9fiq;
	u32 r10fiq;
	u32 r11fiq;
	u32 r12fiq;
	u32 r13fiq;
	u32 r14fiq;

	u32 r13irq;
	u32 r14irq;

	u32 r13svc;
	u32 r14svc;

	u32 r13abt;
	u32 r14abt;

	u32 r13und;
	u32 r14und;

	u32 cpsr;
	u32 overflow;
	u32 carry;
	u32 zero;
	u32 negative;
	u32 spsr;
	u32 fiqspsr;
	u32 irqspsr;
	u32 svcspsr;
	u32 abtspsr;
	u32 undspsr;

	u32 mode;
	u32 pc;
} ARM;

ARM arm;

u32  armOpen();
void armReset();
void armClose();
void armFIQ();
void armExecute(u32 cycle);

void armSwitchMode(int mode, bool state);

#endif