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
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include "debug.h"
#include "demul.h"
#include "memory.h"
#include "inifile.h"
#include "configure.h"

#ifdef  DEMUL_DEBUG

// Private structures and definitions for entire module.
static HANDLE debug_thread;

u32 debug_active;
u32 debug_step_mode;
u32 debug_trace_mode;
u32 debug_exception_mode = 1;
u32 debug_run_after_break;
u32 debug_new_pc;

// debugger dimensions
#define DB_WIDTH        80
#define DB_HEIGHT       60

// debugger windows
#define DB_UPDATE_REGS  (0x0001)	// registers view window
#define DB_UPDATE_DATA  (0x0002)	// memory view window
#define DB_UPDATE_DISA  (0x0004)	// disassembly window
#define DB_UPDATE_MSGS  (0x0008)	// message history
#define DB_UPDATE_EDIT  (0x0010)	// command line
#define DB_UPDATE_STAT  (0x0020)	// status line
#define DB_UPDATE_ALL   (0x003f)	// all

#define DB_LINES        1024		// lines in message buffer
#define DB_LINELEN      256			// length of single line (including color codes)
#define DB_TOKENS       10			// max amount of command arguments

// breakpoint constants
#define BP_READ     (0x00000001)
#define BP_WRITE    (0x00000002)
#define BP_EXECUTE  (0x00000004)
#define BP_ALL      (0x00000007)

// current console window identifier
typedef enum FOCUSWND { WREGS = 0, WDATA, WDISA, WCONSOLE } FOCUSWND;

// current registers window mode (scroll by F6)
typedef enum REGWNDMODE { REGMOD_GPR = 0, REGMOD_FPR, REGMOD_MAX } REGWNDMODE;

typedef struct DebugWind {
	u32 update;									// update windows
	FOCUSWND focus;								// wregs, wdata, wdisa, wconsole
	REGWNDMODE regmode;							// register window mode
	u32 data;									// start address for data
	u32 text;									// start address for disa
	u32 disa_cursor;							// disa cursor
	int show_stats;								// show statistics, instead reg view. (Shift-F1 to toggle)
	int regs_y;									// Y registers window
	int regs_h;									// Height registsrs window
	int data_y;									// Y Data window
	int data_h;									// Height Data window
	int disa_y;									// Y Disassembler
	int disa_h;									// Height of Disassembler
	int roll_y;									// Y Scroller (updates automatically by recalcwnds() )
	int roll_h;									// Height scroller, updates automaically by recalcwnds()
	int edit_y;									// Y Edit Line
	int edit_h;									// Editor Line Height
	int stat_y;									// Y Status Line
	int stat_h;									// Status Line Height
	u32 visible;								// visible windows
} DebugWind;

typedef struct DebugRoller {
	char data[DB_LINES][DB_LINELEN];			// scrolling lines
	int rollpos;								// Where to read (len = wind.roll_h-1)
	int viewpos;								// Current position of "window" which transfers to CON.buf
	char statusline[256];						// console status line
	char editline[DB_LINELEN];					// edit line
	int editpos;								// position to next char in edit line
	int editlen;								// edit line len
	char history[256][DB_LINELEN];				// command history
	int historypos;								// command history position
	int historycur;								// current command history position
	bool autoscroll;							// if true, then viewpos = rollpos-1
} DebugRoller;

static DebugWind wind;
static DebugRoller roll;

int     db_wraproll(int roll, int value);
void    db_recalc_wnds();
void    db_status(char *txt);
void    db_set_autoscroll(bool value);
void    db_change_focus(FOCUSWND newfocus);

void    db_update(u32 mask);			// check areas for refresh
void    db_refresh(void);				// do actual refresh for checked areas

void    db_memorize_cpu_regs();
void    db_update_registers();
void    db_disa_reset(void);
bool    db_disa_cur_visible(void);
void    db_set_disa_cur(u32 addr);
void    db_disa_key(char ch, int ctrl);
void    db_update_disa_window(void);
void    db_data_key(char ch, int ctrl);
void    db_update_dump_window(void);

u32 FASTCALL db_search_breakpoint(u32 cond, u32 eaddr, u32 align);
void    db_add_breakpoint(u32 cond, u32 eaddr, u32 range);
void    db_clear_breakpoint(u32 number);
void    db_clear_breakpoints(void);
void    db_list_breakpoints(void);

void    db_keep_updated(void);

void    db_command(char *line);

// ---------------------------------------------------------------------------
// Regs.

// register memory
static SH4 sh4_old;

typedef struct {
	char *name;
	u32 x;
	u32 y;
	u32  *value;
	u32  *old_value;
} GENREGS;

static void print_reg(GENREGS reg) {
	if (*reg.value != *reg.old_value) {
		ConPrintfAt(reg.x, reg.y + wind.regs_y + 1, CC_CYAN "%-3s: " CC_GREEN "%08X", reg.name, *reg.value);
		*reg.old_value = *reg.value;
	} else
		ConPrintfAt(reg.x, reg.y + wind.regs_y + 1, CC_CYAN "%-3s: " CC_NORM "%08X", reg.name, *reg.value);
}

static GENREGS gen_regs[] = {
	{ "R0", 0, 0, &sh4.r[0], &sh4_old.r[0] },
	{ "R1", 0, 1, &sh4.r[1], &sh4_old.r[1] },
	{ "R2", 0, 2, &sh4.r[2], &sh4_old.r[2] },
	{ "R3", 0, 3, &sh4.r[3], &sh4_old.r[3] },
	{ "R4", 0, 4, &sh4.r[4], &sh4_old.r[4] },
	{ "R5", 0, 5, &sh4.r[5], &sh4_old.r[5] },
	{ "R6", 0, 6, &sh4.r[6], &sh4_old.r[6] },
	{ "R7", 0, 7, &sh4.r[7], &sh4_old.r[7] },
	{ "R8", 0, 8, &sh4.r[8], &sh4_old.r[8] },
	{ "R9", 0, 9, &sh4.r[9], &sh4_old.r[9] },
	{ "R10", 0, 10, &sh4.r[10], &sh4_old.r[10] },
	{ "R11", 0, 11, &sh4.r[11], &sh4_old.r[11] },
	{ "R12", 0, 12, &sh4.r[12], &sh4_old.r[12] },
	{ "R13", 0, 13, &sh4.r[13], &sh4_old.r[13] },
	{ "R14", 0, 14, &sh4.r[14], &sh4_old.r[14] },
	{ "R15", 0, 15, &sh4.r[15], &sh4_old.r[15] },
	{ "R0.B", 15, 0, &sh4.r[16], &sh4_old.r[16] },
	{ "R1.B", 15, 1, &sh4.r[17], &sh4_old.r[17] },
	{ "R2.B", 15, 2, &sh4.r[18], &sh4_old.r[18] },
	{ "R3.B", 15, 3, &sh4.r[19], &sh4_old.r[19] },
	{ "R4.B", 15, 4, &sh4.r[20], &sh4_old.r[20] },
	{ "R5.B", 15, 5, &sh4.r[21], &sh4_old.r[21] },
	{ "R6.B", 15, 6, &sh4.r[22], &sh4_old.r[22] },
	{ "R7.B", 15, 7, &sh4.r[23], &sh4_old.r[23] },
	{ "MACH", 15, 9, &sh4.mach, &sh4_old.mach },
	{ "MACL", 15, 10, &sh4.macl, &sh4_old.macl },
	{ "FPUL", 15, 11, &sh4.fpul, &sh4_old.fpul },
	{ "PR  ", 15, 12, &sh4.pr, &sh4_old.pr },
	{ "PC  ", 15, 13, &sh4.pc, &sh4_old.pc },
	{ "SR  ", 15, 14, &sh4.sr.all, &sh4_old.sr.all },
	{ "SSR", 31, 0, &sh4.ssr, &sh4_old.ssr },
	{ "SPC", 31, 1, &sh4.spc, &sh4_old.spc },
	{ "SGR", 31, 2, &sh4.sgr, &sh4_old.sgr },
	{ "GBR", 31, 3, &sh4.gbr, &sh4_old.gbr },
	{ "DBR", 31, 4, &sh4.dbr, &sh4_old.dbr },
	{ "VBR", 31, 5, &sh4.vbr, &sh4_old.vbr },
};

