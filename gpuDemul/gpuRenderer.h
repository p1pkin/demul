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

#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <windows.h>
#include <gl\gl.h>
#include <gl\glext.h>
#include "types.h"


#define RENDER_TARGET_FRAMEBUFFER   1
#define RENDER_TARGET_TEXTURE       2
#define RENDER_VIEW_DISTANCE        1

u32 width;
u32 height;

int     CreateGpuWindow();
int     InitialOpenGL();
void    DeleteOpenGL();
void    flushPrim();
void    glInternalFlush();
void    setDisplayMode();
GLenum  getSnapshot(u8 *buf);
GLenum  getTexture(u8 *buf);
void    glRenderFramebuffer(u32 backbuf, u32 pixelformat);

#endif
