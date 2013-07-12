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

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>
#include "device.h"
#include "config.h"
#include "plugins.h"

#define KEYDOWN(key) (keys[key] & 0x80)

LPDIRECTINPUT8 lpDI = NULL;
LPDIRECTINPUTDEVICE8 lpDIKeyboard = NULL;
LPDIRECTINPUTDEVICE8 lpDIJoystick[MAX_JOYS] = { NULL };
u32 joysCount = 0;

u32 JoyCaps[MAX_JOYS];
DIJOYSTATE JoyState[MAX_JOYS];
char keys[256];


extern HINSTANCE hinstance;
extern DEmulInfo *gDemulInfo;

BOOL CALLBACK InitJoystickAxes(LPCDIDEVICEOBJECTINSTANCE lpDIOIJoy, LPVOID pvRef) {
	DIPROPRANGE diprg;

	diprg.diph.dwSize = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwObj = lpDIOIJoy->dwType;
	diprg.diph.dwHow = DIPH_BYID;
	diprg.lMin = -127;
	diprg.lMax = 127;

	IDirectInputDevice8_SetProperty((LPDIRECTINPUTDEVICE8)pvRef, DIPROP_RANGE, &diprg.diph);
	return DIENUM_CONTINUE;
}

BOOL CALLBACK InitJoystick(LPCDIDEVICEINSTANCE lpDIIJoy, LPVOID pvRef) {
	HRESULT hr;

	if (joysCount >= MAX_JOYS) return DIENUM_STOP;

	lpDIJoystick[joysCount] = NULL;

	hr = IDirectInput8_CreateDevice(lpDI, (REFGUID)&lpDIIJoy->guidInstance, &lpDIJoystick[joysCount], NULL);
	if (hr != DI_OK) {
		MessageBox(GetActiveWindow(), "IDirectInput CreateDevice FAILED", "padDemul", MB_OK);
		return DIENUM_CONTINUE;
	}

	hr = IDirectInputDevice8_SetDataFormat(lpDIJoystick[joysCount], &c_dfDIJoystick);
	if (hr != DI_OK) {
		IDirectInputDevice8_Release(lpDIJoystick[joysCount]);
		lpDIJoystick[joysCount] = NULL;
		MessageBox(GetActiveWindow(), "IDirectInputDevice8 SetDataFormat FAILED", "padDemul", MB_OK);
		return DIENUM_CONTINUE;
	}

	joysCount++;

	return DIENUM_CONTINUE;
}

bool OpenDevice() {
	int i;
	HRESULT hr;

	for (i = 0; i < MAX_JOYS; i++)
		lpDIJoystick[i] = NULL;

	hr = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, &lpDI, NULL);
	if (hr != DI_OK)
		return 0;

	hr = IDirectInput8_CreateDevice(lpDI, (REFGUID)&GUID_SysKeyboard, &lpDIKeyboard, NULL);
	if (hr != DI_OK) {
		MessageBox(GetActiveWindow(), "IDirectInput CreateDevice FAILED", "padDemul", MB_OK);
		return 0;
	}

	if (gDemulInfo) {
		hr = IDirectInputDevice8_SetCooperativeLevel(lpDIKeyboard, gDemulInfo->hGpuWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		if (hr != DI_OK) {
			MessageBox(GetActiveWindow(), "IDirectInputDevice8_SetCooperativeLevel FAILED", "padDemul", MB_OK);
			return 0;
		}
	}

	hr = IDirectInputDevice8_SetDataFormat(lpDIKeyboard, &c_dfDIKeyboard);
	if (hr != DI_OK) {
		MessageBox(GetActiveWindow(), "IDirectInputDevice8 SetDataFormat FAILED", "padDemul", MB_OK);
		return 0;
	}

	hr = IDirectInput_EnumDevices(lpDI, DI8DEVCLASS_GAMECTRL, InitJoystick, NULL, DIEDFL_ATTACHEDONLY);
	if (hr != DI_OK) {
		MessageBox(GetActiveWindow(), "IDirectInput EnumDevices FAILED", "padDemul", MB_OK);
		return 0;
	}

	for (i = 0; i < MAX_JOYS; i++) {
		if (lpDIJoystick[i] != NULL) {
			hr = IDirectInputDevice8_EnumObjects(lpDIJoystick[i], InitJoystickAxes, lpDIJoystick[i], DIDFT_AXIS);
			if (hr != DI_OK) {
				MessageBox(GetActiveWindow(), "IDirectInputDevice8 EnumObjects FAILED", "padDemul", MB_OK);
				return 0;
			}
		}
	}

	return 1;
}