static int gen_regs_num = sizeof(gen_regs) / sizeof(GENREGS);

static void db_print_gprs() {
	int i;
	for (i = 0; i < gen_regs_num; i++)
		print_reg(gen_regs[i]);
}

// ---

static GENREGS fp_regs[] = {
	{ "FR0 ", 0, 0, &sh4.flti[0], &sh4_old.flti[0] },
	{ "FR1 ", 0, 1, &sh4.flti[1], &sh4_old.flti[1] },
	{ "FR2 ", 0, 2, &sh4.flti[2], &sh4_old.flti[2] },
	{ "FR3 ", 0, 3, &sh4.flti[3], &sh4_old.flti[3] },
	{ "FR4 ", 0, 4, &sh4.flti[4], &sh4_old.flti[4] },
	{ "FR5 ", 0, 5, &sh4.flti[5], &sh4_old.flti[5] },
	{ "FR6 ", 0, 6, &sh4.flti[6], &sh4_old.flti[6] },
	{ "FR7 ", 0, 7, &sh4.flti[7], &sh4_old.flti[7] },
	{ "FR8 ", 0, 8, &sh4.flti[8], &sh4_old.flti[8] },
	{ "FR9 ", 0, 9, &sh4.flti[9], &sh4_old.flti[9] },
	{ "FR10", 0, 10, &sh4.flti[10], &sh4_old.flti[10] },
	{ "FR11", 0, 11, &sh4.flti[11], &sh4_old.flti[11] },
	{ "FR12", 0, 12, &sh4.flti[12], &sh4_old.flti[12] },
	{ "FR13", 0, 13, &sh4.flti[13], &sh4_old.flti[13] },
	{ "FR14", 0, 14, &sh4.flti[14], &sh4_old.flti[14] },
	{ "FR15", 0, 15, &sh4.flti[15], &sh4_old.flti[15] },
	{ "XR0 ", 16, 0, &sh4.flti[16], &sh4_old.flti[16] },
	{ "XR1 ", 16, 1, &sh4.flti[17], &sh4_old.flti[17] },
	{ "XR2 ", 16, 2, &sh4.flti[18], &sh4_old.flti[18] },
	{ "XR3 ", 16, 3, &sh4.flti[19], &sh4_old.flti[19] },
	{ "XR4 ", 16, 4, &sh4.flti[20], &sh4_old.flti[20] },
	{ "XR5 ", 16, 5, &sh4.flti[21], &sh4_old.flti[21] },
	{ "XR6 ", 16, 6, &sh4.flti[22], &sh4_old.flti[22] },
	{ "XR7 ", 16, 7, &sh4.flti[23], &sh4_old.flti[23] },
	{ "XR8 ", 16, 8, &sh4.flti[24], &sh4_old.flti[24] },
	{ "XR9 ", 16, 9, &sh4.flti[25], &sh4_old.flti[25] },
	{ "XR10", 16, 10, &sh4.flti[26], &sh4_old.flti[26] },
	{ "XR11", 16, 11, &sh4.flti[27], &sh4_old.flti[27] },
	{ "XR12", 16, 12, &sh4.flti[28], &sh4_old.flti[28] },
	{ "XR13", 16, 13, &sh4.flti[29], &sh4_old.flti[29] },
	{ "XR14", 16, 14, &sh4.flti[30], &sh4_old.flti[30] },
	{ "XR15", 16, 15, &sh4.flti[31], &sh4_old.flti[31] },
	{ "FPUL ", 32, 0, &sh4.fpul, &sh4_old.fpul },
	{ "FPSCR", 32, 1, &sh4.fpscr.all, &sh4_old.fpscr.all },
};

static int fp_regs_num = sizeof(fp_regs) / sizeof(GENREGS);

static void db_print_fprs(void) {
	int i;
	for (i = 0; i < fp_regs_num; i++)
		print_reg(fp_regs[i]);
}

// ---

void db_memorize_cpu_regs() {
	memcpy(&sh4_old, &sh4, sizeof(SH4));
}

void db_update_registers() {
	if (wind.show_stats) return;	// show statistics instead registers.

	ConFillRegion(wind.regs_y, 1, CC_BLACK CB_CYAN "-");
	if (wind.focus == WREGS) ConPrintfAt(0, wind.regs_y, CC_WHITE CB_CYAN "\x1f");
	ConPrintfAt(2, wind.regs_y, CC_BLACK CB_CYAN "F1");

	switch (wind.regmode) {
	case REGMOD_GPR:    ConPrintfAt(6, wind.regs_y, CC_BLACK CB_CYAN " General Registers");
		db_print_gprs();
		break;
	case REGMOD_FPR:    ConPrintfAt(6, wind.regs_y, CC_BLACK CB_CYAN " Floating-Point Registers");
		db_print_fprs();
		break;
	}
}

// ---------------------------------------------------------------------------
// Disassembly.

static s32 detect_branch(u32 eaddr, u16 instr) {
	switch (instr & 0xF000) {
	case 0x8000:
		switch (instr & 0x0F00) {
		case 0x0F00:
		case 0x0B00:
			if (!sh4.sr.t)
				if ((instr & 0x0080))
					return -1;
				else
					return 1;
			else
				return 0;
			break;
		case 0x0900:
		case 0x0D00:
			if (sh4.sr.t)
				if (instr & 0x0080)
					return -1;
				else
					return 1;
			else
				return 0;
			break;
		}
		break;
	case 0xA000:
	case 0xB000:
		if (instr & 0x0800)
			return -1;
		else
			return 1;
		break;
	case 0x0000:
		if ((instr & 0x00DF) == 0x0003) {
			u32 n = (instr & 0x0F00) >> 8;
			if (sh4.r[n] & 0x80000000)
				return -1;
			else
				return 1;
		}
		break;
	case 0x4000:
		if ((instr & 0x00DF) == 0x000B) {
			u32 n = (instr & 0x0F00) >> 8;
			if (sh4.r[n] < eaddr)
				return -1;
			else
				return 1;
		}
		break;
	}
	return 0;
}

