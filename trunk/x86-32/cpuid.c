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

#include "cpuid.h"

// These are the bit flags that get set on calling cpuid
// with register eax set to 1
#define _MMX_FEATURE_BIT  0x00800000
#define _SSE_FEATURE_BIT  0x02000000
#define _SSE2_FEATURE_BIT 0x04000000
#define _SSE3_FEATURE_BIT 1

// This bit is set when cpuid is called with
// register set to 80000001h (only applicable to AMD)
#define _3DNOW_FEATURE_BIT 0x80000000

void cpuid() {
	u32 dwFeature = 0;
	u32 dwFeature2 = 0;
	u32 dwExt = 0;
	_asm {
		push ebx
		push ecx
		push edx

		// get the Standard bits
		mov eax, 1
		cpuid
		mov dwFeature, edx
		mov dwFeature2, ecx

		// get AMD-specials
		mov eax, 80000000h
		cpuid
		cmp eax, 80000000h
		jc notamd
		mov eax, 80000001h
		cpuid
		mov dwExt, edx

 notamd:
		pop ecx
		pop ebx
		pop edx
	}

	cpuMMX = (dwFeature & _MMX_FEATURE_BIT) != 0;
	cpu3DNOW = (dwExt & _3DNOW_FEATURE_BIT) != 0;
	cpuSSE = (dwFeature & _SSE_FEATURE_BIT) != 0;
	cpuSSE2 = (dwFeature & _SSE2_FEATURE_BIT) != 0;
	cpuSSE3 = (dwFeature2 & _SSE3_FEATURE_BIT) != 0;
}
