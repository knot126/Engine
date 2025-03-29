#pragma once

#include "util/maths.h"
#include "util/window.h"
#include "glad/egl.h"
#ifndef GLAD_GLES2_IMPLEMENTATION
	#include "glad/gles2.h"
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef uint32_t ZnContextFlags;

enum {
	ZN_CONTEXT_FLAG_EXTERNAL_DISPLAY = (1 << 0),
};

typedef enum ZnFormat {
	ZN_FORMAT_RGB = GL_RGB,
	ZN_FORMAT_RGBA = GL_RGBA,
} ZnFormat;

typedef enum ZnTextureFlags {
	ZN_PIXEL_ART = (1 << 0),
	ZN_TEXTURE_REPEAT = (1 << 1),
} ZnTextureFlags;

typedef struct {
	float x, y, z;
	float u, v;
	uint8_t r, g, b, a;
} ZnVertex;

typedef struct ZnOpenGLProgram ZnOpenGLProgram;

typedef struct {
	// Multi-frame state
	Display *display;
	EGLDisplay egl_display;
	EGLConfig egl_config;
	EGLSurface egl_surface;
	EGLContext egl_context;
	ZnContextFlags flags;
	DgVec2I size;
	
	DgColour background;
	struct ZnOpenGLProgram *program;
	
	// Textures
	DgTable textures;
	GLint default_texture_id;
	
	// Single frame state
	DgMemoryStream *buffer;
} ZnContext;

DgError ZnContextCreate(ZnContext * const context, DgVec2I size);
DgError ZnContextCreateFromNativeHandles(ZnContext * const this, void *display, void *window);
DgError ZnContextCreateFromWindow(ZnContext * const this, DgWindow *window);
void ZnContextDestroy(ZnContext * const context);

uint64_t ZnUploadTexture(ZnContext *this, DgTexture *texture, ZnTextureFlags flags);

DgError ZnDrawBegin(ZnContext * const this);
DgError ZnDrawEnd(ZnContext * const this);
DgError ZnGetFrameData(ZnContext * const this, size_t size, void *data, bool alpha);

DgError ZnDrawVerts(ZnContext * const this, size_t count, ZnVertex *verticies, uint64_t texture);
DgError ZnDrawPlainVerts(ZnContext * const this, size_t count, ZnVertex *verticies);
DgError ZnDrawQuad(ZnContext * const this, DgVec2 top, DgVec2 bottom, uint64_t texture);
DgError ZnDrawRect(ZnContext * const this, DgVec2 pos, DgVec2 size, uint64_t texture);