static int disa_sub_h;

static int cpu_disa_line(int line, u32 instr, u32 eaddr, bool nomem) {
	int addend = 1, detect;
	int bg, bgcur, bgbp;
	char *str, bgstr[16];

	instr &= 0xFFFF;

	// Dark magic for line background color :)
	bgcur = (eaddr == wind.disa_cursor) ? (8) : (0);
	bgbp = (db_search_breakpoint(BP_EXECUTE, eaddr, 2) != 0) ? (4) : (0);
	bg = (eaddr == sh4.pc) ? (1) : (0);
	bg = bg ^ bgcur ^ bgbp;
	if (bg == 0) bg = 0x10;	// black!
	bgstr[0] = 0x2;
	bgstr[1] = bg;
	bgstr[2] = ' ';
	bgstr[3] = 0;
	ConFillRegion(line, 1, bgstr);
	bgstr[2] = 0;

	if (nomem) {
		ConPrintfAt(0, line, CC_NORM "%s%08X  ", bgstr, eaddr);
		ConPrintfAt(10, line, CC_CYAN "%s????  ", bgstr);
		ConPrintfAt(16, line, CC_NORM "%s???", bgstr);
		return addend;
	}

	ConPrintfAt(0, line, CC_NORM "%s%08X  ", bgstr, eaddr);
	ConPrintfAt(10, line, CC_CYAN "%s%04X  ", bgstr, instr);

	// Disassemble SH4 Instruction
	str = Disasm(eaddr, instr);
	if (eaddr != sh4.pc)
		ConPrintfAt(16, line, CC_NORM "%s%s", bgstr, str);
	else {
		detect = detect_branch(eaddr, instr);
		ConPrintfAt(16, line, CC_NORM "%s%s%s", bgstr, str, detect == 0 ? "" : detect < 0 ? "[JUMP UP]" : "[JUMP DOWN]");
	}
	return addend;
}

static void db_disa_cpu(void) {
	int line, n;
	u32 instr = 0, eaddr = wind.text;
	disa_sub_h = 0;

	memQuietMode(true);
	for (line = wind.disa_y + 1; line < wind.disa_y + wind.disa_h; line++, eaddr += 2) {
		bool nomem = false;

		if (!EmuRunning) {
			instr = 0;
			nomem = true;
		} else {
			instr = memRead16(eaddr);
		}

		n = cpu_disa_line(line, instr, eaddr, nomem);
		if (n > 1) disa_sub_h += n - 1;
		line += n - 1;
	}
	memQuietMode(false);
}

// ---
// general

void db_disa_reset(void) {
	disa_sub_h = 0;
}

bool db_disa_cur_visible(void) {
	u32 limit;
	limit = wind.text + (wind.disa_h - 1) * 2;
	return((wind.disa_cursor < limit) && (wind.disa_cursor >= wind.text));
}

void db_set_disa_cur(u32 addr) {
	wind.disa_cursor = addr & (~1);
	if ((wind.disa_cursor >= (wind.text + (wind.disa_h - 1) * 2)) || (wind.disa_cursor < wind.text))
		wind.text = wind.disa_cursor - (wind.disa_h & (~1));
	db_update(DB_UPDATE_DISA);
}

void db_disa_key(char ch, int ctrl) {
	disa_sub_h = 0;

	if (ctrl & CK_HOME) {
		db_set_disa_cur(sh4.pc);
	} else if (ctrl & CK_END) {
	} else if (ctrl & CK_UP) {
		if (ctrl & CK_CTRL) {
			if (wind.disa_h > 4) {
				wind.disa_h--;
				db_recalc_wnds();
			}
			db_update(DB_UPDATE_DISA);
			return;
		}
		wind.disa_cursor -= 2;
		if (wind.disa_cursor < wind.text) wind.text -= 2;
	} else if (ctrl & CK_DOWN) {
		if (ctrl & CK_CTRL) {
			if (wind.roll_h > 10) {
				wind.disa_h++;
				db_recalc_wnds();
			}
			db_update(DB_UPDATE_DISA);
			return;
		}
		wind.disa_cursor += 2;
		if (wind.disa_cursor >= (wind.text + (wind.disa_h - disa_sub_h - 1) * 2)) wind.text += 2;
	} else if (ctrl & CK_PGUP) {
		wind.text -= 2 * wind.disa_h;
		if (!db_disa_cur_visible()) wind.disa_cursor = wind.text;
	} else if (ctrl & CK_PGDN) {
		wind.text += 2 * (wind.disa_h - disa_sub_h);
		if (!db_disa_cur_visible()) wind.disa_cursor = wind.text + (wind.disa_h - disa_sub_h - 2) * 2;
	}

	db_update(DB_UPDATE_DISA);
}

void db_update_disa_window(void) {
	ConFillRegion(wind.disa_y, 1, CC_BLACK CB_CYAN "-");
	if (wind.focus == WDISA) ConPrintfAt(0, wind.disa_y, CC_WHITE CB_CYAN "\x1f");
	ConPrintfAt(2, wind.disa_y, CC_BLACK CB_CYAN "F3");

	ConPrintfAt(6, wind.disa_y, CC_BLACK CB_CYAN " SH4");
	ConPrintfAt(65, wind.disa_y, CC_BLACK CB_CYAN " PC: %08X", sh4.pc);
	db_disa_cpu();
}

// ---------------------------------------------------------------------------
// Memory.

#define DB_RAM          pram
#define DB_RAMSIZE      (0x01000000)

static void hexbyte(u32 addr, char *buf) {
	u8 byte[32];

	if (EmuRunning == false) {
		buf[0] = buf[1] = '?';	// unmapped
		buf[2] = 0;
		return;
	}

	memQuietMode(true);
	byte[0] = (u8)memRead8(addr);
	memQuietMode(false);
	sprintf(buf, "%02X", byte[0]);
}

static void charbyte(u32 addr, char *buf) {
	u8 byte[32];

	if (EmuRunning == false) {
		buf[0] = '?';	// unmapped
		buf[1] = 0;
		return;
	}

	buf[0] = '.'; buf[1] = 0;
	memQuietMode(true);
	byte[0] = (u8)memRead8(addr);
	memQuietMode(false);
	if ((byte[0] >= 32) && (byte[0] <= 127)) sprintf(buf, "%c\0", byte[0]);
}

static void db_dump_cpu(void) {
	int row;
	for (row = 0; row < wind.data_h - 1; row++) {
		int col;
		char buf[16];

		ConPrintfAt(0, wind.data_y + row + 1, "%08X", wind.data + row * 16);

		for (col = 0; col < 8; col++) {
			hexbyte(wind.data + row * 16 + col, buf);
			ConPrintfAt(10 + col * 3, wind.data_y + row + 1, buf);
		}

		for (col = 0; col < 8; col++) {
			hexbyte(wind.data + row * 16 + col + 8, buf);
			ConPrintfAt(35 + col * 3, wind.data_y + row + 1, buf);
		}

		for (col = 0; col < 16; col++) {
			charbyte(wind.data + row * 16 + col, buf);
			ConPrintfAt(60 + col, wind.data_y + row + 1, buf);
		}
	}
}

