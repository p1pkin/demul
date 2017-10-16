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
#include <stdio.h>
#include <string.h>
#include <dinput.h>
#include "resource.h"
#include "config.h"
#include "device.h"
#include "inifile.h"

PadControl padControls[4] =
{
	{ IDC_EUPP1, IDC_EDOWNP1, IDC_ELEFTP1, IDC_ERIGHTP1, IDC_EAP1, IDC_EBP1, IDC_ECP1, IDC_EDP1, IDC_EXP1, IDC_EYP1, IDC_EZP1, IDC_ELTRIGP1, IDC_ERTRIGP1, IDC_ESTARTP1, IDC_ES1UPP1, IDC_ES1DOWNP1, IDC_ES1LEFTP1, IDC_ES1RIGHTP1, IDC_ES2UPP1, IDC_ES2DOWNP1, IDC_ES2LEFTP1, IDC_ES2RIGHTP1,
	  IDC_UPP1, IDC_DOWNP1, IDC_LEFTP1, IDC_RIGHTP1, IDC_AP1, IDC_BP1, IDC_CP1, IDC_DP1, IDC_XP1, IDC_YP1, IDC_ZP1, IDC_LTRIGP1, IDC_RTRIGP1, IDC_STARTP1, IDC_S1UPP1, IDC_S1DOWNP1, IDC_S1LEFTP1, IDC_S1RIGHTP1, IDC_S2UPP1, IDC_S2DOWNP1, IDC_S2LEFTP1, IDC_S2RIGHTP1 },
	{ IDC_EUPP2, IDC_EDOWNP2, IDC_ELEFTP2, IDC_ERIGHTP2, IDC_EAP2, IDC_EBP2, IDC_ECP2, IDC_EDP2, IDC_EXP2, IDC_EYP2, IDC_EZP2, IDC_ELTRIGP2, IDC_ERTRIGP2, IDC_ESTARTP2, IDC_ES1UPP2, IDC_ES1DOWNP2, IDC_ES1LEFTP2, IDC_ES1RIGHTP2, IDC_ES2UPP2, IDC_ES2DOWNP2, IDC_ES2LEFTP2, IDC_ES2RIGHTP2,
	  IDC_UPP2, IDC_DOWNP2, IDC_LEFTP2, IDC_RIGHTP2, IDC_AP2, IDC_BP2, IDC_CP2, IDC_DP2, IDC_XP2, IDC_YP2, IDC_ZP2, IDC_LTRIGP2, IDC_RTRIGP2, IDC_STARTP2, IDC_S1UPP2, IDC_S1DOWNP2, IDC_S1LEFTP2, IDC_S1RIGHTP2, IDC_S2UPP2, IDC_S2DOWNP2, IDC_S2LEFTP2, IDC_S2RIGHTP2 },
	{ IDC_EUPP3, IDC_EDOWNP3, IDC_ELEFTP3, IDC_ERIGHTP3, IDC_EAP3, IDC_EBP3, IDC_ECP3, IDC_EDP3, IDC_EXP3, IDC_EYP3, IDC_EZP3, IDC_ELTRIGP3, IDC_ERTRIGP3, IDC_ESTARTP3, IDC_ES1UPP3, IDC_ES1DOWNP3, IDC_ES1LEFTP3, IDC_ES1RIGHTP3, IDC_ES2UPP3, IDC_ES2DOWNP3, IDC_ES2LEFTP3, IDC_ES2RIGHTP3,
	  IDC_UPP3, IDC_DOWNP3, IDC_LEFTP3, IDC_RIGHTP3, IDC_AP3, IDC_BP3, IDC_CP3, IDC_DP3, IDC_XP3, IDC_YP3, IDC_ZP3, IDC_LTRIGP3, IDC_RTRIGP3, IDC_STARTP3, IDC_S1UPP3, IDC_S1DOWNP3, IDC_S1LEFTP3, IDC_S1RIGHTP3, IDC_S2UPP3, IDC_S2DOWNP3, IDC_S2LEFTP3, IDC_S2RIGHTP3 },
	{ IDC_EUPP4, IDC_EDOWNP4, IDC_ELEFTP4, IDC_ERIGHTP4, IDC_EAP4, IDC_EBP4, IDC_ECP4, IDC_EDP4, IDC_EXP4, IDC_EYP4, IDC_EZP4, IDC_ELTRIGP4, IDC_ERTRIGP4, IDC_ESTARTP4, IDC_ES1UPP4, IDC_ES1DOWNP4, IDC_ES1LEFTP4, IDC_ES1RIGHTP4, IDC_ES2UPP4, IDC_ES2DOWNP4, IDC_ES2LEFTP4, IDC_ES2RIGHTP4,
	  IDC_UPP4, IDC_DOWNP4, IDC_LEFTP4, IDC_RIGHTP4, IDC_AP4, IDC_BP4, IDC_CP4, IDC_DP4, IDC_XP4, IDC_YP4, IDC_ZP4, IDC_LTRIGP4, IDC_RTRIGP4, IDC_STARTP4, IDC_S1UPP4, IDC_S1DOWNP4, IDC_S1LEFTP4, IDC_S1RIGHTP4, IDC_S2UPP4, IDC_S2DOWNP4, IDC_S2LEFTP4, IDC_S2RIGHTP4 },
};

