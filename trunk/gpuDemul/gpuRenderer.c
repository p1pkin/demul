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

#include "gpurenderer.h"
#include "gputransfer.h"
#include "gputexture.h"
#include "gpuprim.h"
#include "gpuregs.h"
#include "gpudemul.h"
#include "config.h"

HDC hdc = NULL;
HGLRC hrc = NULL;
int backbufname;
u32 isFramebufferRendered = 0;
u32 TextureAddress = 0;

PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointer = NULL;

bool isExtensionSupported(const char *extension) {
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte*)strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	extensions = glGetString(GL_EXTENSIONS);

	start = extensions;

	for (;; ) {
		where = (GLubyte*)strstr((const char*)start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return TRUE;

		start = terminator;
	}

	return false;
}

static u32 w;
static u32 h;


void setDisplayMode(void) {
	w = (REGS32(0x80E8) & 0x100) ? 320 : 640;
	h = (REGS32(0x80E8) & 0x100) ? 240 : 480;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, 0, RENDER_VIEW_DISTANCE);
	glMatrixMode(GL_MODELVIEW);
}

GLenum getSnapshot(u8 *buf) {
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);
	return glGetError();
}

int InitialOpenGL() {
	int pfmt;

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),		// size of this pfd
		1,									// version number
		PFD_DRAW_TO_WINDOW |				// support window
		PFD_SUPPORT_OPENGL |				// support OpenGL
		PFD_DOUBLEBUFFER,					// double buffered
		PFD_TYPE_RGBA,						// RGBA type
		32,									// color depth
		0, 0, 0, 0, 0, 0,					// color bits ignored
		0,									// no alpha buffer
		0,									// shift bit ignored
		0,									// no accumulation buffer
		0, 0, 0, 0,							// accum bits ignored
		24,									// z-buffer
		0,									// no stencil buffer
		0,									// no auxiliary buffer
		PFD_MAIN_PLANE,						// main layer
		0,									// reserved
		0, 0, 0								// layer masks ignored
	};

	if ((hdc = GetDC(gDemulInfo->hGpuWnd)) == 0) {
		MessageBox(NULL, "GetDC failed", "Error", MB_OK);
		DeleteOpenGL();
		return 0;
	}

	if ((pfmt = ChoosePixelFormat(hdc, &pfd)) == 0) {
		MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
		DeleteOpenGL();
		return 0;
	}

	if ((SetPixelFormat(hdc, pfmt, &pfd)) == 0) {
		MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
		DeleteOpenGL();
		return 0;
	}

	if ((hrc = wglCreateContext(hdc)) == 0) {
		MessageBox(NULL, "wglCreateContext failed", "Error", MB_OK);
		DeleteOpenGL();
		return 0;
	}

	if ((wglMakeCurrent(hdc, hrc)) == 0) {
		MessageBox(NULL, "wglMakeCurrent failed", "Error", MB_OK);
		DeleteOpenGL();
		return 0;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearDepth(0.0f);
	glDepthRange(0.0f, 1.0f);

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);

	glGenTextures(1, &backbufname);
	glBindTexture(GL_TEXTURE_2D, backbufname);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, &VRAM[0]);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), &pVertex->x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(VERTEX), &pVertex->tu);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), &pVertex->color);

	if (isExtensionSupported("GL_EXT_secondary_color")) {
		glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
		glSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTEREXTPROC)wglGetProcAddress("glSecondaryColorPointerEXT");
		glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), &pVertex->extcolor);
	}

	return 1;
}

void DeleteOpenGL() {
	wglMakeCurrent(NULL, NULL);

	if (hrc) {
		wglDeleteContext(hrc);
		hrc = NULL;
	}

	if (hdc) {
		ReleaseDC(gDemulInfo->hGpuWnd, hdc);
		hdc = NULL;
	}
}

void SetupCullMode() {
	static const unsigned int cull[] = { 0, 0, GL_CCW, GL_CW };

	if (polygon.isp.cullmode > 1) {
		glEnable(GL_FRONT_FACE);
		glFrontFace(cull[polygon.isp.cullmode]);
	} else {
		glDisable(GL_FRONT_FACE);
	}
}

