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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "types.h"

#define PAD_MODULE_NAME "padDemul"

#define MAX_JOYS 4

#define CONT_C                  (1 << 0)
#define CONT_B                  (1 << 1)
#define CONT_A                  (1 << 2)
#define CONT_START              (1 << 3)
#define CONT_DPAD_UP            (1 << 4)
#define CONT_DPAD_DOWN          (1 << 5)
#define CONT_DPAD_LEFT          (1 << 6)
#define CONT_DPAD_RIGHT         (1 << 7)
#define CONT_Z                  (1 << 8)
#define CONT_Y                  (1 << 9)
#define CONT_X                  (1 << 10)
#define CONT_D                  (1 << 11)
#define CONT_DPAD2_UP           (1 << 12)
#define CONT_DPAD2_DOWN         (1 << 13)
#define CONT_DPAD2_LEFT         (1 << 14)
#define CONT_DPAD2_RIGHT        (1 << 15)
#define CONT_RIGHT_TRIG         (1 << 16)
#define CONT_LEFT_TRIG          (1 << 17)
#define CONT_STICK1X            (1 << 18)
#define CONT_STICK1Y            (1 << 19)
#define CONT_STICK2X            (1 << 20)
#define CONT_STICK2Y            (1 << 21)

#define KEY_JOY_KEY1    (1 << 24)
#define KEY_JOY_KEY2    (2 << 24)
#define KEY_JOY_POV     (4 << 24)

#define CASE0 (0 << 8)
#define CASE1 (1 << 8)
#define CASE2 (2 << 8)
#define CASE3 (3 << 8)

typedef struct {
	u16 joyButtons;
	u8 rightTrig;
	u8 leftTrig;
	u8 stick1X;
	u8 stick1Y;
	u8 stick2X;
	u8 stick2Y;
} CONTROLLER;

bool OpenDevice();
int  GetKeyDevice();
void GetControllerDevice(CONTROLLER *controller, u32 port);
void CloseDevice();
bool IsOpened();
#endif