CFG cfg;
CFG tuneCfg;

char* GetKeyName(u32 key) {
	static char buf[32];
	u8 joyIdx;
	strcpy(buf, "无");

	if (key < 256) {
		if (key != 0)
			switch (key) {
			case DIK_ESCAPE:
				strcpy(buf, "ESCAPE");
				break;
			case DIK_1:
				strcpy(buf, "1");
				break;
			case DIK_2:
				strcpy(buf, "2");
				break;
			case DIK_3:
				strcpy(buf, "3");
				break;
			case DIK_4:
				strcpy(buf, "4");
				break;
			case DIK_5:
				strcpy(buf, "5");
				break;
			case DIK_6:
				strcpy(buf, "6");
				break;
			case DIK_7:
				strcpy(buf, "7");
				break;
			case DIK_8:
				strcpy(buf, "8");
				break;
			case DIK_9:
				strcpy(buf, "9");
				break;
			case DIK_0:
				strcpy(buf, "0");
				break;
			case DIK_MINUS:
				strcpy(buf, "MINUS");
				break;
			case DIK_EQUALS:
				strcpy(buf, "EQUALS");
				break;
			case DIK_BACK:
				strcpy(buf, "BACK");
				break;
			case DIK_TAB:
				strcpy(buf, "TAB");
				break;
			case DIK_Q:
				strcpy(buf, "Q");
				break;
			case DIK_W:
				strcpy(buf, "W");
				break;
			case DIK_E:
				strcpy(buf, "E");
				break;
			case DIK_R:
				strcpy(buf, "R");
				break;
			case DIK_T:
				strcpy(buf, "T");
				break;
			case DIK_Y:
				strcpy(buf, "Y");
				break;
			case DIK_U:
				strcpy(buf, "U");
				break;
			case DIK_I:
				strcpy(buf, "I");
				break;
			case DIK_O:
				strcpy(buf, "O");
				break;
			case DIK_P:
				strcpy(buf, "P");
				break;
			case DIK_LBRACKET:
				strcpy(buf, "LBRACKET");
				break;
			case DIK_RBRACKET:
				strcpy(buf, "RBRACKET");
				break;
			case DIK_RETURN:
				strcpy(buf, "RETURN");
				break;
			case DIK_LCONTROL:
				strcpy(buf, "LCONTROL");
				break;
			case DIK_A:
				strcpy(buf, "A");
				break;
			case DIK_S:
				strcpy(buf, "S");
				break;
			case DIK_D:
				strcpy(buf, "D");
				break;
			case DIK_F:
				strcpy(buf, "F");
				break;
			case DIK_G:
				strcpy(buf, "G");
				break;
			case DIK_H:
				strcpy(buf, "H");
				break;
			case DIK_J:
				strcpy(buf, "J");
				break;
			case DIK_K:
				strcpy(buf, "K");
				break;
			case DIK_L:
				strcpy(buf, "L");
				break;
			case DIK_SEMICOLON:
				strcpy(buf, "SEMICOLON");
				break;
			case DIK_APOSTROPHE:
				strcpy(buf, "APOSTROPHE");
				break;
			case DIK_GRAVE:
				strcpy(buf, "GRAVE");
				break;
			case DIK_LSHIFT:
				strcpy(buf, "LSHIFT");
				break;
			case DIK_BACKSLASH:
				strcpy(buf, "BACKSLASH");
				break;
			case DIK_Z:
				strcpy(buf, "Z");
				break;
			case DIK_X:
				strcpy(buf, "X");
				break;
			case DIK_C:
				strcpy(buf, "C");
				break;
			case DIK_V:
				strcpy(buf, "V");
				break;
			case DIK_B:
				strcpy(buf, "B");
				break;
			case DIK_N:
				strcpy(buf, "N");
				break;
			case DIK_M:
				strcpy(buf, "M");
				break;
			case DIK_COMMA:
				strcpy(buf, "COMMA");
				break;
			case DIK_PERIOD:
				strcpy(buf, "PERIOD");
				break;
			case DIK_SLASH:
				strcpy(buf, "SLASH");
				break;
			case DIK_RSHIFT:
				strcpy(buf, "RSHIFT");
				break;
			case DIK_MULTIPLY:
				strcpy(buf, "MULTIPLY");
				break;
			case DIK_LMENU:
				strcpy(buf, "LMENU");
				break;
			case DIK_SPACE:
				strcpy(buf, "SPACE");
				break;
			case DIK_CAPITAL:
				strcpy(buf, "CAPITAL");
				break;
			case DIK_F1:
				strcpy(buf, "F1");
				break;
			case DIK_F2:
				strcpy(buf, "F2");
				break;
			case DIK_F3:
				strcpy(buf, "F3");
				break;
			case DIK_F4:
				strcpy(buf, "F4");
				break;
			case DIK_F5:
				strcpy(buf, "F5");
				break;
			case DIK_F6:
				strcpy(buf, "F6");
				break;
			case DIK_F7:
				strcpy(buf, "F7");
				break;
			case DIK_F8:
				strcpy(buf, "F8");
				break;
			case DIK_F9:
				strcpy(buf, "F9");
				break;
			case DIK_F10:
				strcpy(buf, "F10");
				break;
			case DIK_NUMLOCK:
				strcpy(buf, "NUMLOCK");
				break;
			case DIK_SCROLL:
				strcpy(buf, "SCROLL");
				break;
			case DIK_NUMPAD7:
				strcpy(buf, "NUMPAD7");
				break;
			case DIK_NUMPAD8:
				strcpy(buf, "NUMPAD8");
				break;
			case DIK_NUMPAD9:
				strcpy(buf, "NUMPAD9");
				break;
			case DIK_SUBTRACT:
				strcpy(buf, "SUBTRACT");
				break;
			case DIK_NUMPAD4:
				strcpy(buf, "NUMPAD4");
				break;
			case DIK_NUMPAD5:
				strcpy(buf, "NUMPAD5");
				break;
			case DIK_NUMPAD6:
				strcpy(buf, "NUMPAD6");
				break;
			case DIK_ADD:
				strcpy(buf, "ADD");
				break;
			case DIK_NUMPAD1:
				strcpy(buf, "NUMPAD1");
				break;
			case DIK_NUMPAD2:
				strcpy(buf, "NUMPAD2");
				break;
			case DIK_NUMPAD3:
				strcpy(buf, "NUMPAD3");
				break;
			case DIK_NUMPAD0:
				strcpy(buf, "NUMPAD0");
				break;
			case DIK_DECIMAL:
				strcpy(buf, "DECIMAL");
				break;
			case DIK_OEM_102:
				strcpy(buf, "OEM_102");
				break;
			case DIK_F11:
				strcpy(buf, "F11");
				break;
			case DIK_F12:
				strcpy(buf, "F12");
				break;
			case DIK_F13:
				strcpy(buf, "F13");
				break;
			case DIK_F14:
				strcpy(buf, "F14");
				break;
			case DIK_F15:
				strcpy(buf, "F15");
				break;
			case DIK_KANA:
				strcpy(buf, "KANA");
				break;
			case DIK_ABNT_C1:
				strcpy(buf, "ABNT_C1");
				break;
			case DIK_CONVERT:
				strcpy(buf, "CONVERT");
				break;
			case DIK_NOCONVERT:
				strcpy(buf, "NOCONVERT");
				break;
			case DIK_YEN:
				strcpy(buf, "YEN");
				break;
			case DIK_ABNT_C2:
				strcpy(buf, "ABNT_C2");
				break;
			case DIK_NUMPADEQUALS:
				strcpy(buf, "NUMPADEQUALS");
				break;
			case DIK_PREVTRACK:
				strcpy(buf, "PREVTRACK");
				break;
			case DIK_AT:
				strcpy(buf, "AT");
				break;
			case DIK_COLON:
				strcpy(buf, "COLON");
				break;
			case DIK_UNDERLINE:
				strcpy(buf, "UNDERLINE");
				break;
			case DIK_KANJI:
				strcpy(buf, "KANJI");
				break;
			case DIK_STOP:
				strcpy(buf, "STOP");
				break;
			case DIK_AX:
				strcpy(buf, "AX");
				break;
			case DIK_UNLABELED:
				strcpy(buf, "UNLABELED");
				break;
			case DIK_NEXTTRACK:
				strcpy(buf, "NEXTTRACK");
				break;
			case DIK_NUMPADENTER:
				strcpy(buf, "NUMPADENTER");
				break;
			case DIK_RCONTROL:
				strcpy(buf, "RCONTROL");
				break;
			case DIK_MUTE:
				strcpy(buf, "MUTE");
				break;
			case DIK_CALCULATOR:
				strcpy(buf, "CALCULATOR");
				break;
			case DIK_PLAYPAUSE:
				strcpy(buf, "PLAYPAUSE");
				break;
			case DIK_MEDIASTOP:
				strcpy(buf, "MEDIASTOP");
				break;
			case DIK_VOLUMEDOWN:
				strcpy(buf, "VOLUMEDOWN");
				break;
			case DIK_VOLUMEUP:
				strcpy(buf, "VOLUMEUP");
				break;
			case DIK_WEBHOME:
				strcpy(buf, "WEBHOME");
				break;
			case DIK_NUMPADCOMMA:
				strcpy(buf, "NUMPADCOMMA");
				break;
			case DIK_DIVIDE:
				strcpy(buf, "DIVIDE");
				break;
			case DIK_SYSRQ:
				strcpy(buf, "SYSRQ");
				break;
			case DIK_RMENU:
				strcpy(buf, "RMENU");
				break;
			case DIK_PAUSE:
				strcpy(buf, "PAUSE");
				break;
			case DIK_HOME:
				strcpy(buf, "HOME");
				break;
			case DIK_UP:
				strcpy(buf, "UP");
				break;
			case DIK_PRIOR:
				strcpy(buf, "PRIOR");
				break;
			case DIK_LEFT:
				strcpy(buf, "LEFT");
				break;
			case DIK_RIGHT:
				strcpy(buf, "RIGHT");
				break;
			case DIK_END:
				strcpy(buf, "END");
				break;
			case DIK_DOWN:
				strcpy(buf, "DOWN");
				break;
			case DIK_NEXT:
				strcpy(buf, "NEXT");
				break;
			case DIK_INSERT:
				strcpy(buf, "INSERT");
				break;
			case DIK_DELETE:
				strcpy(buf, "DELETE");
				break;
			case DIK_LWIN:
				strcpy(buf, "LWIN");
				break;
			case DIK_RWIN:
				strcpy(buf, "RWIN");
				break;
			case DIK_APPS:
				strcpy(buf, "APPS");
				break;
			case DIK_POWER:
				strcpy(buf, "POWER");
				break;
			case DIK_SLEEP:
				strcpy(buf, "SLEEP");
				break;
			case DIK_WAKE:
				strcpy(buf, "WAKE");
				break;
			case DIK_WEBSEARCH:
				strcpy(buf, "WEBSEARCH");
				break;
			case DIK_WEBFAVORITES:
				strcpy(buf, "WEBFAVORITES");
				break;
			case DIK_WEBREFRESH:
				strcpy(buf, "WEBREFRESH");
				break;
			case DIK_WEBSTOP:
				strcpy(buf, "WEBSTOP");
				break;
			case DIK_WEBFORWARD:
				strcpy(buf, "WEBFORWARD");
				break;
			case DIK_WEBBACK:
				strcpy(buf, "WEBBACK");
				break;
			case DIK_MYCOMPUTER:
				strcpy(buf, "MYCOMPUTER");
				break;
			case DIK_MAIL:
				strcpy(buf, "MAIL");
				break;
			case DIK_MEDIASELECT:
				strcpy(buf, "MEDIASELECT");
				break;
			default:
				strcpy(buf, "未知");
				break;
			}
	} else {
		joyIdx = (key >> 16) & 0xff;
		if (key & KEY_JOY_KEY1) {
			sprintf(buf, "JOY%d_%d", joyIdx, (key & 0xff));
		} else if (key & KEY_JOY_KEY2) {
			switch (key & 0xFF00) {
			case CASE0: sprintf(buf, "JOY%d_ANL%d_%s", joyIdx, key & 7, "KEY+"); break;
			case CASE1: sprintf(buf, "JOY%d_ANL%d_%s", joyIdx, key & 7, "KEY-"); break;
			}
		} else if (key & KEY_JOY_POV) {
			switch (key & 0xFF00) {
			case CASE0: sprintf(buf, "JOY%d_DIG%d_%s", joyIdx, key & 3, "上");      break;
			case CASE1: sprintf(buf, "JOY%d_DIG%d_%s", joyIdx, key & 3, "右");   break;
			case CASE2: sprintf(buf, "JOY%d_DIG%d_%s", joyIdx, key & 3, "下");    break;
			case CASE3: sprintf(buf, "JOY%d_DIG%d_%s", joyIdx, key & 3, "左");    break;
			}
		}
	}

	return buf;
}

