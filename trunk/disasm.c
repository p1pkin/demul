/*
 * Demul
 * Copyright (C) 2006 Org, Demul Team
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
 * Version 0.2: Added LOWERCASE option. Colors to branch opcodes.
 * Version 0.1: All opcodes seems to be implemented.
/*

#include "Demul.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOWERCASE		// Print disassembled text in lowe-case letters.

static char *dinstr[] = {
	"1110nnnniiiiiiii MOV%_#%i%,R%n",
	"1001nnnndddddddd MOV.W%_@(%D%,PC)%,R%n%>[ofs: %Cw > %CW]",
	"1101nnnndddddddd MOV.L%_@(%D%,PC)%,R%n%>[ofs: %Cl > %CL]",
	"0110nnnnmmmm0011 MOV%_R%m%,R%n",
	"0010nnnnmmmm0000 MOV.B%_R%m%,@R%n",
	"0010nnnnmmmm0001 MOV.W%_R%m%,@R%n",
	"0010nnnnmmmm0010 MOV.L%_R%m%,@R%n",
	"0110nnnnmmmm0000 MOV.B%_@R%m%,R%n%>[src: %Mb]",
	"0110nnnnmmmm0001 MOV.W%_@R%m%,R%n%>[src: %Mw]",
	"0110nnnnmmmm0010 MOV.L%_@R%m%,R%n%>[src: %Ml]",
	"0010nnnnmmmm0100 MOV.B%_R%m%,@-R%n",
	"0010nnnnmmmm0101 MOV.W%_R%m%,@-R%n",
	"0010nnnnmmmm0110 MOV.L%_R%m%,@-R%n",
	"0110nnnnmmmm0100 MOV.B%_@R%m+%,R%n%>[src: %Mb]",
	"0110nnnnmmmm0101 MOV.W%_@R%m+%,R%n%>[src: %Mw]",
	"0110nnnnmmmm0110 MOV.L%_@R%m+%,R%n%>[src: %Ml]",
	"10000000nnnndddd MOV.B%_R0%,@(%d%,R%n)",
	"10000001nnnndddd MOV.W%_R0%,@(%d%,R%n)",
	"0001nnnnmmmmdddd MOV.L%_R%m%,@(%d%,R%n)",
	"10000100mmmmdddd MOV.B%_@(%d%,R%m)%,R0",
	"10000101mmmmdddd MOV.W%_@(%d%,R%m)%,R0",
	"0101nnnnmmmmdddd MOV.L%_@(%d%,R%m)%,R%n",
	"0000nnnnmmmm0100 MOV.B%_R%m%,@(R0%,R%n)",
	"0000nnnnmmmm0101 MOV.W%_R%m%,@(R0%,R%n)",
	"0000nnnnmmmm0110 MOV.L%_R%m%,@(R0%,R%n)",
	"0000nnnnmmmm1100 MOV.B%_@(R0%,R%m)%,R%n",
	"0000nnnnmmmm1101 MOV.W%_@(R0%,R%m)%,R%n",
	"0000nnnnmmmm1110 MOV.L%_@(R0%,R%m)%,R%n",
	"11000000dddddddd MOV.B%_R0%,@(%D%,GBR)",
	"11000001dddddddd MOV.W%_R0%,@(%D%,GBR)",
	"11000010dddddddd MOV.L%_R0%,@(%D%,GBR)",
	"11000100dddddddd MOV.B%_@(%D%,GBR)%,R0",
	"11000101dddddddd MOV.W%_@(%D%,GBR)%,R0",
	"11000110dddddddd MOV.L%_@(%D%,GBR)%,R0",
	"11000111dddddddd MOVA%_@(%D%,PC)%,R0%>[ofs: %Cl > %CL]",
	"0000nnnn00101001 MOVT%_R%n",
	"0110nnnnmmmm1000 SWAP.B%_R%m%,R%n",
	"0110nnnnmmmm1001 SWAP.W%_R%m%,R%n",
	"0010nnnnmmmm1101 XTRCT%_R%m%,R%n",
	"0011nnnnmmmm1100 ADD%_R%m%,R%n",
	"0111nnnniiiiiiii ADD%_#%i%,R%n",
	"0011nnnnmmmm1110 ADDC%_R%m%,R%n",
	"0011nnnnmmmm1111 ADDV%_R%m%,R%n",
	"10001000iiiiiiii CMP/EQ%_#%i%,R0",
	"0011nnnnmmmm0000 CMP/EQ%_R%m%,R%n",
	"0011nnnnmmmm0010 CMP/HS%_R%m%,R%n",
	"0011nnnnmmmm0011 CMP/GE%_R%m%,R%n",
	"0011nnnnmmmm0110 CMP/HI%_R%m%,R%n",
	"0011nnnnmmmm0111 CMP/GT%_R%m%,R%n",
	"0100nnnn00010001 CMP/PZ%_R%n",
	"0100nnnn00010101 CMP/PL%_R%n",
	"0010nnnnmmmm1100 CMP/STR%_R%m%,R%n",
	"0011nnnnmmmm0100 DIV1%_R%m%,R%n",
	"0010nnnnmmmm0111 DIV0S%_R%m%,R%n",
	"0000000000011001 DIV0U",
	"0011nnnnmmmm1101 DMULS.L%_R%m%,R%n",
	"0011nnnnmmmm0101 DMULU.L%_R%m%,R%n",
	"0100nnnn00010000 DT%_R%n",
	"0110nnnnmmmm1110 EXTS.B%_R%m%,R%n",
	"0110nnnnmmmm1111 EXTS.W%_R%m%,R%n",
	"0110nnnnmmmm1100 EXTU.B%_R%m%,R%n",
	"0110nnnnmmmm1101 EXTU.W%_R%m%,R%n",
	"0000nnnnmmmm1111 MAC.L%_@R%m+%,@R%n+%>[src: %Ml, dst: %Nl]",
	"0100nnnnmmmm1111 MAC.W%_@R%m+%,@R%n+%>[src: %Mw, dst: %Nw]",
	"0000nnnnmmmm0111 MUL.L%_R%m%,R%n",
	"0010nnnnmmmm1111 MULS.W%_R%m%,R%n",
	"0010nnnnmmmm1110 MULU.W%_R%m%,R%n",
	"0110nnnnmmmm1011 NEG%_R%m%,R%n",
	"0110nnnnmmmm1010 NEGC%_R%m%,R%n",
	"0011nnnnmmmm1000 SUB%_R%m%,R%n",
	"0011nnnnmmmm1010 SUBC%_R%m%,R%n",
	"0011nnnnmmmm1011 SUBV%_R%m%,R%n",
	"0100nnnn00000100 ROTL%_R%n",
	"0100nnnn00000101 ROTR%_R%n",
	"0100nnnn00100100 ROTCL%_R%n",
	"0100nnnn00100101 ROTCR%_R%n",
	"0100nnnnmmmm1100 SHAD%_R%m%,R%n",
	"0100nnnn00100000 SHAL%_R%n",
	"0100nnnn00100001 SHAR%_R%n",
	"0100nnnnmmmm1101 SHLD%_R%m%,R%n",
	"0100nnnn00000000 SHLL%_R%n",
	"0100nnnn00000001 SHLR%_R%n",
	"0100nnnn00001000 SHLL2%_R%n",
	"0100nnnn00001001 SHLR2%_R%n",
	"0100nnnn00011000 SHLL8%_R%n",
	"0100nnnn00011001 SHLR8%_R%n",
	"0100nnnn00101000 SHLL16%_R%n",
	"0100nnnn00101001 SHLR16%_R%n",
	"10001011dddddddd BF%_%x%>",
	"10001111dddddddd BF/S%_%x%>",
	"10001001dddddddd BT%_%x%>",
	"10001101dddddddd BT/S%_%x%>",
	"1010dddddddddddd BRA%_%X%>",
	"0000nnnn00100011 BRAF%_R%n%>",
	"1011dddddddddddd BSR%_%X%>",
	"0000nnnn00000011 BSRF%_R%n%>",
	"0100nnnn00101011 JMP%_R%n%>",
	"0100nnnn00001011 JSR%_R%n%>",
	"0000000000001011 RTS",
	"0000000000101000 CLRMAC",
	"0000000001001000 CLRS",
	"0000000000001000 CLRT",
	"0100mmmm00001110 LDC%_R%m%,SR",
	"0100mmmm00011110 LDC%_R%m%,GBR",
	"0100mmmm00101110 LDC%_R%m%,VBR",
	"0100mmmm00111110 LDC%_R%m%,SSR",
	"0100mmmm01001110 LDC%_R%m%,SPC",
	"0100mmmm11111010 LDC%_R%m%,DBR",
	"0100mmmm1nnn1110 LDC%_R%m%,R%n_BANK",
	"0100mmmm00000111 LDC.L%_@R%m+%,SR%>[src: %Ml]",
	"0100mmmm00010111 LDC.L%_@R%m+%,GBR%>[src: %Ml]",
	"0100mmmm00100111 LDC.L%_@R%m+%,VBR%>[src: %Ml]",
	"0100mmmm00110111 LDC.L%_@R%m+%,SSR%>[src: %Ml]",
	"0100mmmm01000111 LDC.L%_@R%m+%,SPC%>[src: %Ml]",
	"0100mmmm11110110 LDC.L%_@R%m+%,DBR%>[src: %Ml]",
	"0100mmmm1nnn0111 LDC.L%_@R%m+%,R%n_BANK%>[src: %Ml]",
	"0100mmmm00001010 LDS%_R%m%,MACH",
	"0100mmmm00011010 LDS%_R%m%,MACL",
	"0100mmmm00101010 LDS%_R%m%,PR",
	"0100mmmm00000110 LDS.L%_@R%m+%,MACH%>[src: %Ml]",
	"0100mmmm00010110 LDS.L%_@R%m+%,MACL%>[src: %Ml]",
	"0100mmmm00100110 LDS.L%_@R%m+%,PR%>[src: %Ml]",
	"0000000000111000 LDTLB",
	"0000nnnn11000011 MOVCA.L%_R0%,@R%n",
	"0000000000001001 NOP",
	"0000nnnn10010011 OCBI%_@R%n%>[src: %Ml]",
	"0000nnnn10100011 OCBP%_@R%n%>[src: %Ml]",
	"0000nnnn10110011 OCBWB%_@R%n%>[src: %Ml]",
	"0000nnnn10000011 PREF%_@R%n%>[src: %Ml]",
	"0000000000101011 RTE",
	"0000000001011000 SETS",
	"0000000000011000 SETT",
	"0000000000011011 SLEEP",
	"0000nnnn00000010 STC%_SR%,R%n",
	"0000nnnn00010010 STC%_GBR%,R%n",
	"0000nnnn00100010 STC%_VBR%,R%n",
	"0000nnnn00110010 STC%_SSR%,R%n",
	"0000nnnn01000010 STC%_SPC%,R%n",
	"0000nnnn00111010 STC%_SGR%,R%n",
	"0000nnnn11111010 STC%_DBR%,R%n",
	"0000nnnn1mmm0010 STC%_R%m_BANK%,R%n",
	"0100nnnn00000011 STC.L%_SR%,@-R%n",
	"0100nnnn00010011 STC.L%_GBR%,@-R%n",
	"0100nnnn00100011 STC.L%_VBR%,@-R%n",
	"0100nnnn00110011 STC.L%_SSR%,@-R%n",
	"0100nnnn01000011 STC.L%_SPC%,@-R%n",
	"0100nnnn00110010 STC.L%_SGR%,@-R%n",
	"0100nnnn11110010 STC.L%_DBR%,@-R%n",
	"0100nnnn1mmm0011 STC.L%_R%m_BANK%,@-R%n",
	"0000nnnn00001010 STS%_MACH%,R%n",
	"0000nnnn00011010 STS%_MACL%,R%n",
	"0000nnnn00101010 STS%_PR%,R%n",
	"0100nnnn00000010 STS.L%_MACH%,@-R%n",
	"0100nnnn00010010 STS.L%_MACL%,@-R%n",
	"0100nnnn00100010 STS.L%_PR%,@-R%n",
	"11000011iiiiiiii TRAPA%_#%i",
	"1111nnnn10001101 FLDI0%_FR%n",
	"1111nnnn10011101 FLDI1%_FR%n",
	"1111nnnnmmmm1100 FMOV%_FR%m%,FR%n",
	"1111nnnnmmmm1000 FMOV.S%_@R%m%,FR%n%>[src: %Ml > %Mf]",
	"1111nnnnmmmm0110 FMOV.S%_@(R0%,R%m)%,FR%n",
	"1111nnnnmmmm1001 FMOV.S%_@R%m+%,FR%n%>[src: %Ml > %Mf]",
	"1111nnnnmmmm1010 FMOV.S%_FR%m%,@R%n",
	"1111nnnnmmmm1011 FMOV.S%_FR%m%,@-R%n",
	"1111nnnnmmmm0111 FMOV.S%_FR%m%,@(R0%,R%n)",
	"1111nnn0mmm01100 FMOV%_DR%m%,DR%n",
	"1111nnn0mmmm1000 FMOV%_@R%m%,DR%n%>[src: %Md]",
	"1111nnn0mmmm0110 FMOV%_@(R0%,R%m)%,DR%n",
	"1111nnn0mmmm1001 FMOV%_@R%m+%,DR%n%>[src: %Md]",
	"1111nnnnmmm01010 FMOV%_DR%m%,@R%n",
	"1111nnnnmmm01011 FMOV%_DR%m%,@-R%n",
	"1111nnnnmmm00111 FMOV%_DR%m%,@(R0%,R%n)",
	"1111mmmm00011101 FLDS%_FR%m%,FPUL",
	"1111nnnn00001101 FSTS%_FPUL%,FR%n",
	"1111nnnn01011101 FABS%_FR%n",
	"1111nnnnmmmm0000 FADD%_FR%m%,FR%n",
	"1111nnnnmmmm0100 FCMP/EQ%_FR%m%,FR%n",
	"1111nnnnmmmm0101 FCMP/GT%_FR%m%,FR%n",
	"1111nnnnmmmm0011 FDIV%_FR%m%,FR%n",
	"1111nnnn00101101 FLOAT%_FPUL%,FR%n",
	"1111nnnnmmmm1110 FMAC%_FR0%,FR%m%,FR%n",
	"1111nnnnmmmm0010 FMUL%_FR%m%,FR%n",
	"1111nnnn01001101 FNEG%_FR%n",
	"1111nnnn01101101 FSQRT%_FR%n",
	"1111nnnnmmmm0001 FSUB%_FR%m%,FR%n",
	"1111mmmm00111101 FTRC%_FR%m%,FPUL",
	"1111nnn001011101 FABS%_DR%n",
	"1111nnn0mmm00000 FADD%_DR%m%,DR%n",
	"1111nnn0mmm00100 FCMP/EQ%_DR%m%,DR%n",
	"1111nnn0mmm00101 FCMP/GT%_DR%m%,DR%n",
	"1111nnn0mmm00011 FDIV%_DR%m%,DR%n",
	"1111mmm010111101 FCNVDS%_DR%m%,FPUL",
	"1111nnn010101101 FCNVSD%_FPUL%,DR%n",
	"1111nnn000101101 FLOAT%_FPUL%,DR%n",
	"1111nnn0mmm00010 FMUL%_DR%m%,DR%n",
	"1111nnn001001101 FNEG%_DR%n",
	"1111nnn001101101 FSQRT%_DR%n",
	"1111nnn0mmm00001 FSUB%_DR%m%,DR%n",
	"1111mmm000111101 FTRC%_DR%m%,FPUL",
	"0100mmmm01101010 LDS%_R%m%,FPSCR",
	"0100mmmm01011010 LDS%_R%m%,FPUL",
	"0100mmmm01100110 LDS.L%_@R%m+%,FPSCR%>[src: %Ml]",
	"0100mmmm01010110 LDS.L%_@R%m+%,FPUL%>[src: %Ml]",
	"0000nnnn01101010 STS%_FPSCR%,R%n",
	"0000nnnn01011010 STS%_FPUL%,R%n",
	"0100nnnn01100010 STS.L%_FPSCR%,@-R%n",
	"0100nnnn01010010 STS.L%_FPUL%,@-R%n",
	"1111nnn1mmm01100 FMOV%_DR%m%,XD%n",
	"1111nnn0mmm11100 FMOV%_XD%m%,DR%n",
	"1111nnn1mmm11100 FMOV%_XD%m%,XD%n",
	"1111nnn1mmmm1000 FMOV%_@R%m%,XD%n%>[src: %Md]",
	"1111nnn1mmmm1001 FMOV%_@R%m+%,XD%n%>[src: %Md]",
	"1111nnn1mmmm0110 FMOV%_@(R0%,R%m)%,XD%n",
	"1111nnnnmmm11010 FMOV%_XD%m%,@R%n",
	"1111nnnnmmm11011 FMOV%_XD%m%,@-R%n",
	"1111nnnnmmm10111 FMOV%_XD%m%,@(R0%,R%n)",
	"1111nnmm11101101 FIPR%_FV%m%,FV%n",
	"1111nn0111111101 FTRV%_XMTRX%,FV%n",
	"1111101111111101 FRCHG",
	"1111001111111101 FSCHG",
	"0010nnnnmmmm1001 AND%_R%m%,R%n",
	"11001001iiiiiiii AND%_#%i%,R0",
	"11001101iiiiiiii AND.B%_#%i%,@(R0%,GBR)",
	"0110nnnnmmmm0111 NOT%_R%m%,R%n",
	"0010nnnnmmmm1011 OR%_R%m%,R%n",
	"11001011iiiiiiii OR%_#%i%,R0",
	"11001111iiiiiiii OR.B%_#%i%,@(R0%,GBR)",
	"0100nnnn00011011 TAS.B%_@R%n%>[src: %Mb]",
	"0010nnnnmmmm1000 TST%_R%m%,R%n",
	"11001000iiiiiiii TST%_#%i%,R0",
	"11001100iiiiiiii TST.B%_#%i%,@(R0%,GBR)",
	"0010nnnnmmmm1010 XOR%_R%m%,R%n",
	"11001010iiiiiiii XOR%_#%i%,R0",
	"11001110iiiiiiii XOR.B%_#%i%,@(R0%,GBR)",

	0
};

static char dout[256];

char *Disasm(u32 pc, u16 op) {
	char **s = dinstr, *p, *str, *ptr, c, token, subtoken;
	int i;
	u16 t, m;
	int _i, _d, _n, _m;		// Opcode fields.

	dout[0] = '\0';

	while (*s) {
		// Construct [m]ask and create [t]est value; Collect opcode field info.
		_i = _d = _n = _m = 0;
		m = t = 0;
		p = *s;
		while (*p <= ' ') p++;
		for (i = 0; i < 16; i++) {
			t <<= 1;
			m <<= 1;
			switch (p[i]) {
			case '1': t |= 1; m |= 1; break;
			case '0': m |= 1;
			case 'i': _i <<= 1; _i |= (op >> (15 - i)) & 1; break;
			case 'd': _d <<= 1; _d |= (op >> (15 - i)) & 1; break;
			case 'n': _n <<= 1; _n |= (op >> (15 - i)) & 1; break;
			case 'm': _m <<= 1; _m |= (op >> (15 - i)) & 1; break;
			}
		}
		p += 16;
		while (*p <= ' ' && *p >= 3 && *p) p++;

		if (_i & 0x80) _i |= 0xFFFFFF00;
		_n &= 0xf;
		_m &= 0xf;

		// Check if test value match with opcode. If yes - disassemble opcode.
		if ((op & m) == t) {
			str = p;
			ptr = dout;

			while (c = *str++) {
				if (c == '%') {
					token = *str++;
					switch (token) {
					case 'i':
						ptr += sprintf(ptr, "%i", (s32)(s16)(s8)_i);
						break;
					case 'd':
						if (_d & 0x8) _d |= 0xFFFFFFF0;
						ptr += sprintf(ptr, "%i", _d);
						break;
					case 'D':
						if (_d & 0x80) _d |= 0xFFFFFF00;
						ptr += sprintf(ptr, "%i", _d);
						break;
					case 'Z':
						if (_d & 0x800) _d |= 0xFFFFF000;
						ptr += sprintf(ptr, "%i", _d);
						break;
					case 'x':
						if (_d & 0x80) _d |= 0xFFFFFF00;
						_d <<= 1;
						ptr += sprintf(ptr, "0x%08X", pc + (u32)_d + 4);
						break;
					case 'X':
						if (_d & 0x800) _d |= 0xFFFFF000;
						_d <<= 1;
						ptr += sprintf(ptr, "0x%08X", pc + (u32)_d + 4);
						break;
					case 'C':
						subtoken = *str++;
						switch (subtoken) {
						case 'w': ptr += sprintf(ptr, "0x%08X", (_d << 1) + pc + 4); break;
						case 'l': ptr += sprintf(ptr, "0x%08X", (_d << 2) + (pc & 0xFFFFFFFC) + 4); break;
						case 'W': ptr += sprintf(ptr, "0x%08X", memRead16((_d << 1) + pc + 4)); break;
						case 'L': ptr += sprintf(ptr, "0x%08X", memRead32((_d << 2) + (pc & 0xFFFFFFFC) + 4)); break;
						}
						break;
					case 'n':
						ptr += sprintf(ptr, "%i", _n);
						break;
					case 'N':
						subtoken = *str++;
						switch (subtoken) {
						case 'w': ptr += sprintf(ptr, "0x%04X", memRead16(sh4.r[_n])); break;
						case 'l': ptr += sprintf(ptr, "0x%08X", memRead32(sh4.r[_n])); break;
						}
						break;
					case 'm':
						ptr += sprintf(ptr, "%i", _m);
						break;
					case 'M':
						subtoken = *str++;
						switch (subtoken) {
						case 'b': ptr += sprintf(ptr, "0x%02X", memRead8(sh4.r[_m])); break;
						case 'w': ptr += sprintf(ptr, "0x%04X", memRead16(sh4.r[_m])); break;
						case 'l': ptr += sprintf(ptr, "0x%08X", memRead32(sh4.r[_m])); break;
						case 'f': ptr += sprintf(ptr, "%lg", memRead32(sh4.r[_m])); break;
						case 'd':
						{
							double op;
							memRead64(sh4.r[_m], (u64*)&op);
							ptr += sprintf(ptr, "%llg", op);
							break;
						}
						}
						break;
					case '_':
						t = ptr - dout;
						while (t < 10) {
							*ptr++ = ' '; t++;
						}
						break;
					case '>':
						t = ptr - dout;
						while (t < 26) {
							*ptr++ = ' '; t++;
						}
						break;
					case ',':
						ptr += sprintf(ptr, ", ");
						break;
					}
				} else {
					*ptr = c;
					ptr++;
				}
			}

			*ptr = '\0';
			return dout;
		}

		s++;
	}

	return ">WTF OPCODE?";
}

u16 disMaskOpcode(u16 code) {
	switch ((code >> 12) & 0xf) {
	case 0:
		switch ((code >> 0) & 0xf) {
		case 2:
		case 3:
		case 8:
		case 9:
		case 10:
		case 11:
			code &= 0xF0FF;
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 12:
		case 13:
		case 14:
		case 15:
			code &= 0xF00F;
			break;
		}
		break;
	case 1:
	case 5:
	case 7:
	case 9:
	case 10:
	case 11:
	case 13:
	case 14:
		code &= 0xF000;
		break;
	case 2:
	case 3:
	case 6:
	case 8:
	case 12:
		code &= 0xF00F;
		break;
	case 4:
		switch ((code >> 0) & 0xf) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 14:
		case 11:
			code &= 0xF0FF;
			break;
		case 12:
		case 13:
		case 15:
			code &= 0xF00F;
			break;
		}
		break;
	case 15:
		switch ((code >> 0) & 0xf) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 14:
		case 15:
			code &= 0xF00F;
			break;
		case 13:
			code &= 0xF0FF;
			break;
		}
		break;
	}
	return code;
}

