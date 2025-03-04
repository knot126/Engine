#include "util/melon.h"
#define GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#undef GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#define GLAD_GLES2_IMPLEMENTATION
#include "glad/gles2.h"
#include "rendroar.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "ro_program.h"

const char *gRoDefaultShader = "varying vec2 fTextureCoords;\nvarying vec4 fColour;\n\n#ifdef VERTEX\nattribute vec3 inPosition;\nattribute vec2 inTextureCoords;\nattribute vec4 inColour;\n\nvoid main() {\n\tgl_Position = vec4(inPosition, 1.0);\n\tfTextureCoords = inTextureCoords;\n\tfColour = inColour;\n}\n#endif\n\n#ifdef FRAGMENT\nuniform sampler2D gTexture;\n\nvoid main() {\n\tgl_FragColor = fColour * texture2D(gTexture, fTextureCoords);\n}\n#endif\n";

static DgError RoContextCreate_InitGL(RoContext * const this, DgVec2I size, void *ndisplay, void *window) {
	/**
	 * Initialise OpenGL for the new context
	 * 
	 * @warning You can only have one context at a time atm.
	 * 
	 * @param this context
	 * @param size Size of the buffer to create (if window == NULL)
	 * @param window Native window handle (or NULL if pbuffer)
	 * @return error while initialising context
	 */
	
	this->size = size;
	
	if (glGetString) {
		DgLog(DG_LOG_WARNING, "OpenGL %s is already loaded... somehow.", glGetString(GL_VERSION));
		return DG_ERROR_SUCCESS;
	}
	
	// Get the default X display
	Display *display = ndisplay ? (Display *) ndisplay : XOpenDisplay(NULL);
	
	if (!display) {
		DgLog(DG_LOG_ERROR, "Failed to open X display.");
		return DG_ERROR_FAILED;
	}
	
	if (ndisplay) {
		this->flags |= RO_CONTEXT_FLAG_EXTERNAL_DISPLAY;
	}
	
	this->display = display;
	
	// Note that we don't need to create any window.
	
	// Load EGL wihtout display so we can get eglGetDisplay
	int egl_version = gladLoaderLoadEGL(NULL);
	
	if (!egl_version) {
		DgLog(DG_LOG_ERROR, "Initial loading of EGL failed.");
		return DG_ERROR_FAILED;
	}
	
	// Get the EGL abstract display
	EGLDisplay egl_display = eglGetDisplay((EGLNativeDisplayType) display);
	
	if (egl_display == EGL_NO_DISPLAY) {
		DgLog(DG_LOG_ERROR, "Failed to get EGL display for the native display.");
		return DG_ERROR_FAILED;
	}
	
	this->egl_display = egl_display;
	
	// Initialise EGL for this display connection
	if (!eglInitialize(egl_display, NULL, NULL)) {
		DgLog(DG_LOG_ERROR, "Initialising EGL failed.");
		return DG_ERROR_FAILED;
	}
	
	// Now we can load EGL with the display set.
	egl_version = gladLoaderLoadEGL(egl_display);
	
	if (!egl_version) {
		DgLog(DG_LOG_ERROR, "Failed to load EGL for the native display.");
		return DG_ERROR_FAILED;
	}
	
	// Log version
	const int egl_maj = GLAD_VERSION_MAJOR(egl_version);
	const int egl_min = GLAD_VERSION_MINOR(egl_version);
	
	DgLog(DG_LOG_INFO, "Using EGL %d.%d", egl_maj, egl_min);
	
	// Choose display configuration
	EGLConfig egl_config;
	EGLint egl_config_count;
	EGLint egl_config_attr[] = {
		EGL_BUFFER_SIZE, 16,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE,
	};
	
	if (!eglChooseConfig(egl_display, egl_config_attr, &egl_config, 1, &egl_config_count)) {
		DgLog(DG_LOG_ERROR, "Could not choose any EGL config.");
		return DG_ERROR_FAILED;
	}
	
	if (egl_config_count != 1) {
		DgLog(DG_LOG_ERROR, "More than one EGL config.");
		return DG_ERROR_FAILED;
	}
	
	this->egl_config = egl_config;
	
	// Create either a window or offscreen EGL surface
	EGLSurface egl_surface;
	
	if (!window) {
		EGLint egl_surface_attr[] = {
			EGL_WIDTH, size.x,
			EGL_HEIGHT, size.y,
			EGL_NONE,
		};
		
		egl_surface = eglCreatePbufferSurface(egl_display, egl_config, egl_surface_attr);
	}
	else {
		egl_surface = eglCreateWindowSurface(egl_display, egl_config, (EGLNativeWindowType) window, NULL);
	}
	
	if (egl_surface == EGL_NO_SURFACE) {
		DgLog(DG_LOG_ERROR, "Could not create EGL surface: status code <0x%x>", eglGetError());
		return DG_ERROR_FAILED;
	}
	
	this->egl_surface = egl_surface;
	
	// Create EGL context and make it current
	EGLint egl_context_attr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	
	EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attr);
	
	if (egl_context == EGL_NO_CONTEXT) {
		DgLog(DG_LOG_ERROR, "Could not create EGL context: status code %d", eglGetError());
		return DG_ERROR_FAILED;
	}
	
	this->egl_context = egl_context;
	
	if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
		DgLog(DG_LOG_WARNING, "A current EGL context is already set.");
	}
	
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
	
	EGLint egl_error = eglGetError();
	
	if (egl_error != EGL_SUCCESS) {
		DgLog(DG_LOG_ERROR, "EGL error 0x%x while setting context as current for first time", egl_error);
	}
	
	// Finally load OpenGL ES 2.0
	int gl_version = gladLoaderLoadGLES2();
	
	if (!gl_version) {
		return DG_ERROR_FAILED;
	}
	
	DgLog(DG_LOG_INFO, "Using OpenGL ES %d.%d", GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));
	DgLog(DG_LOG_INFO, "Graphics hardware: %s %s", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	
	return DG_ERROR_SUCCESS;
}

