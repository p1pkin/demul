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

#ifndef __DYNAREC_H__
#define __DYNAREC_H__

#include "types.h"

typedef struct {
	u32 pc;
	u32 cycle;
	u32 compile;
	u32 usefpu;
	u32 sz;
	u32 pr;
	u32 need_test_block;
	u32 start_addr;
	u16 start_code;
	u32 finish_addr;
	u16 finish_code;
	u8 *link;
	u32 need_linking_block1;
	u32 block_pc1;
	u32 need_linking_block2;
	u32 block_pc2;
} BLOCK_INFO;

BLOCK_INFO block_info;

#define CONST_UNMAPED   0
#define CONST_MAPPED    1

#define isMappedConst(n) (ConstantMap[(n)].isMapped == CONST_MAPPED)
#define invalidConst(n)  (ConstantMap[(n)].isMapped = CONST_UNMAPED)

#define MapConst(n, val) \
	unMapSh4Reg(n);	\
	ConstantMap[(n)].isMapped = (CONST_MAPPED);	\
	ConstantMap[(n)].constant = (val);

typedef struct {
	u32 isMapped;
	u32 constant;
} CONSTANT_MAP;

CONSTANT_MAP ConstantMap[16];

int  recOpen();
void recExecute();
void recStep(u32 pc);
void *recCompile();
void recCompileBlock(u32 pc);
void recAnalyzeBlock(u32 pc);
void recClose();

void statFlush();

#endif