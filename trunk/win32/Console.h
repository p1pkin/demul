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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#define MAX_PROFILE_COUNT 10

int  openConsole();
void closeConsole();

// Drawing.
// ---

// Console color codes. Format: [Control Byte] [Color Byte]
// Control Byte: 0x1 - set foreground color, 0x2 - set background color.
// You can use colors in following way: print(CC_BRED "Red Message\n");
#define CC_BLACK   "\x1\x10"
#define CC_BLUE    "\x1\x1"
#define CC_GREEN   "\x1\x2"
#define CC_CYAN    "\x1\x3"
#define CC_RED     "\x1\x4"
#define CC_PURPLE  "\x1\x5"
#define CC_BROWN   "\x1\x6"
#define CC_NORM    "\x1\x7"
#define CC_GRAY    "\x1\x8"
#define CC_BBLUE   "\x1\x9"
#define CC_BGREEN  "\x1\xa"
#define CC_BCYAN   "\x1\xb"
#define CC_BRED    "\x1\xc"
#define CC_BPURPLE "\x1\xd"
#define CC_YELLOW  "\x1\xe"
#define CC_WHITE   "\x1\xf"

#define CB_BLACK   "\x2\x10"
#define CB_BLUE    "\x2\x1"
#define CB_GREEN   "\x2\x2"
#define CB_CYAN    "\x2\x3"
#define CB_RED     "\x2\x4"
#define CB_PURPLE  "\x2\x5"
#define CB_BROWN   "\x2\x6"
#define CB_NORM    "\x2\x7"
#define CB_GRAY    "\x2\x8"
#define CB_BBLUE   "\x2\x9"
#define CB_BGREEN  "\x2\xa"
#define CB_BCYAN   "\x2\xb"
#define CB_BRED    "\x2\xc"
#define CB_BPURPLE "\x2\xd"
#define CB_YELLOW  "\x2\xe"
#define CB_WHITE   "\x2\xf"

void    ConPrintAt(int X, int Y, char *text);
void    ConPrintfAt(int X, int Y, char *text, ...);
void    ConUpdateRegion(int regY, int regH);
void    ConFillRegion(int regY, int regH, char *pattern);
void    ConSetCursor(int X, int Y);				// -1 any, to hide.

// Console Input.
// ---

// Console key definitions. ASCII keys are represented by their char symbol.
#define CK_NULL     -1			// no input at all.
#define CK_CONTROL  1			// no ascii input, but control key is pressed (see ctrl mask)
#define CK_ESC      0x0001		// non-ascii control keys (returned in ctrl)
#define CK_RETURN   0x0002
#define CK_BKSPACE  0x0004
#define CK_UP       0x0008
#define CK_DOWN     0x0010
#define CK_RIGHT    0x0020
#define CK_LEFT     0x0040
#define CK_INS      0x0080
#define CK_DEL      0x0100
#define CK_HOME     0x0200
#define CK_END      0x0400
#define CK_PGUP     0x0800
#define CK_PGDN     0x1000
#define CK_TAB      0x2000
#define CK_F(X)     (1 << (16 + X - 1))
#define CK_FKEYS    (CK_F(1) | CK_F(2) | CK_F(3) | CK_F(4) | CK_F(5) | CK_F(6) | CK_F(7) | CK_F(8) | CK_F(9) | CK_F(10) | CK_F(11) | CK_F(12))
#define CK_SHIFT    (0x80000000)	// ctrl, alt or shift bit mask (in ctrl)
#define CK_ALT      (0x40000000)
#define CK_CTRL     (0x20000000)

char    ConPeekInput(int *ctrl);	// return ASCII key, or control key value.
									// "ctrl" will flag pressed control keys.
char    ConGetInput(int *ctrl);		// same as peek, but wait until key pressed.

#endif