void db_data_key(char ch, int ctrl) {
	long maxadr = DB_RAMSIZE - 1;

	if (ctrl & CK_HOME) {
		wind.data = 0;
	} else if (ctrl & CK_END) {
		wind.data = DB_RAMSIZE - ((wind.data_h - 1) * 16);
	} else if (ctrl & CK_PGDN) {
		wind.data += ((wind.data_h - 1) * 16);
	} else if (ctrl & CK_PGUP) {
		wind.data -= ((wind.data_h - 1) * 16);
	} else if (ctrl & CK_UP) {
		if (ctrl & CK_CTRL) {
			if (wind.data_h > 3) {
				wind.data_h--;
				db_recalc_wnds();
			}
		} else wind.data -= 16;
	} else if (ctrl & CK_DOWN) {
		if (ctrl & CK_CTRL) {
			if (wind.roll_h > 10) {
				wind.data_h++;
				db_recalc_wnds();
			}
		} else wind.data += 16;
	}

	db_update(DB_UPDATE_DATA);
}

void db_update_dump_window(void) {
	ConFillRegion(wind.data_y, 1, CC_BLACK CB_CYAN "-");
	if (wind.focus == WDATA) ConPrintfAt(0, wind.data_y, CC_WHITE CB_CYAN "\x1f");
	ConPrintfAt(2, wind.data_y, CC_BLACK CB_CYAN "F2");

	ConPrintfAt(6, wind.data_y, CC_BLACK CB_CYAN " Main memory");
	db_dump_cpu();
}

// ---------------------------------------------------------------------------
// Message output.

static void con_add_roller_line(char *txt, int err) {
	char line[0x1000], *ptr = txt;

	// insert error color
	if (err) {
		sprintf(line, CC_BRED "Break: %s", txt);
		ptr = line;
	}

	// roll console "roller" 1 line up
	roll.rollpos = db_wraproll(roll.rollpos, 1);
	strncpy(roll.data[roll.rollpos], ptr, DB_LINELEN - 1);
	db_update(DB_UPDATE_MSGS);
}

void Printf(char *txt, ...) {
	char buf[0x1000], *s, *p;
	va_list arg;

	if (debug_active == false) return;

	sprintf(buf, CC_NORM);
	va_start(arg, txt);
	vsprintf(buf + strlen(CC_NORM), txt, arg);
	va_end(arg);
	buf[strlen(buf) + 1] = 0;

	p = buf;
	while (*p) {
		while (*p && *p == '\n') {
			con_add_roller_line("", 0);
			p++;
		}

		s = p;
		while (*p && *p != '\n') p++;
		*p = 0;

		if (*s) con_add_roller_line(s, 0);
		p++;
	}
}

// ---
// helpers

int db_wraproll(int roll, int value) {
	roll += value;
	if (roll >= DB_LINES) return roll - DB_LINES;
	else if (roll < 0) return roll + DB_LINES;
	else return roll;
}

void db_recalc_wnds() {
	wind.regs_y = 0;//+4;

	if (wind.visible & DB_UPDATE_REGS)
		wind.data_y = wind.regs_y + wind.regs_h;
	else
		wind.data_y = wind.regs_y;

	if (wind.visible & DB_UPDATE_DATA)
		wind.disa_y = wind.data_y + wind.data_h;
	else
		wind.disa_y = wind.data_y;

	if (wind.visible & DB_UPDATE_DISA)
		wind.roll_y = wind.disa_y + wind.disa_h;
	else
		wind.roll_y = wind.disa_y;

	wind.roll_h = DB_HEIGHT - wind.roll_y - 2;	// - statusline - editline
	wind.stat_h = wind.edit_h = 1;
	wind.edit_y = wind.roll_y + wind.roll_h;
	wind.stat_y = wind.edit_y + 1;

	ConFillRegion(0, DB_HEIGHT, CB_BLACK " ");
	db_update(DB_UPDATE_ALL);
}

void db_status(char *txt) {
	sprintf(roll.statusline, CB_CYAN CC_BLACK " %s\n", txt);
	db_update(DB_UPDATE_STAT);
}

void db_set_autoscroll(bool value) {
	roll.autoscroll = value;
	if (value) db_status("Ready. Press PgUp to look behind.");
	else db_status("Scroll Mode - Press PgUp, PgDown, Up, Down to scroll. Escape to back.");
}

void db_change_focus(FOCUSWND newfocus) {
	FOCUSWND oldfocus = wind.focus;

	if (oldfocus == newfocus)
		return;	// we dont need any repaint

	wind.focus = newfocus;

	switch (newfocus) {	// switch focus to?
	case WREGS:     db_update(DB_UPDATE_REGS); break;
	case WDATA:     db_update(DB_UPDATE_DATA);
		db_status("Memory view. Press Up, Down, PgUp, PgDown to scroll."); break;
	case WDISA:     db_update(DB_UPDATE_DISA);
		db_status("Disassembly view. Press Up, Down, PgUp, PgDown to scroll."); break;
	case WCONSOLE:  db_update(DB_UPDATE_MSGS);
		db_set_autoscroll(roll.autoscroll); break;
	}
}

// ---
// roller window

static void update_scroll_window() {
	int i, y, back, line;

	// cleanup window and skip header line
	ConFillRegion(wind.roll_y + 1, wind.roll_h - 1, " ");
	ConFillRegion(wind.roll_y, 1, CB_CYAN CC_BLACK "-");
	if (wind.focus == WCONSOLE) ConPrintfAt(0, wind.roll_y, CB_CYAN CC_WHITE "\x1f");
	ConPrintfAt(2, wind.roll_y, CB_CYAN CC_BLACK "F4");
	ConPrintfAt(6, wind.roll_y, CB_CYAN CC_BLACK " Console output");

	// printing coord in buffer (skip header)
	y = wind.roll_y + 1;

	// shift backward
	back = wind.roll_h - 1;

	// where to get buffer line
	line = (roll.autoscroll) ? (roll.rollpos - back) : (roll.viewpos - back);
	line += 1;
	for (i = 1; i < wind.roll_h; i++) {
		if (line >= 0) ConPrintfAt(0, y, roll.data[line]);
		line++;
		y++;
	}
}

// ---
// update processing

void db_update(u32 mask) {
	wind.update |= mask;
}

void db_refresh(void) {
	// registers
	if (wind.update & DB_UPDATE_REGS) {
		db_update_registers();
		ConUpdateRegion(wind.regs_y, wind.regs_h);
	}

	// data dump window
	if (wind.update & DB_UPDATE_DATA) {
		db_update_dump_window();
		ConUpdateRegion(wind.data_y, wind.data_h);
	}

	// disassembler window
	if (wind.update & DB_UPDATE_DISA) {
		db_update_disa_window();
		ConUpdateRegion(wind.disa_y, wind.disa_h);
	}

	// message history
	if (wind.update & DB_UPDATE_MSGS) {
		update_scroll_window();
		ConUpdateRegion(wind.roll_y, wind.roll_h);
	}

	// edit line
	if (wind.update & DB_UPDATE_EDIT) {
		ConFillRegion(wind.edit_y, 1, " ");
		ConPrintfAt(0, wind.edit_y, "> ");
		ConPrintfAt(2, wind.edit_y, roll.editline);
		ConUpdateRegion(wind.edit_y, 1);
		ConSetCursor(roll.editpos + 2, wind.edit_y);
	}

	// status line
	if (wind.update & DB_UPDATE_STAT) {
		ConFillRegion(wind.stat_y, 1, CB_CYAN " ");
		ConPrintfAt(1, wind.stat_y, roll.statusline);
		ConUpdateRegion(wind.stat_y, 1);
	}

	wind.update = 0;
}