static GLint RoUploadTextureInternal(RoFormat format, size_t width, size_t height, const void *pixels, RoTextureFlags flags);
static GLuint RoLookupTextureId(RoContext *this, const char *name);

static DgError RoUploadDefaultTexture(RoContext * const this) {
	char data[3] = {255, 255, 255};
	
	this->default_texture_id = RoUploadTextureInternal(RO_FORMAT_RGB, 1, 1, &data, 0);
	
	if (this->default_texture_id < 0) {
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}

static DgError RoContextCreate_Main(RoContext * const this, DgVec2I size, void *display, void *window) {
	/**
	 * Create a new context
	 * 
	 * @param this Context object
	 * @param size Size of the image
	 * @param window Native on-screen window object or NULL
	 * @return Error code while initialising
	 */
	
	DgError status = RoContextCreate_InitGL(this, size, display, window);
	
	if (status) {
		DgRaise("GLInitError", "Failed to initialise OpenGL");
		return status;
	}
	
	status = RoUploadDefaultTexture(this);
	
	if (status) {
		DgRaise("TextureUploadError", "Could not upload default texture to gpu");
		return status;
	}
	
	this->program = DgMemoryAllocate(sizeof this->program);
	
	if (!this->program) {
		return DG_ERROR_ALLOCATION_FAILED;
	}
	
	status = RoOpenGLProgramInit(this->program, gRoDefaultShader);
	
	if (status) {
		DgLog(DG_LOG_ERROR, "Failed to initialise program, status <0x%x>.", status);
		DgRaise("GLProgramError", "Failed to load main OpenGL program");
		return status;
	}
	
	status = RoOpenGLProgramSetGlobalInt(this->program, "gTexture", 0);
	
	if (status) {
		DgRaise("GLProgramError", "Failed to set gTexture");
		return status;
	}
	
	this->buffer = DgMemoryStreamCreate();
	
	if (!this->buffer) {
		return DG_ERROR_FAILED;
	}
	
	this->background = (DgColour) {0.5, 0.5, 0.5, 1.0};
	
	status = DgTableInit(&this->textures);
	
	if (status) {
		return DG_ERROR_FAILED;
	}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	return DG_ERROR_SUCCESS;
}

DgError RoContextCreate(RoContext * const this, DgVec2I size) {
	return RoContextCreate_Main(this, size, NULL, NULL);
}

DgError RoContextCreateFromNativeHandles(RoContext * const this, void *display, void *window) {
	return RoContextCreate_Main(this, (DgVec2I) {0, 0}, display, window);
}

DgError RoContextCreateFromWindow(RoContext * const this, DgWindow *window) {
	return RoContextCreate_Main(this, (DgVec2I) {0, 0}, DgWindowGetNativeDisplayHandleForEGL(window), DgWindowGetNativeWindowHandleForEGL(window));
}

static GLint RoUploadTextureInternal(RoFormat format, size_t width, size_t height, const void *pixels, RoTextureFlags flags) {
	GLuint id;
	
	// Set unpack alignment to 1 if RGB or 4 if RGBA
	glPixelStorei(GL_UNPACK_ALIGNMENT, (format == GL_RGBA) ? 4 : 1);
	
	// Generate texture name and bind texture
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	
	// Push texture data
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	
	// Use bilinear interpolation for normal textures, nearest for pixel art
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & RO_PIXEL_ART) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & RO_PIXEL_ART) ? GL_NEAREST : GL_LINEAR);
	
	// Set clamp to edge by default or repeat texture if requested
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (flags & RO_TEXTURE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (flags & RO_TEXTURE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	
	GLenum gl_error = glGetError();
	
	if (gl_error != GL_NO_ERROR) {
		return -1;
	}
	
	return id;
}

DgError RoUploadTextureRaw(RoContext * const this, const char *name, RoFormat format, size_t width, size_t height, const void *pixels, RoTextureFlags flags) {
	/**
	 * Upload a texture to the gpu
	 */
	
	GLint id = RoUploadTextureInternal(format, width, height, pixels, flags);
	
	// Store in map
	DgValue key = DgMakeString(name);
	DgValue val = DgMakeInt32(id);
	
	DgTablePut(&this->textures, &key, &val);
	
	return DG_ERROR_SUCCESS;
}

DgError RoUploadTexture(RoContext *this, const char *name, DgTexture *texture, RoTextureFlags flags) {
	/**
	 * Upload a copy of the loaded texture to the gpu
	 */
	
	RoUploadTextureRaw(this, name, (texture->format == DG_TEXTURE_RGB) ? RO_FORMAT_RGB : RO_FORMAT_RGBA, texture->width, texture->height, texture->pixels, flags);
}

void RoContextDestroy(RoContext * const this) {
	/**
	 * Destroy the context
	 */
	
	// Delete default texture
	glDeleteTextures(1, &this->default_texture_id);
	
	// Delete program
	RoOpenGLProgramFree(this->program);
	DgMemoryFree(this->program);
	
	// Unload GLES
	gladLoaderUnloadGLES2();
	
	// Terminate EGL
	eglDestroyContext(this->egl_display, this->egl_context);
	eglDestroySurface(this->egl_display, this->egl_surface);
	eglTerminate(this->egl_display);
	
	gladLoaderUnloadEGL();
	
	// Destory builtin display
	if (!(this->flags & RO_CONTEXT_FLAG_EXTERNAL_DISPLAY)) {
		DgLog(DG_LOG_INFO, "Rendroar is destroying X display...");
		XCloseDisplay(this->display);
	}
}

static DgError RoSetTextureAsCurrentFromID(GLuint id) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);
	return DG_ERROR_SUCCESS;
}