void CloseDevice() {
	int i;

	if (lpDIKeyboard != NULL) {
		IDirectInputDevice8_Release(lpDIKeyboard);
		lpDIKeyboard = NULL;
	}

	for (i = 0; i < MAX_JOYS; i++) {
		if (lpDIJoystick[i] != NULL) {
			IDirectInputDevice8_Release(lpDIJoystick[i]);
			lpDIJoystick[i] = NULL;
		}
	}

	if (lpDI != NULL) {
		IDirectInput_Release(lpDI);
		lpDI = NULL;
	}

	joysCount = 0;
}

bool IsOpened() {
	return(lpDI != NULL);
}

__inline bool ReadDevice() {
	u32 i;
	HRESULT hr;

	hr = IDirectInputDevice8_GetDeviceState(lpDIKeyboard, 256, &keys);
	if (DI_OK != hr) {
		do {
			hr = IDirectInputDevice8_Acquire(lpDIKeyboard);
		} while (hr == DIERR_INPUTLOST);

		if (DI_OK != hr)
			return false;
		hr = IDirectInputDevice8_GetDeviceState(lpDIKeyboard, 256, &keys);
		if (DI_OK != hr)
			return false;
	}

	for (i = 0; i < joysCount; i++) {
		hr = IDirectInputDevice8_Poll(lpDIJoystick[i]);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			do {
				hr = IDirectInputDevice8_Acquire(lpDIJoystick[i]);
			} while (hr == DIERR_INPUTLOST);

			hr = IDirectInputDevice8_Poll(lpDIJoystick[i]);
			if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				return false;
		}

		hr = IDirectInputDevice8_GetDeviceState(lpDIJoystick[i], sizeof(DIJOYSTATE), &JoyState[i]);
		if (DI_OK != hr)
			return false;
	}
	return true;
}

/*	char buf[33];
    int i;*/
/*			ltoa(state, buf, 10);
            i = strlen(buf);
            buf[i] = '\n';
            buf[i+1] = 0;

            OutputDebugString(buf);*/


__inline bool CheckKeyPressed(u32 key) {
	u8 joyIdx;

	if (key == 0)
		return false;

	if (key < 256) {
		return KEYDOWN(key) != 0;
	} else {
		joyIdx = (key >> 16) & 0xff;
		if (key & KEY_JOY_KEY1) {
			return (JoyState[joyIdx].rgbButtons[key & 0xff]) != 0;
		} else if (key & KEY_JOY_KEY2) {
			LONG state;
			state = ((LONG*)&(JoyState[joyIdx]))[key & 0x7];
			switch (key & 0xFF00) {
			case CASE0: return(state > 100);
			case CASE1: return(state < -100);
			}
		} else if (key & KEY_JOY_POV) {
			DWORD state = JoyState[joyIdx].rgdwPOV[key & 0x3];

			switch (key & 0xFF00) {
			case CASE0: return(((state >= 0) && (state <= 4500)) || ((state >= 31500) && (state < 36000)));
			case CASE1: return((state >= 4500) && (state <= 13500));
			case CASE2: return((state >= 13500) && (state <= 22500));
			case CASE3: return((state >= 22500) && (state <= 31500));
			}
		}
	}
	return false;
}

__inline u8 CheckKeyAxis(u32 key) {
	u8 joyIdx;

	if (key == 0)
		return 0;

	if (key < 256) {
		return (KEYDOWN(key) != 0) ? 127 : 0;
	} else {
		joyIdx = (key >> 16) & 0xff;
		if (key & KEY_JOY_KEY1) {
			return ((JoyState[joyIdx].rgbButtons[key & 0xff]) != 0) ? 127 : 0;
		} else if (key & KEY_JOY_KEY2) {
			LONG state;
			state = ((LONG*)&(JoyState[joyIdx]))[key & 0x7];
			switch (key & 0xFF00) {
			case CASE0: return (state > 0) ? (u8)state : 0;
			case CASE1: return (state < 0) ? (u8)(-state) : 0;
			}
		} else if (key & KEY_JOY_POV) {
			DWORD state = JoyState[joyIdx].rgdwPOV[key & 0x3];

			switch (key & 0xFF00) {
			case CASE0: return (((state >= 0) && (state <= 4500)) || ((state >= 31500) && (state < 36000)))  ? 127 : 0;
			case CASE1: return ((state >= 4500) && (state <= 13500)) ? 127 : 0;
			case CASE2: return ((state >= 13500) && (state <= 22500)) ? 127 : 0;
			case CASE3: return ((state >= 22500) && (state <= 31500)) ? 127 : 0;
			}
		}
	}
	return 0;
}