// ---------------------------------------------------------------------------
// Console input.

// Sub-windows input

static void function_key(int ctrl) {
	if (ctrl & CK_F(1)) {
		if (wind.show_stats == false)
			db_update(DB_UPDATE_REGS);
	} else if (ctrl & CK_F(2))
		db_change_focus(WDATA);		// Data
	else if (ctrl & CK_F(3))
		db_change_focus(WDISA);		// Disa
	else if (ctrl & CK_F(4))
		db_change_focus(WCONSOLE);
	else if (ctrl & CK_F(6)) {		// Switch regs
		if (wind.show_stats) return;
		wind.regmode = (REGWNDMODE)(((int)wind.regmode + 1) % REGMOD_MAX);
		ConFillRegion(wind.regs_y, wind.regs_h, CB_BLACK " ");
		db_update(DB_UPDATE_REGS);
	}
	//----
	// runtime controls
	else if (ctrl & CK_F(5)) {		// toggle Go / Break (ALL)
		if (debug_step_mode)
			db_command("run");
		else
			db_command("pause");
	} else if (ctrl & CK_F(8)) {	// Set pc at cursor
		db_command("pc");
	} else if (ctrl & CK_F(9)) {	// Set breakpoint at cursor
		u32 idx;
		idx = db_search_breakpoint(BP_EXECUTE, wind.disa_cursor, 2);
		if (!idx)
			db_add_breakpoint(BP_EXECUTE, wind.disa_cursor, 0);
		else
			db_clear_breakpoint(idx);
		db_update(DB_UPDATE_DISA);
	}
//    else if(ctrl & CK_F(10))        // Step Over
//    {
//        db_command("thru");
//    }
	else if (ctrl & CK_F(11)) {		// Step In
		db_command("step");
	} else if (ctrl & CK_F(12)) {	// Skip
		db_command("skip");
	}
}

// ---
// Roller

// searching for beginning of each word
static void search_left() {
	roll.editlen = (int)strlen(roll.editline);

	// check editpos!=0 for possibility check [editpos-1]
	if ((roll.editlen == 0) || (roll.editpos == 0)) return;

	// we at beginning of the word? if so, move to the space
	if ((roll.editline[roll.editpos] != 0x20) && (roll.editline[roll.editpos - 1] == 0x20)) roll.editpos--;

	// note: no need check editpos!=0 here
	if (roll.editline[roll.editpos] == 0x20) {	// are we at space?
												//search for first non-space
		while ((roll.editpos != 0) && (roll.editline[roll.editpos] == 0x20)) roll.editpos--;
		if (!roll.editpos) return;
	}

	// we at last char, search for first space now
	while (roll.editpos != 0) {
		// sure, editpos!=0  here, coz while check it first
		if (roll.editline[roll.editpos - 1] == 0x20) return;
		roll.editpos--;
	}
}

static void search_right() {
	roll.editlen = (int)strlen(roll.editline);
	if ((roll.editlen == 0) || (roll.editpos == roll.editlen)) return;
	if (roll.editline[roll.editpos] != 0x20) {
		while (roll.editline[roll.editpos] != 0x20) {
			if (roll.editpos == roll.editlen) return;
			roll.editpos++;
		}
	}

	while ((roll.editpos != roll.editlen) && (roll.editline[roll.editpos] == 0x20)) roll.editpos++;
}

static int testempty(char *str) {
	int i, len = (int)strlen(str);

	for (i = 0; i < len; i++) {
		if (str[i] != 0x20) return 0;
	}

	return 1;
}

static void roll_edit_key(char ascii, int ctrl) {
	if (ascii != CK_CONTROL) {
		roll.editlen = (int)strlen(roll.editline);
		if (roll.editlen >= 77) return;
		else {
			if (roll.editpos == roll.editlen) {
				roll.editline[roll.editpos++] = ascii;
				roll.editline[roll.editpos] = 0;
			} else {
				memmove(
					roll.editline + roll.editpos + 1,
					roll.editline + roll.editpos,
					roll.editlen - roll.editpos + 1
					);
				roll.editline[roll.editpos++] = ascii;
			}

			db_update(DB_UPDATE_EDIT);
		}
	} else {
		if (ctrl & CK_RETURN) {
			roll.editlen = (int)strlen(roll.editline);
			if (!roll.editlen) return;
			if (testempty(roll.editline)) return;
			strcpy(roll.history[roll.historypos], roll.editline);
			roll.historypos++;
			if (roll.historypos > 255) roll.historypos = 0;
			roll.historycur = roll.historypos;
			Printf(": %s", roll.editline);
			db_command(roll.editline);
			roll.editpos = roll.editlen = roll.editline[0] = 0;
			db_update(DB_UPDATE_EDIT | DB_UPDATE_MSGS);
		} else if (ctrl & CK_UP) {
			if (!roll.autoscroll) {
				roll.viewpos--;
				if (roll.viewpos < 0) roll.viewpos = 0;
				db_update(DB_UPDATE_MSGS);
			} else {
				if (--roll.historycur < 0) roll.historycur = 0;
				else {
					strcpy(roll.editline, roll.history[roll.historycur]);
					roll.editpos = roll.editlen = (int)strlen(roll.editline);
					db_update(DB_UPDATE_EDIT);
				}
			}
		} else if (ctrl & CK_DOWN) {
			if (!roll.autoscroll) {
				roll.viewpos++;
				if (roll.viewpos >= roll.rollpos) {
					roll.viewpos = roll.rollpos;
					db_set_autoscroll(true);
				}
				db_update(DB_UPDATE_MSGS);
			} else {
				if (++roll.historycur >= roll.historypos) {
					roll.historycur = roll.historypos;
					roll.editpos = roll.editlen = roll.editline[0] = 0;
					db_update(DB_UPDATE_EDIT);
				} else {
					strcpy(roll.editline, roll.history[roll.historycur]);
					roll.editpos = roll.editlen = (int)strlen(roll.editline);
					db_update(DB_UPDATE_EDIT);
				}
			}
		} else if (ctrl & CK_HOME) {
			if (!roll.autoscroll) {
				roll.viewpos = 0;
				db_update(DB_UPDATE_MSGS);
			} else {
				roll.editpos = 0;
				db_update(DB_UPDATE_EDIT);
			}
		} else if (ctrl & CK_END) {
			if (!roll.autoscroll) {
				roll.viewpos = roll.rollpos;
				db_update(DB_UPDATE_MSGS);
			} else {
				roll.editpos = (int)strlen(roll.editline);
				db_update(DB_UPDATE_EDIT);
			}
		} else if (ctrl & CK_LEFT) {
			if (ctrl & CK_CTRL) search_left();
			else if (roll.editpos) roll.editpos--;
			db_update(DB_UPDATE_EDIT);
		} else if (ctrl & CK_RIGHT) {
			roll.editlen = (int)strlen(roll.editline);
			if (roll.editlen && (roll.editpos != roll.editlen)) {
				if (ctrl & CK_CTRL) search_right();
				else if (roll.editpos < roll.editlen) roll.editpos++;
				db_update(DB_UPDATE_EDIT);
			}
		} else if (ctrl & CK_BKSPACE) {
			if (roll.editpos) {
				roll.editlen = (int)strlen(roll.editline);
				if (roll.editlen == roll.editpos) roll.editline[--roll.editpos] = 0;
				else memmove(
						roll.editline + roll.editpos - 1,
						roll.editline + roll.editpos,
						roll.editlen - roll.editpos + 1
						), roll.editpos--;
				db_update(DB_UPDATE_EDIT);
			}
		} else if (ctrl & CK_DEL) {
			if (roll.editpos) {
				roll.editlen = (int)strlen(roll.editline);
				if (roll.editlen != roll.editpos) {
					memmove(
						roll.editline + roll.editpos,
						roll.editline + roll.editpos + 1,
						roll.editlen - roll.editpos - 1);
					roll.editline[roll.editlen - 1] = '\0';
					db_update(DB_UPDATE_EDIT);
				}
			}
		} else if (ctrl & CK_ESC) {
			if (!roll.autoscroll) {
				db_set_autoscroll(true);
				db_update(DB_UPDATE_MSGS);
			} else {
				roll.historycur = roll.historypos;
				roll.editpos = roll.editlen = roll.editline[0] = 0;
				db_update(DB_UPDATE_EDIT);
			}
		} else if (ctrl & CK_PGUP) {
			if (roll.autoscroll) {
				db_set_autoscroll(false);
				roll.viewpos = roll.rollpos;
			}

			roll.viewpos -= 8;
			if (roll.viewpos < 0) roll.viewpos = 0;
			db_update(DB_UPDATE_MSGS);
		} else if (ctrl & CK_PGDN) {
			if (roll.autoscroll) {
				db_set_autoscroll(false);
				roll.viewpos = roll.rollpos;
			}

			roll.viewpos += 8;
			if (roll.viewpos >= roll.rollpos) {
				roll.viewpos = roll.rollpos;
				db_set_autoscroll(true);
			}
			db_update(DB_UPDATE_MSGS);
		}
	}
}