void RemoveUsedKey(u32 key, JOY *joy) {
#define REMOVEUSEDKEY(KeyName) if (joy->KeyName == key) joy->KeyName = 0
	REMOVEUSEDKEY(UP);
	REMOVEUSEDKEY(DOWN);
	REMOVEUSEDKEY(LEFT);
	REMOVEUSEDKEY(RIGHT);
	REMOVEUSEDKEY(A);
	REMOVEUSEDKEY(B);
	REMOVEUSEDKEY(C);
	REMOVEUSEDKEY(D);
	REMOVEUSEDKEY(X);
	REMOVEUSEDKEY(Y);
	REMOVEUSEDKEY(Z);
	REMOVEUSEDKEY(LTRIG);
	REMOVEUSEDKEY(RTRIG);
	REMOVEUSEDKEY(START);
	REMOVEUSEDKEY(STICK1UP);
	REMOVEUSEDKEY(STICK1DOWN);
	REMOVEUSEDKEY(STICK1LEFT);
	REMOVEUSEDKEY(STICK1RIGHT);
	REMOVEUSEDKEY(STICK2UP);
	REMOVEUSEDKEY(STICK2DOWN);
	REMOVEUSEDKEY(STICK2LEFT);
	REMOVEUSEDKEY(STICK2RIGHT);
}