void GetControllerDevice(CONTROLLER *controller, u32 port) {
/*	char buf[32];
    int i;*/

	controller->joyButtons = 0xffff;
	controller->leftTrig = controller->rightTrig = 0;
	controller->stick1X = controller->stick1Y = controller->stick2X = controller->stick2Y = 128;

	if (!ReadDevice()) return;

	if (KEYDOWN(DIK_ESCAPE) != 0)
		PostMessage(gDemulInfo->hMainWnd, WM_QUIT, 0, 0);

#define CHECK_KEY(key, mask) \
	if (CheckKeyPressed(key)) controller->joyButtons &= ~mask; else

	CHECK_KEY(cfg.joy[port].UP, CONT_DPAD_UP);
	CHECK_KEY(cfg.joy[port].DOWN, CONT_DPAD_DOWN);
	CHECK_KEY(cfg.joy[port].LEFT, CONT_DPAD_LEFT);
	CHECK_KEY(cfg.joy[port].RIGHT, CONT_DPAD_RIGHT);
	CHECK_KEY(cfg.joy[port].A, CONT_A);
	CHECK_KEY(cfg.joy[port].B, CONT_B);
	CHECK_KEY(cfg.joy[port].C, CONT_C);
	CHECK_KEY(cfg.joy[port].D, CONT_D);
	CHECK_KEY(cfg.joy[port].X, CONT_X);
	CHECK_KEY(cfg.joy[port].Y, CONT_Y);
	CHECK_KEY(cfg.joy[port].Z, CONT_Z);
	CHECK_KEY(cfg.joy[port].START, CONT_START);

#define CHECK_AXIS(key, name) \
	{ u8 v = CheckKeyAxis(key); if (v > 0) controller->name = v << 1; else controller->name = 0; }

	CHECK_AXIS(cfg.joy[port].LTRIG, leftTrig);
	CHECK_AXIS(cfg.joy[port].RTRIG, rightTrig);

#define CHECK_AXIS2(key1, key2, name) \
	{ u8 v = CheckKeyAxis(key1); if (v > 0) controller->name = 127 - v; else { v = CheckKeyAxis(key2);  if (v > 0) controller->name = 128 + v; else controller->name = 128; } }

	CHECK_AXIS2(cfg.joy[port].STICK1LEFT, cfg.joy[port].STICK1RIGHT, stick1X);
	CHECK_AXIS2(cfg.joy[port].STICK1UP, cfg.joy[port].STICK1DOWN, stick1Y);
	CHECK_AXIS2(cfg.joy[port].STICK2LEFT, cfg.joy[port].STICK2RIGHT, stick2X);
	CHECK_AXIS2(cfg.joy[port].STICK2UP, cfg.joy[port].STICK2DOWN, stick2Y);

/*	if (port == 0)
    {
    ltoa(controller->stick1X, buf, 10);
    i = strlen(buf);
    buf[i] = '\n';
    buf[i+1] = 0;

    OutputDebugString(buf);
    ltoa(controller->stick1Y, buf, 10);
    i = strlen(buf);
    buf[i] = '\n';
    buf[i+1] = 0;

    OutputDebugString(buf);
    }*/
}


int GetKeyDevice() {
	u32 i, j, joyIdx;

	if (!ReadDevice()) return 0;

	for (i = 1; i < 256; i++)
		if (KEYDOWN(i)) return i;

	for (i = 0; i < joysCount; i++) {
		joyIdx = (i << 16);
		for (j = 0; j < 32; j++) {
			if (JoyState[i].rgbButtons[j] != 0)
				return KEY_JOY_KEY1 | joyIdx | j;
		}

		for (j = 0; j < 8; j++) {
			LONG state;
			state = ((LONG*)&(JoyState[i]))[j];
			if (abs(state) < 100) continue;

			if (state > 0) return KEY_JOY_KEY2 | joyIdx | j | CASE0;
			else if (state < 0) return KEY_JOY_KEY2 | joyIdx | j | CASE1;
		}

		for (j = 0; j < 4; j++) {
			DWORD state = JoyState[i].rgdwPOV[j];

			if (((state >= 0) && (state <= 4500)) || ((state >= 31500) && (state < 36000))) {
				return KEY_JOY_POV | joyIdx | j | CASE0;
			}

			if ((state >= 4500) && (state <= 13500)) {
				return KEY_JOY_POV | joyIdx | j | CASE1;
			}

			if ((state >= 13500) && (state <= 22500)) {
				return KEY_JOY_POV | joyIdx | j | CASE2;
			}

			if ((state >= 22500) && (state <= 31500)) {
				return KEY_JOY_POV | joyIdx | j | CASE3;
			}
		}
	}

	return 0;
}

