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

#include <windows.h>
#include <stdio.h>
#include "console.h"
#include "demul.h"

#ifdef DEMUL_DEBUG

#define CON_WIDTH   80
#define CON_HEIGHT  60
#define CON_TITLE   "Dreamcast Console Debugger"

// Private structure, used internally in this module.
static struct CON_CB {
	BOOL opened;
	int x, y;
	int width, height;

	HWND hwnd;
	HANDLE input, output;
	CHAR_INFO*  buf;			// console output buffer : [width][height];
	CHAR_INFO attr;
} con;

// ---------------------------------------------------------------------------
// Drawing.

// Set current output text color
#define con_attr(bg, fg)    (con.attr.Attributes = ((bg & 0xf) << 4) | (fg & 0xf))
#define con_attr_fg(fg)     (con.attr.Attributes = (con.attr.Attributes & ~0xf) | (fg & 0xf))
#define con_attr_bg(bg)     (con.attr.Attributes = (con.attr.Attributes & ~(0xf << 4)) | ((bg & 0xf) << 4))

static void con_nextline(void) {
	con.x = 0;
	con.y++;
	if (con.y >= con.height) con.y = con.height - 1;
}

static void con_printchar(char ch) {
	if ((con.x >= 0 && con.x < con.width) && (con.y >= 0 && con.y < con.height)) {
		con.buf[con.y * con.width + con.x] = con.attr;
		con.buf[con.y * con.width + con.x].Char.AsciiChar = ch;
		con.x++;
	}
}

// ---

void ConPrintAt(int X, int Y, char *text) {
	int i, length;

	if (!con.opened) return;

	length = strlen(text);
	con.x = X; con.y = Y;

	con_attr(0, 7);			// set normal color

	for (i = 0; i < length; i++) {
		unsigned ch = text[i];
		if (ch == '\n') con_nextline();
		else if (ch == '\t') {
			int tab = (con.x % 4) + 4;
			while (tab--) con_printchar(' ');
		} else if (ch == 0x1) {
			char col = text[++i];
			if (col == 0x10) col = 0;
			con_attr_fg(col);
		} else if (ch == 0x2) {
			char col = text[++i];
			if (col == 0x10) col = 0;
			con_attr_bg(col);
		} else con_printchar(ch);
	}
}

void ConPrintfAt(int X, int Y, char *text, ...) {	// wraps to ConPrintAt.
	va_list arg;
	char buffer[1000];

	va_start(arg, text);
	vsprintf(buffer, text, arg);
	va_end(arg);

	ConPrintAt(X, Y, buffer);
}

void ConUpdateRegion(int regY, int regH) {
	COORD pos = { 0, regY };
	COORD sz = { con.width, con.height };
	SMALL_RECT rgn = { 0, regY, con.width - 1, (regY + regH - 1) };

	if (!con.opened) return;
	WriteConsoleOutput(con.output, con.buf, sz, pos, &rgn);
}

void ConFillRegion(int regY, int regH, char *pattern) {
	int y, i, patlen, wx;

	if (!con.opened) return;
	patlen = strlen(pattern);
	if (patlen == 0) pattern = " ";

	con_attr(0, 7);			// set normal color

	for (y = regY; y < (regY + regH); y++) {
		con.x = 0; con.y = y;
		wx = con.width;
		while (wx) {
			for (i = 0; i < patlen; i++) {
				unsigned ch = pattern[i];
				if (ch == 0x1) {
					char col = pattern[++i];
					if (col == 0x10) col = 0;
					con_attr_fg(col);
				} else if (ch == 0x2) {
					char col = pattern[++i];
					if (col == 0x10) col = 0;
					con_attr_bg(col);
				} else { con_printchar(ch); wx--; }
				if (wx == 0) break;
			}
		}
	}
}

void ConSetCursor(int X, int Y) {
	COORD cr = { X, Y };
	if (!con.opened) return;
	SetConsoleCursorPosition(con.output, cr);
}

// ---------------------------------------------------------------------------
// Input.