void UpdatePadEdits(HWND hwnd, JOY *joy, PadControl *padControl) {
	SetDlgItemText(hwnd, padControl->EditUp, GetKeyName(joy->UP));
	SetDlgItemText(hwnd, padControl->EditDown, GetKeyName(joy->DOWN));
	SetDlgItemText(hwnd, padControl->EditLeft, GetKeyName(joy->LEFT));
	SetDlgItemText(hwnd, padControl->EditRight, GetKeyName(joy->RIGHT));
	SetDlgItemText(hwnd, padControl->EditA, GetKeyName(joy->A));
	SetDlgItemText(hwnd, padControl->EditB, GetKeyName(joy->B));
	SetDlgItemText(hwnd, padControl->EditC, GetKeyName(joy->C));
	SetDlgItemText(hwnd, padControl->EditD, GetKeyName(joy->D));
	SetDlgItemText(hwnd, padControl->EditX, GetKeyName(joy->X));
	SetDlgItemText(hwnd, padControl->EditY, GetKeyName(joy->Y));
	SetDlgItemText(hwnd, padControl->EditZ, GetKeyName(joy->Z));
	SetDlgItemText(hwnd, padControl->EditLTrig, GetKeyName(joy->LTRIG));
	SetDlgItemText(hwnd, padControl->EditRTrig, GetKeyName(joy->RTRIG));
	SetDlgItemText(hwnd, padControl->EditStart, GetKeyName(joy->START));
	SetDlgItemText(hwnd, padControl->EditS1Up, GetKeyName(joy->STICK1UP));
	SetDlgItemText(hwnd, padControl->EditS1Down, GetKeyName(joy->STICK1DOWN));
	SetDlgItemText(hwnd, padControl->EditS1Left, GetKeyName(joy->STICK1LEFT));
	SetDlgItemText(hwnd, padControl->EditS1Right, GetKeyName(joy->STICK1RIGHT));
	SetDlgItemText(hwnd, padControl->EditS2Up, GetKeyName(joy->STICK2UP));
	SetDlgItemText(hwnd, padControl->EditS2Down, GetKeyName(joy->STICK2DOWN));
	SetDlgItemText(hwnd, padControl->EditS2Left, GetKeyName(joy->STICK2LEFT));
	SetDlgItemText(hwnd, padControl->EditS2Right, GetKeyName(joy->STICK2RIGHT));
}

