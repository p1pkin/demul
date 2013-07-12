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

#ifndef __TYPES_H__
#define __TYPES_H__

#ifndef INLINE
#define INLINE      __inline
#endif

#ifndef FASTCALL
#define FASTCALL    __fastcall
#endif

#ifndef CDECL
#define CDECL       __cdecl
#endif

#ifndef STDCALL
#define STDCALL     __stdcall
#endif

#if !defined(__BOOL_DEFINED)
typedef unsigned __int8 bool;
#define true 1
#define false 0
#endif

#if _MSC_VER < 1400
#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))
#endif

typedef __int8 s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#endif