char ConPeekInput(int *ctrl) {
	INPUT_RECORD record;
	DWORD count;

	static int vkey_mapping[256] = {	// 0-ignore, -1 - ascii, otherwise-control code.
		0, 0, 0, 0, 0, 0, 0, 0, CK_BKSPACE, CK_TAB, 0, 0, 0, CK_RETURN, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CK_ESC, 0, 0, 0, 0,
		-1, CK_PGUP, CK_PGDN, CK_END, CK_HOME, CK_LEFT, CK_UP, CK_RIGHT, CK_DOWN, 0, 0, 0, 0, CK_INS, CK_DEL, 0,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		CK_F(1), CK_F(2), CK_F(3), CK_F(4), CK_F(5), CK_F(6), CK_F(7), CK_F(8), CK_F(9), CK_F(10), CK_F(11), CK_F(12), 0, 0, 0, 0,

		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};

	if (!con.opened) return CK_NULL;

	PeekConsoleInput(con.input, &record, 1, &count);
	if (!count) return CK_NULL;
	ReadConsoleInput(con.input, &record, 1, &count);

	if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
		char ascii = record.Event.KeyEvent.uChar.AsciiChar;
		int vcode = record.Event.KeyEvent.wVirtualKeyCode;
		int ckeys = record.Event.KeyEvent.dwControlKeyState;

		*ctrl = 0;
		if (ckeys & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) *ctrl |= CK_CTRL;
		if (ckeys & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) *ctrl |= CK_ALT;
		if (ckeys & SHIFT_PRESSED) *ctrl |= CK_SHIFT;

		if (vkey_mapping[vcode] == 0) return CK_NULL;
		else if (vkey_mapping[vcode] == -1) return ascii;
		else { *ctrl |= vkey_mapping[vcode]; return CK_CONTROL; }
	} else return CK_NULL;
}

char ConGetInput(int *ctrl) {		// wraps to input peek
	char ch;
	while ((ch = ConPeekInput(ctrl)) == CK_NULL) ;
	return ch;
}

// ---------------------------------------------------------------------------
// Controls.

static BOOL CALLBACK WndEnumProc(HWND hWnd, LPARAM lpParam) {
	TCHAR szClassName[1024];
	DWORD dwPid = 0;

	ZeroMemory(szClassName, sizeof(szClassName));

	if (!GetClassName(hWnd, szClassName, sizeof(szClassName))) return TRUE;
	if (lstrcmp(szClassName, TEXT("tty")) != 0) return TRUE;

	GetWindowThreadProcessId(hWnd, &dwPid);
	if (dwPid == GetCurrentProcessId()) {
		*(HWND*)lpParam = hWnd;
		return FALSE;
	}

	return TRUE;
}

static HWND GetConsoleWindow(void) {
	HWND hwnd = NULL;
	EnumWindows((WNDENUMPROC)WndEnumProc, (LPARAM)&hwnd);
	return hwnd;
}

int openConsole() {
	DWORD flags;
	COORD coord;
	SMALL_RECT rect;
	RECT wndrect;
	CONSOLE_CURSOR_INFO curinfo;
	int tries = 3;

	if (con.opened) return true;

	con.width = CON_WIDTH;
	con.height = CON_HEIGHT;

	// Allocate output buffer
	con.buf = (CHAR_INFO*)malloc(con.width * con.height * sizeof(CHAR_INFO));
	if (con.buf == NULL) return false;
	memset(con.buf, 0, con.width * con.height * sizeof(CHAR_INFO));

	// create console
	if (AllocConsole() == 0) return false;

	// get input/ouput handles
	con.input = GetStdHandle(STD_INPUT_HANDLE);
	if (con.input == INVALID_HANDLE_VALUE) {
		FreeConsole();
		return false;
	}
	con.output = GetStdHandle(STD_OUTPUT_HANDLE);
	if (con.output == INVALID_HANDLE_VALUE) {
		FreeConsole();
		return false;
	}

	// setup console window
	con.hwnd = GetConsoleWindow();
	GetConsoleCursorInfo(con.output, &curinfo);
	GetConsoleMode(con.input, &flags);
	flags &= ~ENABLE_MOUSE_INPUT;
	SetConsoleMode(con.input, flags);

	rect.Top = rect.Left = 0;
	rect.Right = con.width - 1;
	rect.Bottom = con.height - 1;

	coord.X = con.width;
	coord.Y = con.height;

	SetConsoleWindowInfo(con.output, TRUE, &rect);
	SetConsoleScreenBufferSize(con.output, coord);

	// change window layout
	if (con.hwnd)
		while (tries--) {
			GetWindowRect(con.hwnd, &wndrect);
			if (wndrect.right >= GetSystemMetrics(SM_CXSCREEN)) break;
			SetWindowPos(
				con.hwnd,
				HWND_TOP,
				GetSystemMetrics(SM_CXSCREEN) - (wndrect.right - wndrect.left),
				0, 0, 0, SWP_NOSIZE);
		}
	SetConsoleTitle(CON_TITLE);

	return(con.opened = true);
}

void closeConsole() {
	if (!con.opened) return;

	if (con.buf) {
		free(con.buf);
		con.buf = NULL;
	}
	if (con.input) {
		CloseHandle(con.input);
		con.input = NULL;
	}
	if (con.output) {
		CloseHandle(con.output);
		con.output = NULL;
	}
	FreeConsole();

	con.opened = false;
}

#endif