void SetupShadeMode() {
	if (polygon.pcw.gouraud) glShadeModel(GL_SMOOTH);
	else glShadeModel(GL_FLAT);
}

void SetupZBuffer() {
	static const unsigned int depthfunc[] =
	{ GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };

	if (polygon.isp.zwritedis) glDepthMask(GL_FALSE);
	else glDepthMask(GL_TRUE);

	glDepthFunc(depthfunc[polygon.isp.depthmode]);
}

void SetupAlphaTest() {
	if (polygon.tsp.usealpha) glEnable(GL_ALPHA_TEST);
	else glDisable(GL_ALPHA_TEST);
	if (polygon.pcw.listtype == 4) glAlphaFunc(GL_GEQUAL, (float)REGS[0x811C] / 255.0f);
	else glAlphaFunc(GL_ALWAYS, 0.0f);
}

void SetupAlphaBlend() {
	static const unsigned int sfactor[] = { GL_ZERO, GL_ONE, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
											GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };

	static const unsigned int dfactor[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA,
											GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };

	glBlendFunc(sfactor[polygon.tsp.srcinstr], dfactor[polygon.tsp.dstinstr]);
}

void SetupTexture() {
	if (polygon.pcw.texture) {
		GLuint name = (GLuint)CreateTexture();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, name);

		if (polygon.pcw.offset) glEnable(GL_COLOR_SUM);
		else glDisable(GL_COLOR_SUM);

		if (polygon.tsp.clampuv & 1) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		} else if (polygon.tsp.flipuv & 1) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}

		if (polygon.tsp.clampuv & 2) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else if (polygon.tsp.flipuv & 2) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		switch (polygon.tsp.shadinstr) {
		case 0:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			break;

		case 1:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

			if (polygon.tsp.ignoretexa) {
				glDisable(GL_ALPHA_TEST);
//				static const float color[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
//
//				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
//				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,    GL_REPLACE);
//				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,    GL_CONSTANT);
//				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,   GL_SRC_ALPHA);
			} else {
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			}
			break;

		case 2:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			break;

		case 3:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

			if (polygon.tsp.ignoretexa) {
				glDisable(GL_ALPHA_TEST);
//				static const float color[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
//				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
//				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,    GL_MODULATE);
//				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,    GL_CONSTANT);
//				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA,    GL_PRIMARY_COLOR);
//				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,   GL_SRC_ALPHA);
//				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,   GL_SRC_ALPHA);
			} else {
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
			}
			break;
		}
		if (polygon.tsp.filtermode) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

//void CONV_8888_REV_TO_8888(u32 *src, u32 *dst)
//{
//	u32 i;
//	for(i=0; i<w*h; i++)
//	{
//		u32 color = *(src++);
//		*(dst++) =	((color & 0x00ff0000) >> 16) |
//					((color & 0x000000ff) << 16) |
//					((color & 0xff00ff00) << 0 );
//	}
//}

void CONV_8888_REV_TO_8888(u32 *src, u32 *dst, u32 sizef) {
	static __int64 mask1 = 0x00FF000000FF0000;
	static __int64 mask2 = 0x000000FF000000FF;
	static __int64 mask3 = 0xFF00FF00FF00FF00;
	__asm {
		push esi
		push edi
		mov esi, src
		mov edi, dst
		mov ecx, dword ptr sizef
		movq mm3, qword ptr mask1
		movq mm4, qword ptr mask2
		movq mm5, qword ptr mask3
 _main_loop:
		movq mm0, [esi]
		movq mm1, mm0
		movq mm2, mm0
		pand mm0, mm3
		pand mm1, mm4
		pand mm2, mm5
		psrld mm0, 16
		pslld mm1, 16
		por mm0, mm1
		por mm0, mm2
			movq    [edi], mm0
		add esi, 8
		add edi, 8
		dec ecx
		jnz _main_loop
		emms
		pop edi
		pop esi
	}
}

//void CONV_0565_REV_TO_8888(u16 *src, u32 *dst)
//{
//	u32 i;
//	for(i=0; i<w*h; i++)
//	{
//		u32 color = *(src++);
//		*(dst++)  = ((color & 0x001f) << 19) |
//					((color & 0x07e0) << 5 ) |
//					((color & 0xf800) >> 8 ) |
//					((0xff000000    ) << 0);
//	}
//}