// ---

static void virtual_key(char ascii, int ctrl) {
	// functions F1..F12 and control
	if ((ascii == CK_CONTROL) && (ctrl & CK_FKEYS)) {
		function_key(ctrl);
		return;
	}

	// switch focus to input line if symbol key
	if ((ctrl & (CK_BKSPACE | 0)) || (ascii >= 0x20 && ascii < 256)) {
		db_change_focus(WCONSOLE);
	}

	switch (wind.focus) {
	case WREGS:
		break;
	case WDATA:
		db_data_key(ascii, ctrl);
		break;
	case WDISA:
		db_disa_key(ascii, ctrl);
		break;
	case WCONSOLE:
		roll_edit_key(ascii, ctrl);
		break;
	}
}

void db_keep_updated(void) {
	int ctrl;
	while (debug_active) {
		char ascii = ConPeekInput(&ctrl);
		__asm nop
		__asm nop
		if (ascii != CK_NULL) virtual_key(ascii, ctrl);
		__asm nop
		__asm nop
		db_refresh();
		__asm nop
		__asm nop
	}
}

// ---------------------------------------------------------------------------
// Hooks and breakponts

typedef struct {
	u32 number;
	u32 cond;
	u32 eaddr;
	u32 range;
	void *nextbreak;
	void *prevbreak;
} BREAKPOINT;

static BREAKPOINT *first_breakpoint = NULL;
static BREAKPOINT *last_breakpoint = NULL;
static u32 idx, breakpoint_count;

u32 FASTCALL db_search_breakpoint(u32 cond, u32 eaddr, u32 align) {
	if ((first_breakpoint != NULL) && cond) {
		BREAKPOINT *tmpbp = first_breakpoint;
		u32 mask = ~(align - 1);
		while (tmpbp->nextbreak != NULL) {
			if (tmpbp->cond & cond)
				if (tmpbp->range) {
					if ((tmpbp->eaddr <= eaddr) && (eaddr <= tmpbp->range)) return tmpbp->number;
				} else
				if ((tmpbp->eaddr & mask) == eaddr) return tmpbp->number;
			tmpbp = tmpbp->nextbreak;
		}
		if (tmpbp->cond & cond)
			if (tmpbp->range) {
				if ((tmpbp->eaddr <= eaddr) && (eaddr <= tmpbp->range)) return tmpbp->number;
			} else
			if ((tmpbp->eaddr & mask) == eaddr) return tmpbp->number;
	}
	return 0;
}

void db_add_breakpoint(u32 cond, u32 eaddr, u32 range) {
	if (first_breakpoint == NULL) {
		breakpoint_count = 1;
		first_breakpoint = malloc(sizeof(BREAKPOINT));
		first_breakpoint->number = breakpoint_count;
		first_breakpoint->cond = cond;
		first_breakpoint->eaddr = eaddr;
		first_breakpoint->range = range;
		first_breakpoint->prevbreak = first_breakpoint;
		first_breakpoint->nextbreak = NULL;
		last_breakpoint = first_breakpoint;
	} else {
		BREAKPOINT *newbp = malloc(sizeof(BREAKPOINT));
		newbp->number = ++breakpoint_count;
		newbp->cond = cond;
		newbp->eaddr = eaddr;
		newbp->range = range;
		newbp->prevbreak = last_breakpoint;
		newbp->nextbreak = NULL;
		last_breakpoint->nextbreak = newbp;
		last_breakpoint = newbp;
	}
}

void db_clear_breakpoints(void) {
	BREAKPOINT *tmpbp = last_breakpoint;
	if (tmpbp == NULL) return;
	while (tmpbp->prevbreak != tmpbp) {
		tmpbp = tmpbp->prevbreak;
		free(tmpbp->nextbreak);
	}
	free(tmpbp);
	first_breakpoint = NULL;
	last_breakpoint = NULL;
}

void db_clear_breakpoint(u32 number) {
	BREAKPOINT *tmpbp = first_breakpoint;
	if (tmpbp == NULL) return;
	if (last_breakpoint == first_breakpoint) {
		free(first_breakpoint);
		free(last_breakpoint);
		first_breakpoint = NULL;
		last_breakpoint = NULL;
		return;
	}
	if (tmpbp->number == number) {
		first_breakpoint = first_breakpoint->nextbreak;
		first_breakpoint->prevbreak = first_breakpoint;
		free(tmpbp);
		return;
	}
	while (tmpbp->nextbreak != NULL) {
		if (tmpbp->number == number) {
			BREAKPOINT *next, *prev;
			prev = tmpbp->prevbreak;
			prev->nextbreak = tmpbp->nextbreak;
			next = tmpbp->nextbreak;
			next->prevbreak = tmpbp->prevbreak;
			free(tmpbp);
			return;
		}
		tmpbp = tmpbp->nextbreak;
	}
	if (tmpbp->number == number) {
		last_breakpoint = last_breakpoint->prevbreak;
		last_breakpoint->nextbreak = NULL;
		free(tmpbp);
	}
}