static GLuint RoLookupTextureId(RoContext *this, const char *name) {
	DgValue key = DgMakeStaticString(name);
	DgValue *val = DgTableAt(&this->textures, &key);
	
	return val ? val->data.asInt32 : 0;
}

static void RoContextMakeCurrent(RoContext *this) {
	eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context);
	
	EGLint egl_error;
	
	if ((egl_error = eglGetError()) != EGL_SUCCESS) {
		DgLog(DG_LOG_ERROR, "EGL error 0x%x while setting context as current", egl_error);
	}
}

static GLint RoUseVertexAttrib(GLint program, const char *name, GLint size, GLenum type, GLboolean normalise, GLsizei stride, const void *pointer) {
	GLint location = glGetAttribLocation(program, name);
	
	if (location >= 0) {
		glEnableVertexAttribArray(location);
		glVertexAttribPointer(
			location,
			size,
			type,
			normalise,
			stride,
			pointer
		);
	}
	else {
		DgLog(DG_LOG_WARNING, "attrib %s not found", name);
	}
	
	return location;
}

DgError RoDrawBegin(RoContext * const this) {
	/**
	 * Start the drawing process
	 * 
	 * @note Only really does some basic housekeeping
	 */
	
	DgMemoryStreamRewind(this->buffer);
	
	return DG_ERROR_SUCCESS;
}

