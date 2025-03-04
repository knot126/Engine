#pragma once

#include "util/maths.h"
#include "util/window.h"
#include "glad/egl.h"
#ifndef GLAD_GLES2_IMPLEMENTATION
	#include "glad/gles2.h"
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef uint32_t RoContextFlags;

enum {
	RO_CONTEXT_FLAG_EXTERNAL_DISPLAY = (1 << 0),
};

typedef enum RoFormat {
	RO_FORMAT_RGB = GL_RGB,
	RO_FORMAT_RGBA = GL_RGBA,
} RoFormat;

typedef enum RoTextureFlags {
	RO_PIXEL_ART = (1 << 0),
	RO_TEXTURE_REPEAT = (1 << 1),
} RoTextureFlags;

typedef struct {
	float x, y, z;
	float u, v;
	uint8_t r, g, b, a;
} RoVertex;

typedef struct RoOpenGLProgram RoOpenGLProgram;

typedef struct {
	// Multi-frame state
	Display *display;
	EGLDisplay egl_display;
	EGLConfig egl_config;
	EGLSurface egl_surface;
	EGLContext egl_context;
	RoContextFlags flags;
	DgVec2I size;
	
	DgColour background;
	struct RoOpenGLProgram *program;
	
	// Textures
	DgTable textures;
	GLint default_texture_id;
	
	// Single frame state
	DgMemoryStream *buffer;
} RoContext;

DgError RoContextCreate(RoContext * const context, DgVec2I size);
DgError RoContextCreateFromNativeHandles(RoContext * const this, void *display, void *window);
DgError RoContextCreateFromWindow(RoContext * const this, DgWindow *window);
void RoContextDestroy(RoContext * const context);

DgError RoUploadTextureRaw(RoContext * const this, const char *name, RoFormat format, size_t width, size_t height, const void *pixels, RoTextureFlags flags);
DgError RoUploadTexture(RoContext *this, const char *name, DgTexture *texture, RoTextureFlags flags);

DgError RoDrawBegin(RoContext * const this);
DgError RoDrawEnd(RoContext * const this);
DgError RoGetFrameData(RoContext * const this, size_t size, void *data, bool alpha);

DgError RoDrawVerts(RoContext * const this, size_t count, RoVertex *verticies, const char *texture);
DgError RoDrawPlainVerts(RoContext * const this, size_t count, RoVertex *verticies);
DgError RoDrawQuad(RoContext * const this, DgVec2 top, DgVec2 bottom, const char *texture);