static DWORD enableTimer = 0;
static DWORD timerCount = 0;
static DWORD currentEditID = 0;
static DWORD currentButtonID = 0;
static u32  *key = 0;

bool CheckButton(HWND hwnd, int param, int ButtonID, int EditID, u32 *aKey) {
	if (ButtonID != param) return false;
	if (GetKeyState(VK_SHIFT) < 0) {
		*aKey = 0;
		SetDlgItemText(hwnd, EditID, GetKeyName(0));
		return true;
	}
	if (enableTimer) SetDlgItemText(hwnd, currentEditID, GetKeyName(*key));
	key = aKey;
	enableTimer = 1;
	timerCount = GetTickCount();
	currentEditID = EditID;
	currentButtonID = ButtonID;
	SetFocus(GetDlgItem(hwnd, EditID));
	SendMessage(hwnd, DM_SETDEFID, (WPARAM)IDC_FAKE, (LPARAM)0);
	return true;
}

void EndButtonInputScan(HWND hwnd) {
	SetDlgItemText(hwnd, currentEditID, GetKeyName(*key));
	SetFocus(GetDlgItem(hwnd, currentButtonID));
	SendMessage(hwnd, DM_SETDEFID, (WPARAM)IDOK, (LPARAM)0);
}
bool CheckPad(HWND hwnd, int param, int joyIdx) {
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonUp, padControls[joyIdx].EditUp, &tuneCfg.joy[joyIdx].UP))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonDown, padControls[joyIdx].EditDown, &tuneCfg.joy[joyIdx].DOWN))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonLeft, padControls[joyIdx].EditLeft, &tuneCfg.joy[joyIdx].LEFT))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonRight, padControls[joyIdx].EditRight, &tuneCfg.joy[joyIdx].RIGHT))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonA, padControls[joyIdx].EditA, &tuneCfg.joy[joyIdx].A))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonB, padControls[joyIdx].EditB, &tuneCfg.joy[joyIdx].B))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonC, padControls[joyIdx].EditC, &tuneCfg.joy[joyIdx].C))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonD, padControls[joyIdx].EditD, &tuneCfg.joy[joyIdx].D))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonX, padControls[joyIdx].EditX, &tuneCfg.joy[joyIdx].X))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonY, padControls[joyIdx].EditY, &tuneCfg.joy[joyIdx].Y))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonZ, padControls[joyIdx].EditZ, &tuneCfg.joy[joyIdx].Z))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonLTrig, padControls[joyIdx].EditLTrig, &tuneCfg.joy[joyIdx].LTRIG))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonRTrig, padControls[joyIdx].EditRTrig, &tuneCfg.joy[joyIdx].RTRIG))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonStart, padControls[joyIdx].EditStart, &tuneCfg.joy[joyIdx].START))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS1Up, padControls[joyIdx].EditS1Up, &tuneCfg.joy[joyIdx].STICK1UP))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS1Down, padControls[joyIdx].EditS1Down, &tuneCfg.joy[joyIdx].STICK1DOWN))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS1Left, padControls[joyIdx].EditS1Left, &tuneCfg.joy[joyIdx].STICK1LEFT))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS1Right, padControls[joyIdx].EditS1Right, &tuneCfg.joy[joyIdx].STICK1RIGHT))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS2Up, padControls[joyIdx].EditS2Up, &tuneCfg.joy[joyIdx].STICK2UP))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS2Down, padControls[joyIdx].EditS2Down, &tuneCfg.joy[joyIdx].STICK2DOWN))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS2Left, padControls[joyIdx].EditS2Left, &tuneCfg.joy[joyIdx].STICK2LEFT))
		return true;
	if (CheckButton(hwnd, param, padControls[joyIdx].ButtonS2Right, padControls[joyIdx].EditS2Right, &tuneCfg.joy[joyIdx].STICK2RIGHT))
		return true;

	return false;
}