void CONV_0565_REV_TO_8888(u16 *src, u32 *dst, u32 sizef) {
	static __int64 mask1 = 0x0000001F0000001F;
	static __int64 mask2 = 0x000007E0000007E0;
	static __int64 mask3 = 0x0000F8000000F800;
	static __int64 mask4 = 0xFF000000FF000000;
	__asm {
		push esi
		push edi
		mov esi, src
		mov edi, dst
		mov ecx, dword ptr sizef
		movq mm3, qword ptr mask1
		movq mm4, qword ptr mask2
		movq mm5, qword ptr mask3
		movq mm6, qword ptr mask4
 _main_loop:
		movq mm0, [esi]
		movq mm1, mm0
		psrlq mm1, 32
		punpcklwd mm0, mm1
		movq mm7, mm0
		movq mm1, mm0
		movq mm2, mm0
		pand mm0, mm3
		pand mm1, mm4
		pand mm2, mm5
		pslld mm0, 19
		pslld mm1, 5
		psrld mm2, 8
		por mm0, mm1
		por mm0, mm2
		por mm0, mm6
			movq    [edi], mm0
		psrld mm7, 16
		movq mm1, mm7
		movq mm2, mm7
		pand mm7, mm3
		pand mm1, mm4
		pand mm2, mm5
		pslld mm7, 19
		pslld mm1, 5
		psrld mm2, 8
		por mm7, mm1
		por mm7, mm2
		por mm7, mm6
			movq    [edi + 8], mm7
		add edi, 16
		add esi, 8
		dec ecx
		jnz _main_loop
		emms
		pop edi
		pop esi
	}
}
/*
void CONV_8888_REV_TO_0565(u32 *src, u16 *dst, u32 w, u32 h)
{
    static __int64 mask1=0x00F8000000F80000;
    static __int64 mask2=0x0000FC000000FC00;
    static __int64 mask3=0x000000F8000000F8;
    __asm {
        push	esi
        push	edi
        push	ebx
        mov		esi, src
        mov		edi, dst
        mov		eax, dword ptr w
        mov		ecx, dword ptr h
        imul	ecx
        mov		edx, eax
        shl		edx, 1
        add		edi, edx
        shr		eax, 2
        mov		ecx, eax
        mov		edx, dword ptr w
        shl		edx, 1
//		mov		dword ptr w, edx
        sub		edi, edx
//		shl		edx, 1
        xor		ebx, ebx
        movq	mm3, qword ptr mask1
        movq	mm4, qword ptr mask2
        movq	mm5, qword ptr mask3
_main_loop:
        movq	mm0, [esi]
        movq	mm1, mm0
        movq	mm2, mm0
        pand	mm0, mm3
        pand	mm1, mm4
        pand	mm2, mm5
        psrld	mm0, 19
        psrld	mm1, 5
        pslld	mm2, 8
        por		mm0, mm1
        por		mm0, mm2
        movq	mm7, [esi + 8]
        movq	mm1, mm7
        movq	mm2, mm7
        pand	mm7, mm3
        pand	mm1, mm4
        pand	mm2, mm5
        psrld	mm7, 19
        psrld	mm1, 5
        pslld	mm2, 8
        por		mm7, mm1
        por		mm7, mm2
        pslld	mm7, 16
        por		mm0, mm7
        movq	mm1, mm0
        psrlq	mm1, 32
        punpcklwd mm0, mm1
        movq	[edi + ebx], mm0
        add		ebx, 8
        cmp		ebx, edx
        jb		next_value
        xor		ebx, ebx
        sub		edi, edx
next_value:
        add		esi, 16
        dec		ecx
        jnz		_main_loop
        emms
        pop		ebx
        pop		edi
        pop		esi
    }
}
*/
void glRenderFramebuffer(u32 backbuf, u32 pixelformat) {
	u32 pixels = w * h;
	u32 *buf = malloc(pixels << 2);

	glDisable(GL_BLEND);

	glClearColor((float)REGS[0x8042] / 255.0f,
				 (float)REGS[0x8041] / 255.0f,
				 (float)REGS[0x8040] / 255.0f,
				 (float)REGS[0x8043] / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (pixelformat) {
	case 0: break;
	case 1: CONV_0565_REV_TO_8888((u16*)&VRAM[backbuf], buf, pixels >> 2); break;
	case 2: break;
	case 3: CONV_8888_REV_TO_8888((u32*)&VRAM[backbuf], buf, pixels >> 1); break;
	}

	glBindTexture(GL_TEXTURE_2D, backbufname);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, (float)h / 512.0f); glVertex3f(0.0f, (float)h, 0.0f);
	glTexCoord2f((float)w / 1024.0f, (float)h / 512.0f); glVertex3f((float)w, (float)h, 0.0f);
	glTexCoord2f((float)w / 1024.0f, 0.0f); glVertex3f((float)w, 0.0f, 0.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	SwapBuffers(hdc);
	free(buf);
}

void flushPrim() {
	OBJECT *obj = &objList[objCount++];

	obj->vIndex = vIndex;
	obj->vCount = vCount;
	obj->midZ = ((maxz1 + minz1) / 2);
	obj->polygon = polygon;

	if ((maxz1) > maxz) maxz = maxz1;
	if ((minz1) < minz) minz = minz1;

//	DEMUL_printf(">flush Prim (vIndex=%d,vCount=%d,maxz1=%f,minz1=%f)\n",vIndex,vCount,maxz1,minz1);

	maxz1 = -10000000.0f;
	minz1 = 10000000.0f;
	vIndex += vCount;
	vCount = 0;
}

int comp_func(const void* a, const void *b) {
	OBJECT *obja = (OBJECT*)a;
	OBJECT *objb = (OBJECT*)b;

//    if((obja->polygon.pcw.listtype<objb->polygon.pcw.listtype)) return -1;
//	else if(obja->polygon.pcw.listtype>objb->polygon.pcw.listtype) return 1;
//	else if(obja->polygon.pcw.listtype==4)
//	{
//		if(obja->midZ>objb->midZ) return -1;
//		else if(obja->midZ<objb->midZ) return 1;
//		else return 0;
//	}
//	else if((obja->polygon.pcw.listtype==2)&&(!REGS32(0x8098))&&(!(REGS32(0x807c)&0x00200000)))
//	{
//		if(obja->midZ>objb->midZ) return -1;
//		else if(obja->midZ<objb->midZ) return 1;
//		else return 0;
//	}
//	else return 0;

	if (obja->polygon.tsp.usealpha < objb->polygon.tsp.usealpha) return -1;
	else if (obja->polygon.tsp.usealpha > objb->polygon.tsp.usealpha) return 1;
	else if ((obja->midZ > objb->midZ) && obja->polygon.tsp.usealpha && cfg.AlphasubSort) return -1;
	else if ((obja->midZ < objb->midZ) && obja->polygon.tsp.usealpha && cfg.AlphasubSort) return 1;
	else if ((obja->midZ > objb->midZ) && cfg.PunchsubSort) return -1;
	else if ((obja->midZ < objb->midZ) && cfg.PunchsubSort) return 1;
	else return 0;

//	if((obja->midZ>objb->midZ)) return -1;
//	else if((obja->midZ<objb->midZ)) return 1;
//	else return 0;
}

int listtypesort_func(const void* a, const void *b) {
	OBJECT *obja = (OBJECT*)a;
	OBJECT *objb = (OBJECT*)b;

	if ((obja->polygon.pcw.listtype < objb->polygon.pcw.listtype)) return -1;
	else if (obja->polygon.pcw.listtype > objb->polygon.pcw.listtype) return 1;
	else return 0;
}

int transsort_func(const void* a, const void *b) {
	OBJECT *obja = (OBJECT*)a;
	OBJECT *objb = (OBJECT*)b;

	if ((obja->polygon.pcw.listtype == 2) && (objb->polygon.pcw.listtype == 2)) {	//&&(!REGS32(0x8098))&&(!(REGS32(0x807c)&0x00200000)))
		if (obja->midZ > objb->midZ) return -1;
		else if (obja->midZ < objb->midZ) return 1;
	}
	return 0;
}

int punchsort_func(const void* a, const void *b) {
	OBJECT *obja = (OBJECT*)a;
	OBJECT *objb = (OBJECT*)b;

	if ((obja->polygon.pcw.listtype == 4) && (objb->polygon.pcw.listtype == 4)) {
		if (obja->midZ > objb->midZ) return -1;
		else if (obja->midZ < objb->midZ) return 1;
	}
	return 0;
}

void glInternalFlush(u32 target) {
	u32 i;
	u32 param_base = REGS32(0x8020) & 0x007FFFFF;
	u32 background = (REGS32(0x808C) & 0x00FFFFF8) >> 1;
	u32 address = (param_base + background) & 0x007FFFFF;

	flushPrim();
	NewStripList();

	BACKGROUND((u32*)&VRAM[address]);

	flushPrim();
	NewStripList();

	glClearColor((float)REGS[0x8042] / 255.0f,
				 (float)REGS[0x8041] / 255.0f,
				 (float)REGS[0x8040] / 255.0f,
				 (float)REGS[0x8043] / 255.0f);
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (cfg.ListSorting) {
		qsort((void*)objList, objCount, sizeof(OBJECT), comp_func);
//		qsort((void*)objList,objCount,sizeof(OBJECT),listtypesort_func);
//		qsort((void*)objList,objCount,sizeof(OBJECT),transsort_func);
//		qsort((void*)objList,objCount,sizeof(OBJECT),punchsort_func);
	}

	glPushMatrix();

	switch (target) {
	case RENDER_TARGET_FRAMEBUFFER:
		glViewport(0, 0, 640, 480);
		glTranslatef(0.0f, 0.0f, -(float)RENDER_VIEW_DISTANCE);
		break;
	case RENDER_TARGET_TEXTURE:
	{
		glViewport(0, 0, 640, (REGS16(0x806E) - REGS16(0x806c) + 1));
		glScalef(1.0f, -1.0f, 1.0f);
		glTranslatef(0.0f, -480.0f, -(float)RENDER_VIEW_DISTANCE);
		break;
	}
	}

//	DEMUL_printf(">render scene (objCount=%d)\n",objCount);

	for (i = 0; i < objCount; i++) {
		OBJECT *obj = &objList[i];
		vCount = obj->vCount;

		if (vCount) {
			polygon = obj->polygon;

//			DEMUL_printf(">object %3d, dist = %6f, listtype = %d, pcw = %08X, isp = %08X, tsp = %08X, tcw = %08X\n",i,obj->midZ, obj->polygon.pcw.listtype, obj->polygon.pcw.all, obj->polygon.isp.all, obj->polygon.tsp.all, obj->polygon.tcw.all);

			if (polygon.tsp.usealpha && cfg.AlphaZWriteDisable)
				polygon.isp.zwritedis = 1;

			SetupShadeMode();
			SetupCullMode();
			SetupZBuffer();
			if (cfg.Wireframe) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			} else {
				SetupAlphaTest();
				SetupAlphaBlend();
				SetupTexture();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			glDrawElements(GL_TRIANGLES, vCount, GL_UNSIGNED_INT, &pIndex[obj->vIndex]);
		}
	}
	glPopMatrix();

//	ProfileFinish(0);

	IRQ(0x0002);

	switch (target) {
	case RENDER_TARGET_FRAMEBUFFER:
		SwapBuffers(hdc);
		break;
	case RENDER_TARGET_TEXTURE:
	{
		u32 tw = REGS32(0x804C) << 2;
		u32 th = 512;
		glReadBuffer(GL_BACK);
		glBindTexture(GL_TEXTURE_2D, backbufname);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, tw, th, 0);
		isFramebufferRendered = 1;
		TextureAddress = REGS32(0x8060) & 0x007FFFFF;
		break;
	}
	}

//	DEMUL_printf(">render end\n");
//	DEMUL_printf(">frame summary (maxz=%f,minz=%f)\n",maxz,minz);

	maxz = -10000000.0f;
	minz = 10000000.0f;
	objCount = 0;
	vIndex = 0;
	vCount = 0;
	vVertex = 0;
}