void db_list_breakpoints(void) {
	if (first_breakpoint != NULL) {
		BREAKPOINT *tmpbp = first_breakpoint;
		while (tmpbp->nextbreak != NULL) {
			if (tmpbp->range)
				Printf(CC_CYAN "%3d" CC_WHITE " > %s%s%s %08X %08X\n",
					   tmpbp->number,
					   (tmpbp->cond & BP_READ) ? "r" : "-",
					   (tmpbp->cond & BP_WRITE) ? "w" : "-",
					   (tmpbp->cond & BP_EXECUTE) ? "x" : "-",
					   tmpbp->eaddr, tmpbp->range
					   );
			else
				Printf(CC_CYAN "%3d" CC_WHITE " > %s%s%s %08X\n",
					   tmpbp->number,
					   (tmpbp->cond & BP_READ) ? "r" : "-",
					   (tmpbp->cond & BP_WRITE) ? "w" : "-",
					   (tmpbp->cond & BP_EXECUTE) ? "x" : "-",
					   tmpbp->eaddr
					   );
			tmpbp = tmpbp->nextbreak;
		}
		if (tmpbp->range)
			Printf(CC_CYAN "%3d" CC_WHITE " > %s%s%s %08X %08X\n",
				   tmpbp->number,
				   (tmpbp->cond & BP_READ) ? "r" : "-",
				   (tmpbp->cond & BP_WRITE) ? "w" : "-",
				   (tmpbp->cond & BP_EXECUTE) ? "x" : "-",
				   tmpbp->eaddr, tmpbp->range
				   );
		else
			Printf(CC_CYAN "%3d" CC_WHITE " > %s%s%s %08X\n",
				   tmpbp->number,
				   (tmpbp->cond & BP_READ) ? "r" : "-",
				   (tmpbp->cond & BP_WRITE) ? "w" : "-",
				   (tmpbp->cond & BP_EXECUTE) ? "x" : "-",
				   tmpbp->eaddr
				   );
	} else
		Printf(CC_CYAN "Breakpoint list is empty!\n");
}

void db_load_breakpoints() {
/*	IniFile iniFile;
    if (!IniFile_open(&iniFile, DEMUL_NAME)) return false;
    if(IniFile_exist(&iniFile))
    {
        u32 i;
        breakpoint_count = IniFile_getLong(&iniFile, "debugger", "bpcount");
        for(i=0; i<breakpoint_count; i++)
        {
            u32 cond, eaddr;
            char bpname[16];
            sprintf(bpname,"bp%dcond",i);
            cond = IniFile_getLong(&iniFile, "debugger", bpname);
            sprintf(bpname,"bp%deaddr",i);
            eaddr = IniFile_getLong(&iniFile, "debugger", bpname);
            db_add_breakpoint(cond, eaddr);
        }
    }*/
}

void db_save_breakpoints(void) {
/*	IniFile iniFile;
    if (!IniFile_open(&iniFile, DEMUL_NAME)) return false;
    if(IniFile_exist(&iniFile))
    {
        if(first_breakpoint != NULL)
        {
            char bpname[16];
            BREAKPOINT *tmpbp = first_breakpoint;
            breakpoint_count = 1;
            while(tmpbp->nextbreak != NULL)
            {
                sprintf(bpname,"bp%dcond",breakpoint_count);
                IniFile_setLong(&iniFile, "debugger", bpname, tmpbp->cond);
                sprintf(bpname,"bp%deaddr",breakpoint_count);
                IniFile_setLong(&iniFile, "debugger", bpname, tmpbp->eaddr);
                tmpbp = tmpbp->nextbreak;
                breakpoint_count++;
            }
            sprintf(bpname,"bp%dcond",breakpoint_count);
            IniFile_setLong(&iniFile, "debugger", bpname, tmpbp->cond);
            sprintf(bpname,"bp%deaddr",breakpoint_count);
            IniFile_setLong(&iniFile, "debugger", bpname, tmpbp->eaddr);
            IniFile_setLong(&iniFile, "debugger", "bpcount", breakpoint_count);
        }
    }*/
}

u32 FASTCALL hookDebugExecute(u32 pc) {
	debug_new_pc = pc;
	if (debug_active) {
		if (debug_trace_mode) {
			db_set_disa_cur(pc);
			db_update(DB_UPDATE_ALL);
			debug_trace_mode = 0;
		}
		while (1) {
			__asm nop			// ms optimization must die!!!
			__asm nop
			if ((!debug_step_mode) || (debug_trace_mode)) {
				__asm nop		// ms optimization must die!!!
				__asm nop
				if ((!debug_run_after_break) && (idx = db_search_breakpoint(BP_EXECUTE, pc, 2))) {
					__asm nop	// ms optimization must die!!!
					__asm nop
				  debug_step_mode = 1;
					debug_run_after_break = 1;
					db_set_disa_cur(pc);
					db_update(DB_UPDATE_ALL);
					Printf("Breakpoint #%d reached (execute)!", idx);
					__asm nop	// ms optimization must die!!!
					__asm nop
				} else break;
				__asm nop		// ms optimization must die!!!
				__asm nop
			}
			__asm nop
			__asm nop
		}
	}
	debug_run_after_break = 0;
	return debug_new_pc;
}

void FASTCALL hookDebugRead(u32 mem, u32 align) {
	if (first_breakpoint != NULL)
		if (idx = db_search_breakpoint(BP_READ, mem, align)) {
			debug_step_mode = 1;
			db_set_disa_cur(sh4.pc);
			db_update(DB_UPDATE_ALL);
			Printf("Breakpoint #%d reached (memory read)!", idx);
		}
}

void FASTCALL hookDebugWrite(u32 mem, u32 align) {
	if (first_breakpoint != NULL)
		if (idx = db_search_breakpoint(BP_WRITE, mem, align)) {
			debug_step_mode = 1;
			db_set_disa_cur(sh4.pc);
			db_update(DB_UPDATE_ALL);
			Printf("Breakpoint #%d reached (memory write)!", idx);
		}
}

void FASTCALL hookDebugException(void) {
	if (debug_exception_mode) {
		debug_step_mode = 1;
		db_set_disa_cur(sh4.pc);
		db_update(DB_UPDATE_ALL);
		Printf("Processor exception!");
	}
}

// ---------------------------------------------------------------------------
// Command Processor.

static char tokens[DB_TOKENS][DB_LINELEN];		// tokens parsed from command line
static int tokencount;							// parsed tokens count

// command record
typedef struct DebugCmd {
	char    *name;				// command
	void (*callback)(void);		// call after '%cmd% [arguments]', where arguments are placed
								// in tokens array (see above).
	int need_args;				// need at least N arguments
	char    *helptext;			// show after 'help %cmd%'
} DebugCmd;