BOOL CALLBACK Configure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int i;

	switch (msg) {
	case WM_INITDIALOG:
	{
		for (i = 0; i < MAX_JOYS; i++)
			UpdatePadEdits(hwnd, &tuneCfg.joy[i], &padControls[i]);

		SetTimer(hwnd, 1, 50, NULL);
		return TRUE;
	}

	case WM_TIMER:
		if (enableTimer) {
			u32 Key = GetKeyDevice();

			if (Key) {
				enableTimer = 0;
				for (i = 0; i < MAX_JOYS; i++) {
					RemoveUsedKey(Key, &tuneCfg.joy[i]);
					UpdatePadEdits(hwnd, &tuneCfg.joy[i], &padControls[i]);
				}
				*key = Key;
				EndButtonInputScan(hwnd);
				return TRUE;
			}

			if ((GetTickCount() - timerCount) / 1000 <= 10) {
				char buf[64];

				sprintf(buf, "timeout: %d", 10 - (GetTickCount() - timerCount) / 1000);
				SetDlgItemText(hwnd, currentEditID, buf);
			} else {
				enableTimer = 0;
				EndButtonInputScan(hwnd);
			}
		}
		return TRUE;

	case WM_COMMAND:
		for (i = 0; i < MAX_JOYS; i++)
			if (CheckPad(hwnd, LOWORD(wParam), i))
				return TRUE;
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


bool SetConfig() {
	enableTimer = 0;
	timerCount = 0;
	currentEditID = 0;
	currentButtonID = 0;
	key = NULL;

	tuneCfg = cfg;
	if (DialogBox(hinstance, MAKEINTRESOURCE(IDD_DIALOG), GetActiveWindow(), (DLGPROC)Configure) == IDOK) {
		cfg = tuneCfg;
		SaveConfig();
		CalcJoyCaps();
		return true;
	}
	CalcJoyCaps();
	return false;
}

void IniFile_LoadJoy(IniFile* iniFile, char*joyName, JOY* joy) {
	joy->UP = IniFile_getLong(iniFile, joyName, "上");
	joy->DOWN = IniFile_getLong(iniFile, joyName, "下");
	joy->LEFT = IniFile_getLong(iniFile, joyName, "左");
	joy->RIGHT = IniFile_getLong(iniFile, joyName, "右");
	joy->A = IniFile_getLong(iniFile, joyName, "A");
	joy->B = IniFile_getLong(iniFile, joyName, "B");
	joy->C = IniFile_getLong(iniFile, joyName, "C");
	joy->D = IniFile_getLong(iniFile, joyName, "D");
	joy->X = IniFile_getLong(iniFile, joyName, "X");
	joy->Y = IniFile_getLong(iniFile, joyName, "Y");
	joy->Z = IniFile_getLong(iniFile, joyName, "Z");
	joy->LTRIG = IniFile_getLong(iniFile, joyName, "LTRIG");
	joy->RTRIG = IniFile_getLong(iniFile, joyName, "RTRIG");
	joy->START = IniFile_getLong(iniFile, joyName, "START");
	joy->STICK1UP = IniFile_getLong(iniFile, joyName, "STICK1UP");
	joy->STICK1DOWN = IniFile_getLong(iniFile, joyName, "STICK1DOWN");
	joy->STICK1LEFT = IniFile_getLong(iniFile, joyName, "STICK1LEFT");
	joy->STICK1RIGHT = IniFile_getLong(iniFile, joyName, "STICK1RIGHT");
	joy->STICK2UP = IniFile_getLong(iniFile, joyName, "STICK2UP");
	joy->STICK2DOWN = IniFile_getLong(iniFile, joyName, "STICK2DOWN");
	joy->STICK2LEFT = IniFile_getLong(iniFile, joyName, "STICK2LEFT");
	joy->STICK2RIGHT = IniFile_getLong(iniFile, joyName, "STICK2RIGHT");
}

void IniFile_SaveJoy(IniFile* iniFile, char*joyName, JOY* joy) {
	IniFile_setLong(iniFile, joyName, "上", joy->UP);
	IniFile_setLong(iniFile, joyName, "下", joy->DOWN);
	IniFile_setLong(iniFile, joyName, "左", joy->LEFT);
	IniFile_setLong(iniFile, joyName, "右", joy->RIGHT);
	IniFile_setLong(iniFile, joyName, "A", joy->A);
	IniFile_setLong(iniFile, joyName, "B", joy->B);
	IniFile_setLong(iniFile, joyName, "C", joy->C);
	IniFile_setLong(iniFile, joyName, "D", joy->D);
	IniFile_setLong(iniFile, joyName, "X", joy->X);
	IniFile_setLong(iniFile, joyName, "Y", joy->Y);
	IniFile_setLong(iniFile, joyName, "Z", joy->Z);
	IniFile_setLong(iniFile, joyName, "LTRIG", joy->LTRIG);
	IniFile_setLong(iniFile, joyName, "RTRIG", joy->RTRIG);
	IniFile_setLong(iniFile, joyName, "START", joy->START);
	IniFile_setLong(iniFile, joyName, "STICK1UP", joy->STICK1UP);
	IniFile_setLong(iniFile, joyName, "STICK1DOWN", joy->STICK1DOWN);
	IniFile_setLong(iniFile, joyName, "STICK1LEFT", joy->STICK1LEFT);
	IniFile_setLong(iniFile, joyName, "STICK1RIGHT", joy->STICK1RIGHT);
	IniFile_setLong(iniFile, joyName, "STICK2UP", joy->STICK2UP);
	IniFile_setLong(iniFile, joyName, "STICK2DOWN", joy->STICK2DOWN);
	IniFile_setLong(iniFile, joyName, "STICK2LEFT", joy->STICK2LEFT);
	IniFile_setLong(iniFile, joyName, "STICK2RIGHT", joy->STICK2RIGHT);
}

bool LoadConfig(bool autoSetConfig) {
	IniFile iniFile;
	char padName[10];
	int i;

	memset(&cfg, 0, sizeof(CFG));
	if (!IniFile_open(&iniFile, PAD_MODULE_NAME)) return false;

	if (!IniFile_exist(&iniFile)) {
		if (!autoSetConfig)
			return true;
		MessageBox(GetActiveWindow(), "padDemul 未配置", PAD_MODULE_NAME, MB_ICONINFORMATION);
		return SetConfig();
	}
	for (i = 0; i < MAX_JOYS; i++) {
		sprintf(padName, "PAD%i", i);
		IniFile_LoadJoy(&iniFile, padName, &cfg.joy[i]);
	}
	CalcJoyCaps();
	return true;
}

void SaveConfig() {
	IniFile iniFile;
	char padName[10];
	int i;
	if (!IniFile_open(&iniFile, PAD_MODULE_NAME)) return;

	for (i = 0; i < MAX_JOYS; i++) {
		sprintf(padName, "PAD%i", i);
		IniFile_SaveJoy(&iniFile, padName, &cfg.joy[i]);
	}
}

void CalcJoyCaps() {
	int i;
	u32 caps;
#ifdef _DEBUG
	char buf[32];
#endif

	for (i = 0; i < MAX_JOYS; i++) {
		caps = /*0xfe060f00 +*/ 0xfe000000 | CONT_Z | CONT_Y | CONT_X | CONT_D | CONT_LEFT_TRIG | CONT_STICK1X;

		if (cfg.joy[i].UP != 0)
			caps |= CONT_DPAD_UP;
		if (cfg.joy[i].DOWN != 0)
			caps |= CONT_DPAD_DOWN;
		if (cfg.joy[i].LEFT != 0)
			caps |= CONT_DPAD_LEFT;
		if (cfg.joy[i].RIGHT != 0)
			caps |= CONT_DPAD_RIGHT;
		if (cfg.joy[i].A != 0)
			caps |= CONT_A;
		if (cfg.joy[i].B != 0)
			caps |= CONT_B;
		if (cfg.joy[i].C != 0)
			caps |= CONT_C;
		if (cfg.joy[i].D != 0)
			caps |= CONT_D;
		if (cfg.joy[i].X != 0)
			caps |= CONT_X;
		if (cfg.joy[i].Y != 0)
			caps |= CONT_Y;
		if (cfg.joy[i].Z != 0)
			caps |= CONT_Z;
		if ((cfg.joy[i].STICK1UP != 0) && (cfg.joy[i].STICK1DOWN != 0))
			caps |= CONT_STICK1Y;
		if ((cfg.joy[i].STICK1LEFT != 0) && (cfg.joy[i].STICK1RIGHT != 0))
			caps |= CONT_STICK1X;
		if ((cfg.joy[i].STICK2UP != 0) && (cfg.joy[i].STICK2DOWN != 0))
			caps |= CONT_STICK2Y;
		if ((cfg.joy[i].STICK2LEFT != 0) && (cfg.joy[i].STICK2RIGHT != 0))
			caps |= CONT_STICK2X;
		if (cfg.joy[i].LTRIG != 0)
			caps |= CONT_LEFT_TRIG;
		if (cfg.joy[i].RTRIG != 0)
			caps |= CONT_RIGHT_TRIG;

		JoyCaps[i] = caps;

#define SHOW_CAP(Cap)       if ((caps & Cap) == Cap) { sprintf(buf, # Cap " | "); OutputDebugString(buf); \
}

#ifdef _DEBUG
		SHOW_CAP(CONT_C)
		SHOW_CAP(CONT_B)
		SHOW_CAP(CONT_A)
		SHOW_CAP(CONT_START)
		SHOW_CAP(CONT_DPAD_UP)
		SHOW_CAP(CONT_DPAD_DOWN)
		SHOW_CAP(CONT_DPAD_LEFT)
		SHOW_CAP(CONT_DPAD_RIGHT)
		SHOW_CAP(CONT_Z)
		SHOW_CAP(CONT_Y)
		SHOW_CAP(CONT_X)
		SHOW_CAP(CONT_D)
		SHOW_CAP(CONT_DPAD2_UP)
		SHOW_CAP(CONT_DPAD2_DOWN)
		SHOW_CAP(CONT_DPAD2_LEFT)
		SHOW_CAP(CONT_DPAD2_RIGHT)
		SHOW_CAP(CONT_RIGHT_TRIG)
		SHOW_CAP(CONT_LEFT_TRIG)
		SHOW_CAP(CONT_STICK1X)
		SHOW_CAP(CONT_STICK1Y)
		SHOW_CAP(CONT_STICK2X)
		SHOW_CAP(CONT_STICK2Y)

		sprintf(buf, "\n");
		OutputDebugString(buf);
#endif
	}
}
