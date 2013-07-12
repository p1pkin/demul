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

// Console commands. EXCLUDE THIS FILE FROM THE BUILD!

DebugCmd cmd_list[];
int cmd_num;

// ---------------------------------------------------------------------------
// .

static void cmd_dot(void) {
	db_set_disa_cur(sh4.pc);
	db_update(DB_UPDATE_DISA);
}

// ---------------------------------------------------------------------------
// db

static void cmd_db(void) {
	if (tokencount <= 1) {
		Printf("Current memory view address: 0x%08X\n", wind.data);
		return;
	}
	wind.data = db_convert_expression(tokens[1]);
	db_update(DB_UPDATE_DATA);
}

// ---------------------------------------------------------------------------
// u

static void cmd_u(void) {
	if (tokencount <= 1) {
		Printf("Current CPU view address: 0x%08X\n", wind.disa_cursor);
		return;
	}
	db_set_disa_cur(db_convert_expression(tokens[1]));
	db_update(DB_UPDATE_DISA);
}

// ---------------------------------------------------------------------------
// pc

static void cmd_pc(void) {
	if (tokencount > 1)
		sh4.pc = db_convert_expression(tokens[1]);
	else
		sh4.pc = wind.disa_cursor;
	debug_new_pc = sh4.pc;
	db_set_disa_cur(sh4.pc);
	db_update(DB_UPDATE_DISA);
}

// ---------------------------------------------------------------------------
// bp

static void cmd_bp(void) {
	char cur;
	u32 idx = 0, cond = 0, expr;
	strlwr(tokens[1]);
	while ((cur = tokens[1][idx++]) != 0) {
		switch (cur) {
		case 'r': cond |= BP_READ; break;
		case 'w': cond |= BP_WRITE; break;
		case 'x': cond |= BP_EXECUTE; break;
		}
	}
	if (cond) {
		expr = db_convert_expression(tokens[2]);
		if (!db_search_breakpoint(cond, expr, 1))
			db_add_breakpoint(cond, expr, 0);
		else
			Printf(CC_CYAN "Breakpoint already exists!\n");
	} else
		Printf(CC_CYAN "Wrong break conditions!\n");
}

// ---------------------------------------------------------------------------
// bpr

static void cmd_bpr(void) {
	char cur;
	u32 idx = 0, cond = 0, expr, range;
	strlwr(tokens[1]);
	while ((cur = tokens[1][idx++]) != 0) {
		switch (cur) {
		case 'r': cond |= BP_READ; break;
		case 'w': cond |= BP_WRITE; break;
		case 'x': cond |= BP_EXECUTE; break;
		}
	}
	if (cond) {
		expr = db_convert_expression(tokens[2]);
		range = db_convert_expression(tokens[3]);
		if (range < expr) {
			u32 temp = range;
			range = expr;
			expr = temp;
		}
		if (!db_search_breakpoint(cond, expr, 1))
			db_add_breakpoint(cond, expr, range);
		else
			Printf(CC_CYAN "Breakpoint already exists!\n");
	} else
		Printf(CC_CYAN "Wrong break conditions!\n");
}

// ---------------------------------------------------------------------------
// bc

static void cmd_bc(void) {
	char *tok = strlwr(tokens[1]);
	if (tok[0] == '*')
		db_clear_breakpoints();
	else
		db_clear_breakpoint(strtoul(tok, NULL, 16));
	db_update(DB_UPDATE_DISA);
}

// ---------------------------------------------------------------------------
// bl

static void cmd_bl(void) {
	db_list_breakpoints();
}

// ---------------------------------------------------------------------------
// run

static void cmd_run(void) {
	debug_step_mode = 0;
}

// ---------------------------------------------------------------------------
// pause

static void cmd_pause(void) {
	debug_step_mode = 1;
	db_set_disa_cur(sh4.pc);
	db_update(DB_UPDATE_ALL);
}

// ---------------------------------------------------------------------------
// step in

static void cmd_step(void) {
	debug_trace_mode = 1;
}

// ---------------------------------------------------------------------------
// help

char *help_text =
	CC_CYAN  "--- cpu debug commands --------------------------------------------------------\n"
	CC_WHITE "    .                    " CC_NORM "- view code at PC\n"
	CC_WHITE "    u [expression]       " CC_NORM "- view code at specified address\n"
	CC_WHITE "    db [expression]      " CC_NORM "- view data at specified address\n"
	CC_WHITE "    pc [expression]      " CC_NORM "- set program counter (F8 also)\n"
	CC_WHITE "    step                 " CC_NORM "- execute one instruction (F11 also)\n"