// detect expression
static u32 db_convert_expression(char *str) {
	u32 n, len = strlen(str);
	strlwr(str);
	if ((str[0] == 'r')) {
		n = strtoul(&str[1], NULL, 10);
		if (str[len - 1] != 'b')
			return sh4.r[(n & 15)];
		else
			return sh4.r[(n & 7) + 16];
	} else if ((str[0] == 'p') && (str[1] == 'r')) {
		return sh4.pr;
	}
	return strtoul(str, NULL, 16);
}

#include "cmd.c"

// tokenize line into arguments

#define endl    (line[p] == 0)
#define space   (line[p] == 0x20)
#define quot    (line[p] == '\'')
#define dquot   (line[p] == '\"')

static bool db_tokenize(char *line) {
	int p, start, end;
	p = tokencount = start = end = 0;
	memset(tokens, 0, sizeof(tokens));

	// while not end line
	while (!endl) {
		// skip space first, if any
		while (space) p++;
		if (!endl && (quot || dquot)) {	// quotation, need special case
			p++;
			start = p;
			while (1) {
				if (endl) {
					Printf(CC_BRED "Open quotation.\n");
					return false;
				}

				if (quot || dquot) {
					end = p;
					p++;
					break;
				} else p++;
			}

			if (tokencount >= DB_TOKENS) {
				Printf(CC_BRED "Too many arguments (>%i) or need quotes.\n", DB_TOKENS);
				return false;
			}

			strncpy(tokens[tokencount], line + start, end - start);

			// make lower only first token
			if (tokencount == 0) strlwr(tokens[tokencount]);

			tokencount++;
		} else if (!endl) {
			start = p;
			while (1) {
				if (endl || space || quot || dquot) {
					end = p;
					break;
				}

				p++;
			}

			if (tokencount >= DB_TOKENS) {
				Printf(CC_BRED "Too many arguments (>%i) or need quotes.\n", DB_TOKENS);
				return false;
			}

			strncpy(tokens[tokencount++], line + start, end - start);
		}
	}

	return true;
}

// execute command
void db_command(char *line) {
	int i;
	bool ok = db_tokenize(line);	// depart command line to arguments.
	if (!ok) return;

	// look for command in command' list
	for (i = 0; i < cmd_num; i++) {
		if (!stricmp(tokens[0], cmd_list[i].name)) {
			if (cmd_list[i].need_args > (tokencount - 1)) {
				Printf(CC_GREEN "Command need at least %i argument(s). Type 'help %s' for more info.",
					   cmd_list[i].need_args, cmd_list[i].name);
				return;
			} else {
				cmd_list[i].callback();
				return;
			}
		}
	}

	Printf(CC_GREEN "Unknown command '%s', try 'help'.", tokens[0]);
}

// ---------------------------------------------------------------------------

void openDebug(void) {
	debug_active = openConsole();
	if (debug_active == false) return;

	debug_thread = (HANDLE)_beginthread(db_keep_updated, 0, NULL);
	if (debug_thread == INVALID_HANDLE_VALUE) return;

//	db_load_breakpoints();

	// Setup debugger windows layout
	memset(&wind, 0, sizeof(wind));
	memset(&roll, 0, sizeof(roll));

	wind.visible = DB_UPDATE_ALL;		// sub-windows
	wind.focus = WCONSOLE;
	wind.regs_h = 17;
	wind.data_h = 8;
	wind.disa_h = 19;	// 16
	db_recalc_wnds();

	db_set_autoscroll(true);			// roller
	roll.rollpos = db_wraproll(0, -1);

	wind.data = 0xA0000000;
	db_set_disa_cur(0xA0000000);
	db_disa_reset();
	db_memorize_cpu_regs();

	// show emulator version and copyright
	#define BGR CB_GRAY
	Printf(
		BGR CC_CYAN CC_WHITE "This software comes with " CC_WHITE CB_BBLUE "ABSOLUTELY NO WARRANTY!!" BGR "                                       \n"
		BGR CC_CYAN CC_WHITE "You shouldnt mind if it'll fuck your system. I dont mind too.                        \n"
		BGR CC_CYAN  "                                                                                              \n"
		BGR CC_CYAN CC_WHITE "Type " CC_WHITE CB_BBLUE "help" CC_WHITE BGR " to get first instructions. Type " CC_WHITE CB_BBLUE "help cmdname" CC_WHITE BGR " to get more                           \n"
		BGR CC_CYAN CC_WHITE BGR "detailed information about commands.                                               \n\n");

	db_update(DB_UPDATE_ALL);
}

void closeDebug(void) {
	if (debug_thread != INVALID_HANDLE_VALUE) {
		TerminateThread(debug_thread, 1);
		CloseHandle(debug_thread);
	}
//	db_save_breakpoints();
	db_clear_breakpoints();
	closeConsole();
	Sleep(500);
	debug_active = false;
}

#else	// Stubs.

void Printf(char *format, ...) {
	FILE *ofile;
	char temp[2048];
	va_list ap;
	va_start(ap, format);
	vsprintf(temp, format, ap);
	ofile = fopen("stdout.txt", "ab");
	fwrite(temp, 1, strlen(temp), ofile);
	fclose(ofile);
	va_end(ap);
}

#endif	// DEMUL_DEBUG

void PrintfLog(char *format, ...) {
	FILE *ofile;
	char temp[2048];
	va_list ap;
	va_start(ap, format);
	vsprintf(temp, format, ap);
	ofile = fopen("stdout.txt", "ab");
	fwrite(temp, 1, strlen(temp), ofile);
	fclose(ofile);
	va_end(ap);
}

#ifdef STAT_ENABLE

#define STAT_COL 8
#define STAT_ROW 65536 / STAT_COL

static LONGLONG statOpcodes[0x10000];
static u16 statOpcodesCycle[0x10000];

void statFlush() {
	int i, j;
	for (i = 0; i < STAT_ROW; i++) {
		for (j = 0; j < STAT_COL; j++)
			PrintfLog("%08x ", statOpcodes[i * STAT_COL + j]);
		PrintfLog("\n");
	}
	memset(statOpcodes, 0, 0x10000);
}

void statUpdate(u16 code) {
	code = db_mask_opcode(code);
	ProfileStart(code);
}
void statFinalize(void) {
	ProfileFinish();
}


static u16 codeStat;
static LARGE_INTEGER codeTime1;
static LARGE_INTEGER codeTime2;

void ProfileStart(u16 cs) {
	QueryPerformanceCounter(&codeTime1);
	codeStat = cs;
}

void ProfileFinish(void) {
	LONGLONG d;
	QueryPerformanceCounter(&codeTime2);
	d = codeTime2.QuadPart - codeTime1.QuadPart;
	if (statOpcodesCycle[codeStat] < 1) {
		statOpcodesCycle[codeStat]++;
		statOpcodes[codeStat] += d;
	} else {
		statOpcodesCycle[codeStat]++;
		if (statOpcodesCycle[codeStat] < 65535) {
			statOpcodesCycle[codeStat]++;
			statOpcodes[codeStat] = ((statOpcodes[codeStat] * (statOpcodesCycle[codeStat] - 1)) + d) / statOpcodesCycle[codeStat];
		}
	}
//	Printf("profile[%d] %d\n",n, d);
}
#endif
