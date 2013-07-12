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

#include "gpuregs.h"
#include "gpuprim.h"
#include "gpudemul.h"
#include "gputransfer.h"
#include "gpurenderer.h"
#include "gputexture.h"

#ifdef REGS_DEBUG

struct REGINF {
	u32 regaddr;
	char *regname;
};

static struct REGINF reg_names[0x200 >> 2] =
{
	{ 0x005F8000, "ID" },
	{ 0x005F8004, "REVISION" },
	{ 0x005F8008, "SOFTRESET" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8014, "STARTRENDER" },
	{ 0x005F8018, "TEST_SELECT" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8020, "PARAM_BASE" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F802C, "REGION_BASE" },
	{ 0x005F8030, "SPAN_SORT_CFG" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8040, "VO_BORDER_COL" },
	{ 0x005F8044, "FB_R_CTRL" },
	{ 0x005F8048, "FB_W_CTRL" },
	{ 0x005F804C, "FB_W_LINESTRIDE" },
	{ 0x005F8050, "FB_R_SOF1" },
	{ 0x005F8054, "FB_R_SOF2" },
	{ 0x00000000, "-NA-" },
	{ 0x005F805C, "FB_R_SIZE" },
	{ 0x005F8060, "FB_W_SOF1" },
	{ 0x005F8064, "FB_W_SOF2" },
	{ 0x005F8068, "FB_X_CLIP" },
	{ 0x005F806C, "FB_Y_CLIP" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8074, "FPU_SHAD_SCALE" },
	{ 0x005F8078, "FPU_CULL_VAL" },
	{ 0x005F807C, "FPU_PARAM_CFG" },
	{ 0x005F8080, "HALF_OFFSET" },
	{ 0x005F8084, "FPU_PERP_VAL" },
	{ 0x005F8088, "ISP_BACKGND_D" },
	{ 0x005F808C, "ISP_BACKGND_T" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8098, "ISP_FEED_CFG" },
	{ 0x00000000, "-NA-" },
	{ 0x005F80A0, "SDRAM_REFRESH" },
	{ 0x005F80A4, "SDRAM_ARB_CFG" },
	{ 0x005F80A8, "SDRAM_CFG" },
	{ 0x00000000, "-NA-" },
	{ 0x005F80B0, "FOG_COL_RAM" },
	{ 0x005F80B4, "FOG_COL_VERT" },
	{ 0x005F80B8, "FOG_DENSITY" },
	{ 0x005F80BC, "FOG_CLAMP_MAX" },
	{ 0x005F80C0, "FOG_CLAMP_MIN" },
	{ 0x005F80C4, "SPG_TRIGGER_POS" },
	{ 0x005F80C8, "SPG_HBLANK_INT" },
	{ 0x005F80CC, "SPG_VBLANK_INT" },
	{ 0x005F80D0, "SPG_CONTROL" },
	{ 0x005F80D4, "SPG_HBLANK" },
	{ 0x005F80D8, "SPG_LOAD" },
	{ 0x005F80DC, "SPG_VBLANK" },
	{ 0x005F80E0, "SPG_WIDTH" },
	{ 0x005F80E4, "TEXT_CONTROL" },
	{ 0x005F80E8, "VO_CONTROL" },
	{ 0x005F80EC, "VO_STARTX" },
	{ 0x005F80F0, "VO_STARTY" },
	{ 0x005F80F4, "SCALER_CTL" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8108, "PAL_RAM_CTRL" },
	{ 0x005F810C, "SPG_STATUS" },
	{ 0x005F8110, "FB_BURSTCTRL" },
	{ 0x005F8114, "FB_C_SOF" },
	{ 0x005F8118, "Y_COEFF" },
	{ 0x005F811C, "PT_ALPHA_REF" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8124, "TA_OL_BASE" },
	{ 0x005F8128, "TA_ISP_BASE" },
	{ 0x005F812C, "TA_OL_LIMIT" },
	{ 0x005F8130, "TA_ISP_LIMIT" },
	{ 0x005F8134, "TA_NEXT_OPB" },
	{ 0x005F8138, "TA_ITP_CURRENT" },
	{ 0x005F813C, "TA_GLOB_TILE_CLIP" },
	{ 0x005F8140, "TA_ALLOC_CTRL" },
	{ 0x005F8144, "TA_LIST_INIT" },
	{ 0x005F8148, "TA_YUV_TEX_BASE" },
	{ 0x005F814C, "TA_YUV_TEX_CTRL" },
	{ 0x005F8150, "TA_YUV_TEX_CNT" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x005F8160, "TA_LIST_CONT" },
	{ 0x005F8164, "TA_NEXT_OPB_INIT" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" },
	{ 0x00000000, "-NA-" }
//	{0x005F8200, "FOG_TABLE"},
//	{0x005F83FC, "-"},
//	{0x005F8600, "TA_OL_POINTERS"},
//	{0x005F8F5C, "-"},
//	{0x005F9000, "PALETTE_RAM"},
//	{0x005F9FFC, "-"},
};

static const char *get_reg_name(u32 reg) {
	char *fog_msg = "FOG_TABLE_VALUE";
	char *taol_msg = "TA_OL_POINTERS_VALUE";
	char *pal_msg = "PALETTE_RAM_VALUE";
	char *def = "-na-";

	if ((reg & 0xffff) < 0x8200)
		return reg_names[(reg & 0x1FF) >> 2].regname;
	else {
		reg = reg & 0xffff;
		if (reg >= 0x8200 && reg <= 0x83FC) return fog_msg;
		if (reg >= 0x8600 && reg <= 0x8F5C) return taol_msg;
		if (reg >= 0x9000 && reg <= 0x9FFC) return pal_msg;
		return def;
	}
}

#endif


u32 FASTCALL gpuRead32(u32 mem) {
	#ifdef REGS_DEBUG
	DEMUL_printf("READF FROM REG[%04x]=%08x; %s\n", mem & 0xFFFF, REGS32(mem & 0xFFFF), get_reg_name(mem & 0xFFFF));
	#endif

	switch (mem & 0x007fffff) {
	case 0x005f8144: return 0;
	case 0x005f8138: return REGS32(0x8128);
	default:         return REGS32(mem & 0xFFFF);
	}
}

void FASTCALL gpuWrite32(u32 mem, u32 value) {
	REGS32(mem & 0xffff) = value;

	#ifdef REGS_DEBUG
	DEMUL_printf("WRITE TO REG[%04x]=%08x; %s\n", mem & 0xFFFF, value, get_reg_name(mem));
	#endif

	switch (mem & 0x007fffff) {
	case 0x005f8008:
		if (value & 1) {
			listtype = 0;
			completed = 1;
		}
		break;

	case 0x005f8014:
		if (value)
			if (REGS32(0x8060) & 0x01000000)
				glInternalFlush(RENDER_TARGET_TEXTURE);
			else
				glInternalFlush(RENDER_TARGET_FRAMEBUFFER);
		renderingison = 1;
		break;

	case 0x005f8060:
		if (!(value & 0x01000000)) {
			u32 *pix = (u32*)&VRAM[(value & 0x007FFFFF)];
			*pix = 0xBABAC0C0;
			pix = (u32*)&VRAM[(value & 0x007FFFFF) + 640 * 480];
			*pix = 0xBABAC0C0;
		}
		break;

	case 0x005f80e8:
		setDisplayMode();
		break;

	case 0x005f8144:
		if (value & 0x80000000) {
			listtype = 0;
			completed = 1;
		}
		break;
	default:
//		DEMUL_printf("REG[%04X]=%04x\n",mem&0xffff,value);
		break;
	}
}