enum {
	RO_CMD_STOP,
	RO_CMD_DRAW_TRIS,
	RO_CMD_SET_TEXTURE,
	RO_CMD_CLEAR_TEXTURE,
};

DgError RoDrawVerts(RoContext * const this, size_t count, RoVertex *verticies, const char *texture) {
	/**
	 * Draw textured verticies to the screen
	 * 
	 * @param this Context
	 * @param count Number of verticies
	 * @param verticies Vertex data
	 * @param texture Name of texture to use
	 * @return Error while drawing verticies
	 */
	
	if (texture) {
		DgMemoryStreamWriteUInt32(this->buffer, RO_CMD_SET_TEXTURE);
		DgMemoryStreamWriteInt32(this->buffer, RoLookupTextureId(this, texture));
	}
	else {
		DgMemoryStreamWriteUInt32(this->buffer, RO_CMD_CLEAR_TEXTURE);
	}
	
	DgMemoryStreamWriteUInt32(this->buffer, RO_CMD_DRAW_TRIS);
	DgMemoryStreamWriteUInt32(this->buffer, count);
	DgMemoryStreamWrite(this->buffer, sizeof *verticies * count, verticies);
	
	if (DgMemoryStreamError(this->buffer) != DG_MEMORY_STREAM_OKAY) {
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}

DgError RoDrawPlainVerts(RoContext * const this, size_t count, RoVertex *verticies) {
	return RoDrawVerts(this, count, verticies, NULL);
}

DgError RoDrawQuad(RoContext * const this, DgVec2 top, DgVec2 bottom, const char *texture) {
	RoVertex verts[] = {
		(RoVertex) {bottom.x, top.y,    1.0, 1.0, 1.0, 255, 255, 255, 255},
		(RoVertex) {bottom.x, bottom.y, 1.0, 1.0, 0.0, 255, 255, 255, 255},
		(RoVertex) {top.x,    top.y,    1.0, 0.0, 1.0, 255, 255, 255, 255},
		(RoVertex) {top.x,    bottom.y, 1.0, 0.0, 0.0, 255, 255, 255, 255},
		(RoVertex) {bottom.x, bottom.y, 1.0, 1.0, 0.0, 255, 255, 255, 255},
		(RoVertex) {top.x,    top.y,    1.0, 0.0, 1.0, 255, 255, 255, 255},
	};
	
	return RoDrawVerts(this, 6, verts, texture);
}

DgError RoDrawEnd(RoContext * const this) {
	/**
	 * Finish the drawing process and swap front and back buffers
	 */
	
	GLenum gl_error = glGetError();
	
	if (gl_error != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Not drawing due to previous unhandled OpenGL error: <0x%x>", gl_error);
		return DG_ERROR_FAILED;
	}
	
	// finish off buffer
	DgMemoryStreamWriteUInt32(this->buffer, RO_CMD_STOP);
	DgMemoryStreamRewind(this->buffer);
	
	RoContextMakeCurrent(this);
	
	// Update the viewport
	EGLint width, height;
	eglQuerySurface(this->egl_display, this->egl_surface, EGL_WIDTH, &width);
	eglQuerySurface(this->egl_display, this->egl_surface, EGL_HEIGHT, &height);
	
	glViewport(0, 0, width, height);
	
	// Clear the screen
	glClearColor(this->background.r, this->background.g, this->background.b, this->background.a);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Set the active program
	GLuint program = this->program->program;
	glUseProgram(program);
	
	bool drawing = true;
	
	while (drawing) {
		uint32_t cmd = DgMemoryStreamReadUInt32(this->buffer);
		
		switch (cmd) {
			case RO_CMD_STOP: {
				DgLog(DG_LOG_VERBOSE, "Drawing.Stop");
				drawing = false;
				break;
			}
			
			case RO_CMD_SET_TEXTURE: {
				GLint id = DgMemoryStreamReadInt32(this->buffer);
				DgLog(DG_LOG_VERBOSE, "Drawing.SetTexture %d", id);
				RoSetTextureAsCurrentFromID(id);
				break;
			}
			
			case RO_CMD_CLEAR_TEXTURE: {
				DgLog(DG_LOG_VERBOSE, "Drawing.ClearTexture");
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->default_texture_id);
				break;
			}
			
			case RO_CMD_DRAW_TRIS: {
// 				glValidateProgram(program);
// 				GLint status;
// 				glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
// 				
// 				if (status != GL_TRUE) {
// 					DgLog(DG_LOG_WARNING, "Program validation failed!");
// 					char log[2048];
// 					glGetProgramInfoLog(program, sizeof(log), NULL, log);
// 					DgLog(DG_LOG_WARNING, "%s", log);
// 				}
				
				size_t vertex_count = DgMemoryStreamReadUInt32(this->buffer);
				RoVertex *data = DgMemoryStreamGetHeadPointer(this->buffer);
				DgMemoryStreamSetpos(this->buffer, DG_MEMORY_STREAM_CUR, sizeof(RoVertex) * vertex_count);
				
				DgLog(DG_LOG_VERBOSE, "Drawing.DrawTris %zu <@ 0x%llx>", vertex_count, data);
				
				for (size_t i = 0; i < vertex_count; i++) {
					DgLog(DG_LOG_VERBOSE, "%f %f %f   %f %f   %d %d %d %d", data[i].x, data[i].y, data[i].z, data[i].u, data[i].v, data[i].r, data[i].g, data[i].b, data[i].a);
				}
				
				GLint inPosition = RoUseVertexAttrib(
					program,
					"inPosition",
					3,
					GL_FLOAT,
					GL_FALSE,
					sizeof(RoVertex),
					&data[0].x
				);
				
				GLint inTextureCoords = RoUseVertexAttrib(
					program,
					"inTextureCoords",
					2,
					GL_FLOAT,
					GL_FALSE,
					sizeof(RoVertex),
					&data[0].u
				);
				
				GLint inColour = RoUseVertexAttrib(
					program,
					"inColour",
					4,
					GL_UNSIGNED_BYTE,
					GL_TRUE,
					sizeof(RoVertex),
					&data[0].r
				);
				
				// Draw the arrays
				glDrawArrays(GL_TRIANGLES, 0, vertex_count);
				
				// Undo setup
				if (inPosition >= 0) glDisableVertexAttribArray(inPosition);
				if (inTextureCoords >= 0) glDisableVertexAttribArray(inTextureCoords);
				if (inColour >= 0) glDisableVertexAttribArray(inColour);
				
				gl_error = glGetError();
				
				if (gl_error != GL_NO_ERROR) {
					DgLog(DG_LOG_ERROR, "Did not draw sucessfully: <0x%x>", gl_error);
					return DG_ERROR_FAILED;
				}
				
				break;
			}
			
			default: {
				DgLog(DG_LOG_ERROR, "Invalid draw buffer command");
				return DG_ERROR_FAILED;
				break;
			}
		}
	}
	
	// Swap buffers
	eglSwapBuffers(this->egl_display, this->egl_surface);
	
	EGLint egl_error = eglGetError();
	
	if (egl_error != EGL_SUCCESS) {
		DgLog(DG_LOG_ERROR, "EGL error 0x%x while swapping buffers", egl_error);
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}

DgError RoGetFrameData(RoContext * const this, size_t size, void *data, bool alpha) {
	/**
	 * Get the data for the front frame
	 * 
	 * @warning OpenGL ES does not support having alpha be false
	 * 
	 * @warning Due to how OpenGL handles textures the result will be flipped
	 * upside down.
	 * 
	 * @param this Context
	 * @param size Max size of the buffer
	 * @param data Pointer to the buffer to write into
	 * @param alpha If alpha should be included
	 */
	
	size_t req_size = (alpha ? 4 : 3) * this->size.x * this->size.y;
	
	if (size < req_size || !data) {
		return DG_ERROR_NOT_SAFE;
	}
	
	glReadPixels(0, 0, this->size.x, this->size.y, alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
	
	GLenum error = glGetError();
	
	if (error != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "OpenGL error <0x%x> while getting frame data.", error);
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}