//     CC_WHITE "    thru                 " CC_NORM "- execute call (F10 also)\n"
	CC_WHITE "    pause                " CC_NORM "- toggle pause emulation (F5 also)\n"
	CC_WHITE "    run                  " CC_NORM "- toggle start emulation (F5 also)\n"
	CC_WHITE "    bp [rwx] [expression]" CC_NORM "- set breakpoint [r]ead/[w]rite/e[x]ecute at address\n"
	CC_WHITE "    bpr [rwx] [range]    " CC_NORM "- set breakpoint [r]ead/[w]rite/e[x]ecute at address range\n"
	CC_WHITE "    bc ##|*              " CC_NORM "- clear breakpoint\n"
	CC_WHITE "    bl                   " CC_NORM "- list breakpoints\n"
	"\n"
	CC_CYAN  "--- functional keys -----------------------------------------------------------\n"
	CC_WHITE "    F1                   " CC_NORM "- update registers instantly\n"
	CC_WHITE "    F2                   " CC_NORM "- switch to memory view\n"
	CC_WHITE "    F3                   " CC_NORM "- switch to disassembly\n"
	CC_WHITE "    F4                   " CC_NORM "- switch to command input\n"
	CC_WHITE "    F5                   " CC_NORM "- toggle run, stop\n"
	CC_WHITE "    F6, ^F6              " CC_NORM "- switch registers view (CPU/FPU)\n"
	CC_WHITE "    F8                   " CC_NORM "- set program counter at cursor\n"
	CC_WHITE "    F9                   " CC_NORM "- toggle breakpoint\n"
//      CC_WHITE "    F10                  " CC_NORM "- step thru call\n"
	CC_WHITE "    F11                  " CC_NORM "- step \n"
	"\n"
	CC_CYAN  "--- misc keys -----------------------------------------------------------------\n"
	CC_WHITE "    PGUP, PGDN           " CC_NORM "- scroll windows\n"
	CC_WHITE "    ENTER, ESC           " CC_NORM "- follow/return branch (in disasm window)\n"
	"\n";

static void cmd_help(void) {
	int i;

	if (tokencount > 1) {
		for (i = 0; i < cmd_num - 1; i++) {
			if (!stricmp(tokens[1], cmd_list[i].name)) {
				if (cmd_list[i].helptext) Printf(cmd_list[i].helptext);
				else Printf(CC_GREEN "help: no topic found for \"%s\"\n", cmd_list[i].name);
				return;
			}
		}
		Printf(CC_GREEN "help: unknown command '%s'\n", tokens[1]);
	} else Printf(help_text);
}

static void cmd_except(void) {
	debug_exception_mode ^= 1;
	Printf("Exception mode: %s", debug_exception_mode ? "On" : "Off");
}

#ifdef STAT_ENABLE
static void cmd_stat(void) {
	Printf("Stat Flush Start\n");
	statFlush();
	Printf("Stat Flush Finished\n");
}
#endif

// ---------------------------------------------------------------------------

DebugCmd cmd_list[] = {
	{ ".", cmd_dot, 0,												// .
	  CC_CYAN ".: Set disasm cursor to PC location.\n"
	  "syntax: .\n" },

	{ "db", cmd_db, 0,												// d
	  CC_CYAN "db: Set memory viewer cursor to given address.\n"
	  "syntax: db [r##|r#.b|rp]\n" },

	{ "u", cmd_u, 0,												// u
	  CC_CYAN "u: Set CPU disassembler cursor to given address.\n"
	  "syntax: u [r##|r#.b|rp]\n" },

	{ "pc", cmd_pc, 0,												// pc
	  CC_CYAN "pc: Set program counter to current disassembler line or expression.\n"
	  "syntax: pc [r##|r#.b|rp]\n" },

	{ "bp", cmd_bp, 2,												// bp
	  CC_CYAN "bp: Set access breakpoint to memory address.\n"
	  "syntax: bp [rwx] [r##|r#.b|rp]\n" },

	{ "bpr", cmd_bpr, 3,											// bpr
	  CC_CYAN "bpr: Set access breakpoint to memory area.\n"
	  "syntax: bpr [rwx] [r##|r#.b|rp] [r##|r#.b|rp]\n" },

	{ "bc", cmd_bc, 1,												// bc
	  CC_CYAN "bc: Clear breakpoint with desired number or clear breakpoints list.\n"
	  "syntax: bc [#|*]\n" },

	{ "bl", cmd_bl, 0,												// bl
	  CC_CYAN "bl: List current breakpoits.\n"
	  "syntax: bl\n" },

	{ "run", cmd_run, 0,											// run
	  CC_CYAN "run: Run paused emulation.\n" },

	{ "pause", cmd_pause, 0,										// pause
	  CC_CYAN "pause: Pause emualation.\n" },

	{ "step", cmd_step, 0,											// step in
	  CC_CYAN "step: Step by step execution with subroutines.\n" },

	{ "help", cmd_help, 0, NULL },									// help

	{ "exception", cmd_except, 0, NULL },							// exception

#ifdef STAT_ENABLE
	{ "stat", cmd_stat, 0, NULL },									// stat
#endif
};
int cmd_num = sizeof(cmd_list) / sizeof(DebugCmd);
