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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "types.h"
#include "device.h"

typedef struct {
	u32 A;
	u32 B;
	u32 C;
	u32 D;

	u32 X;
	u32 Y;
	u32 Z;

	u32 UP;
	u32 DOWN;
	u32 LEFT;
	u32 RIGHT;

	u32 UP2;
	u32 DOWN2;
	u32 LEFT2;
	u32 RIGHT2;

	u32 START;

	u32 LTRIG;
	u32 RTRIG;

	u32 STICK1UP;
	u32 STICK1DOWN;
	u32 STICK1LEFT;
	u32 STICK1RIGHT;

	u32 STICK2UP;
	u32 STICK2DOWN;
	u32 STICK2LEFT;
	u32 STICK2RIGHT;
} JOY;

typedef struct {
	JOY joy[MAX_JOYS];
} CFG;

typedef struct {
	int EditUp;
	int EditDown;
	int EditLeft;
	int EditRight;
	int EditA;
	int EditB;
	int EditC;
	int EditD;
	int EditX;
	int EditY;
	int EditZ;
	int EditRTrig;
	int EditLTrig;
	int EditStart;
	int EditS1Up;
	int EditS1Down;
	int EditS1Left;
	int EditS1Right;
	int EditS2Up;
	int EditS2Down;
	int EditS2Left;
	int EditS2Right;

	int ButtonUp;
	int ButtonDown;
	int ButtonLeft;
	int ButtonRight;
	int ButtonA;
	int ButtonB;
	int ButtonC;
	int ButtonD;
	int ButtonX;
	int ButtonY;
	int ButtonZ;
	int ButtonRTrig;
	int ButtonLTrig;
	int ButtonStart;
	int ButtonS1Up;
	int ButtonS1Down;
	int ButtonS1Left;
	int ButtonS1Right;
	int ButtonS2Up;
	int ButtonS2Down;
	int ButtonS2Left;
	int ButtonS2Right;
} PadControl;

extern CFG cfg;
extern u32 JoyCaps[MAX_JOYS];
extern HINSTANCE hinstance;


bool LoadConfig(bool autoSetConfig);
bool SetConfig();
void SaveConfig();
void CalcJoyCaps();

#endif