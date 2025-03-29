#include "util/melon.h"
#define GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#undef GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#define GLAD_GLES2_IMPLEMENTATION
#include "glad/gles2.h"
#include "zinc.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "shaders.h"

typedef struct ZnOpenGLProgram {
	GLuint vertex, fragment, program;
} ZnOpenGLProgram;

static GLuint ZnOpenGLLoadShaderFromSource(GLenum type, const char *source) {
	/**
	 * Load a shader from source.
	 * 
	 * @param source Source of the shader to compile
	 * @return shader handle on success, 0 on failure
	 */
	
	GLuint shader;
	GLint status;
	
	shader = glCreateShader(type);
	
	if (!shader) {
		return 0;
	}
	
	// We like to #define VERTEX or #define FRAGMENT based on the type
	const char *source_real[] = {
		"precision mediump float;\n",
		(type == GL_VERTEX_SHADER) ? "#define VERTEX\n\n" : "#define FRAGMENT\n\n",
		source,
	};
	
	// DgLog(DG_LOG_VERBOSE, "--------\n%s%s%s--------\n", source_real[0], source_real[1], source_real[2]);
	
	glShaderSource(shader, 3, source_real, NULL);
	
	glCompileShader(shader);
	
	// Get the status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	
	if (!status) {
		GLint error_length = 0;
		
		const char *source_real[] = {
			"precision mediump float;\n",
			(type == GL_VERTEX_SHADER) ? "#define VERTEX\n\n" : "#define FRAGMENT\n\n",
			source,
		};
		
		glShaderSource(shader, 3, source_real, NULL);
		
		glCompileShader(shader);
		
		// Get the status
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		
		if (!status) {
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_length);
			
			if (error_length > 0) {
				char *error = DgMemoryAllocate(error_length);
				
				if (!error) {
					DgLog(DG_LOG_ERROR, "Error while displaying shader compilation error message.");
					goto L_DeepError;
				}
				
				glGetShaderInfoLog(shader, error_length, NULL, error);
				
				DgLog(DG_LOG_ERROR, "Failed to compile shader:\n%s", error);
				
				DgMemoryFree(error);
			}
			else {
				DgLog(DG_LOG_ERROR, "Failed to compile shader but no log output was given.");
			}
		}
		
		L_DeepError:
		glDeleteShader(shader);
		
		return 0;
	}
	
	return shader;
}

static DgError ZnOpenGLProgramInit(ZnOpenGLProgram *this, const char *source) {
	/**
	 * Initialise a new program with it's source code
	 */
	
	GLint status;
	
	this->vertex = ZnOpenGLLoadShaderFromSource(GL_VERTEX_SHADER, source);
	this->fragment = ZnOpenGLLoadShaderFromSource(GL_FRAGMENT_SHADER, source);
	
	this->program = glCreateProgram();
	
	if (!this->program) {
		return DG_ERROR_FAILED;
	}
	
	glAttachShader(this->program, this->vertex);
	glAttachShader(this->program, this->fragment);
	
	glLinkProgram(this->program);
	
	// Check status
	glGetProgramiv(this->program, GL_LINK_STATUS, &status);
	
	if (!status) {
		GLint error_length = 0;
		
		glGetProgramiv(this->program, GL_INFO_LOG_LENGTH, &error_length);
		
		if (error_length > 0) {
			char *error = DgMemoryAllocate(error_length);
			
			if (!error) {
				DgLog(DG_LOG_ERROR, "Error while displaying program link error message.");
				goto L_DeepError;
			}
			
			glGetProgramInfoLog(this->program, error_length, NULL, error);
			
			DgLog(DG_LOG_ERROR, "Failed to link program:\n%s", error);
			
			DgMemoryFree(error);
		}
		else {
			DgLog(DG_LOG_ERROR, "Failed to link program but no log output was given.");
		}
		
		L_DeepError:
		glDeleteProgram(this->program);
		
		return 0;
	}
	
	return DG_ERROR_SUCCESS;
}

static void ZnOpenGLProgramFree(ZnOpenGLProgram *this) {
	glDeleteProgram(this->program);
	glDeleteShader(this->vertex);
	glDeleteShader(this->fragment);
}

static bool ZnOpenGLProgramSetGlobalInt(ZnOpenGLProgram *this, const char *name, GLint val) {
	// Clear error
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Not setting %s because of previous unhandled OpenGL error: <0x%x>", name, err);
		return false;
	}
	
	// Get uniform location
	GLint location = glGetUniformLocation(this->program, name);
	
	err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Failed to get uniform location for %s: <0x%x>", name, err);
		return false;
	}
	
	// Set it
	glUseProgram(this->program);
	glUniform1i(location, val);
	
	err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Failed to set global integer %s to %d: <0x%x>", name, val, err);
		return false;
	}
	
	return true;
}

static bool ZnOpenGLProgramSetGlobalVec2f(ZnOpenGLProgram *this, const char *name, DgVec2 val) {
	// Clear error
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Not setting %s because of previous unhandled OpenGL error: <0x%x>", name, err);
		return false;
	}
	
	// Get uniform location
	GLint location = glGetUniformLocation(this->program, name);
	
	err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Failed to get uniform location for %s: <0x%x>", name, err);
		return false;
	}
	
	// Set it
	glUseProgram(this->program);
	glUniform2f(location, val.x, val.y);
	
	err = glGetError();
	if (err != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Failed to set global integer %s to %d: <0x%x>", name, val, err);
		return false;
	}
	
	return true;
}

static DgError ZnContextCreate_InitGL(ZnContext * const this, DgVec2I size, void *ndisplay, void *window) {
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
		this->flags |= ZN_CONTEXT_FLAG_EXTERNAL_DISPLAY;
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

static GLint ZnCreateOpenGLTexture(ZnFormat format, size_t width, size_t height, const void *pixels, ZnTextureFlags flags);

static DgError ZnUploadDefaultTexture(ZnContext * const this) {
	char data[3] = {255, 255, 255};
	
	this->default_texture_id = ZnCreateOpenGLTexture(ZN_FORMAT_RGB, 1, 1, &data, 0);
	
	if (this->default_texture_id < 0) {
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}

static DgError ZnContextCreate_Main(ZnContext * const this, DgVec2I size, void *display, void *window) {
	/**
	 * Create a new context
	 * 
	 * @param this Context object
	 * @param size Size of the image
	 * @param window Native on-screen window object or NULL
	 * @return Error code while initialising
	 */
	
	DgError status = ZnContextCreate_InitGL(this, size, display, window);
	
	if (status) {
		DgRaise("GLInitError", "Failed to initialise OpenGL");
		return status;
	}
	
	status = ZnUploadDefaultTexture(this);
	
	if (status) {
		DgRaise("TextureUploadError", "Could not upload default texture to gpu");
		return status;
	}
	
	this->program = DgMemoryAllocate(sizeof this->program);
	
	if (!this->program) {
		return DG_ERROR_ALLOCATION_FAILED;
	}
	
	status = ZnOpenGLProgramInit(this->program, shader_basicTexturedColoured2D_glsl);
	
	if (status) {
		DgLog(DG_LOG_ERROR, "Failed to initialise program, status <0x%x>.", status);
		DgRaise("GLProgramError", "Failed to load main OpenGL program");
		return status;
	}
	
	if (!ZnOpenGLProgramSetGlobalInt(this->program, "gTexture", 0)) {
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

DgError ZnContextCreate(ZnContext * const this, DgVec2I size) {
	return ZnContextCreate_Main(this, size, NULL, NULL);
}

DgError ZnContextCreateFromNativeHandles(ZnContext * const this, void *display, void *window) {
	return ZnContextCreate_Main(this, (DgVec2I) {0, 0}, display, window);
}

DgError ZnContextCreateFromWindow(ZnContext * const this, DgWindow *window) {
	return ZnContextCreate_Main(this, (DgVec2I) {0, 0}, DgWindowGetNativeDisplayHandleForEGL(window), DgWindowGetNativeWindowHandleForEGL(window));
}

static GLint ZnCreateOpenGLTexture(ZnFormat format, size_t width, size_t height, const void *pixels, ZnTextureFlags flags) {
	GLuint id;
	
	// Set unpack alignment to 1 if RGB or 4 if RGBA
	glPixelStorei(GL_UNPACK_ALIGNMENT, (format == GL_RGBA) ? 4 : 1);
	
	// Generate texture name and bind texture
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	
	// Push texture data
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	
	// Use bilinear interpolation for normal textures, nearest for pixel art
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & ZN_PIXEL_ART) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & ZN_PIXEL_ART) ? GL_NEAREST : GL_LINEAR);
	
	// Set clamp to edge by default or repeat texture if requested
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (flags & ZN_TEXTURE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (flags & ZN_TEXTURE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	
	GLenum gl_error = glGetError();
	
	if (gl_error != GL_NO_ERROR) {
		return -1;
	}
	
	return id;
}

static uint64_t ZnUploadTextureRaw(ZnContext * const this, ZnFormat format, size_t width, size_t height, const void *pixels, ZnTextureFlags flags) {
	/**
	 * Upload a texture to the gpu
	 */
	
	return ZnCreateOpenGLTexture(format, width, height, pixels, flags) + 1;
}

uint64_t ZnUploadTexture(ZnContext *this, DgTexture *texture, ZnTextureFlags flags) {
	/**
	 * Upload a copy of the loaded texture to the gpu
	 */
	
	return ZnUploadTextureRaw(this, (texture->format == DG_TEXTURE_RGB) ? ZN_FORMAT_RGB : ZN_FORMAT_RGBA, texture->width, texture->height, texture->pixels, flags);
}

void ZnContextDestroy(ZnContext * const this) {
	/**
	 * Destroy the context
	 */
	
	// Delete default texture
	glDeleteTextures(1, &this->default_texture_id);
	
	// Delete program
	ZnOpenGLProgramFree(this->program);
	DgMemoryFree(this->program);
	
	// Unload GLES
	gladLoaderUnloadGLES2();
	
	// Terminate EGL
	eglDestroyContext(this->egl_display, this->egl_context);
	eglDestroySurface(this->egl_display, this->egl_surface);
	eglTerminate(this->egl_display);
	
	gladLoaderUnloadEGL();
	
	// Destory builtin display
	if (!(this->flags & ZN_CONTEXT_FLAG_EXTERNAL_DISPLAY)) {
		DgLog(DG_LOG_INFO, "Rendroar is destroying X display...");
		XCloseDisplay(this->display);
	}
}

static DgError ZnSetTextureAsCurrentFromID(GLuint id) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);
	return DG_ERROR_SUCCESS;
}

static void ZnContextMakeCurrent(ZnContext *this) {
	eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context);
	
	EGLint egl_error;
	
	if ((egl_error = eglGetError()) != EGL_SUCCESS) {
		DgLog(DG_LOG_ERROR, "EGL error 0x%x while setting context as current", egl_error);
	}
}

static GLint ZnUseVertexAttrib(GLint program, const char *name, GLint size, GLenum type, GLboolean normalise, GLsizei stride, const void *pointer) {
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

DgError ZnDrawBegin(ZnContext * const this) {
	/**
	 * Start the drawing process
	 * 
	 * @note Only really does some basic housekeeping
	 */
	
	DgMemoryStreamRewind(this->buffer);
	
	return DG_ERROR_SUCCESS;
}

enum {
	ZN_CMD_STOP,
	ZN_CMD_DRAW_TRIS,
	ZN_CMD_SET_TEXTURE,
	ZN_CMD_CLEAR_TEXTURE,
};

DgError ZnDrawVerts(ZnContext * const this, size_t count, ZnVertex *verticies, uint64_t texture) {
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
		DgMemoryStreamWriteUInt32(this->buffer, ZN_CMD_SET_TEXTURE);
		DgMemoryStreamWriteInt32(this->buffer, texture - 1);
	}
	else {
		DgMemoryStreamWriteUInt32(this->buffer, ZN_CMD_CLEAR_TEXTURE);
	}
	
	DgMemoryStreamWriteUInt32(this->buffer, ZN_CMD_DRAW_TRIS);
	DgMemoryStreamWriteUInt32(this->buffer, count);
	DgMemoryStreamWrite(this->buffer, sizeof *verticies * count, verticies);
	
	if (DgMemoryStreamError(this->buffer) != DG_MEMORY_STREAM_OKAY) {
		return DG_ERROR_FAILED;
	}
	
	return DG_ERROR_SUCCESS;
}

DgError ZnDrawPlainVerts(ZnContext * const this, size_t count, ZnVertex *verticies) {
	return ZnDrawVerts(this, count, verticies, 0);
}

DgError ZnDrawQuad(ZnContext * const this, DgVec2 top, DgVec2 bottom, uint64_t texture) {
	ZnVertex verts[] = {
		(ZnVertex) {bottom.x, top.y,    1.0, 1.0, 1.0, 255, 255, 255, 255},
		(ZnVertex) {bottom.x, bottom.y, 1.0, 1.0, 0.0, 255, 255, 255, 255},
		(ZnVertex) {top.x,    top.y,    1.0, 0.0, 1.0, 255, 255, 255, 255},
		(ZnVertex) {top.x,    bottom.y, 1.0, 0.0, 0.0, 255, 255, 255, 255},
		(ZnVertex) {bottom.x, bottom.y, 1.0, 1.0, 0.0, 255, 255, 255, 255},
		(ZnVertex) {top.x,    top.y,    1.0, 0.0, 1.0, 255, 255, 255, 255},
	};
	
	return ZnDrawVerts(this, 6, verts, texture);
}

DgError ZnDrawRect(ZnContext * const this, DgVec2 pos, DgVec2 size, uint64_t texture) {
	return ZnDrawQuad(this, (DgVec2){pos.x - (0.5f * size.x), pos.y - (0.5f * size.y)}, (DgVec2){pos.x + (0.5f * size.x), pos.y + (0.5f * size.y)}, texture);
}

DgError ZnDrawEnd(ZnContext * const this) {
	/**
	 * Finish the drawing process and swap front and back buffers
	 */
	
	GLenum gl_error = glGetError();
	
	if (gl_error != GL_NO_ERROR) {
		DgLog(DG_LOG_ERROR, "Not drawing due to previous unhandled OpenGL error: <0x%x>", gl_error);
		return DG_ERROR_FAILED;
	}
	
	// finish off buffer
	DgMemoryStreamWriteUInt32(this->buffer, ZN_CMD_STOP);
	DgMemoryStreamRewind(this->buffer);
	
	ZnContextMakeCurrent(this);
	
	// Update the viewport
	EGLint width, height;
	eglQuerySurface(this->egl_display, this->egl_surface, EGL_WIDTH, &width);
	eglQuerySurface(this->egl_display, this->egl_surface, EGL_HEIGHT, &height);
	
	glViewport(0, 0, width, height);
	
	// Update screen size in shader
	ZnOpenGLProgramSetGlobalVec2f(this->program, "gScreenSize", (DgVec2){width, height});
	
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
			case ZN_CMD_STOP: {
				// DgLog(DG_LOG_VERBOSE, "Drawing.Stop");
				drawing = false;
				break;
			}
			
			case ZN_CMD_SET_TEXTURE: {
				GLint id = DgMemoryStreamReadInt32(this->buffer);
				// DgLog(DG_LOG_VERBOSE, "Drawing.SetTexture %d", id);
				ZnSetTextureAsCurrentFromID(id);
				break;
			}
			
			case ZN_CMD_CLEAR_TEXTURE: {
				// DgLog(DG_LOG_VERBOSE, "Drawing.ClearTexture");
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->default_texture_id);
				break;
			}
			
			case ZN_CMD_DRAW_TRIS: {
				size_t vertex_count = DgMemoryStreamReadUInt32(this->buffer);
				ZnVertex *data = DgMemoryStreamGetHeadPointer(this->buffer);
				DgMemoryStreamSetpos(this->buffer, DG_MEMORY_STREAM_CUR, sizeof(ZnVertex) * vertex_count);
				
				// DgLog(DG_LOG_VERBOSE, "Drawing.DrawTris %zu <@ 0x%llx>", vertex_count, data);
				
				// for (size_t i = 0; i < vertex_count; i++) {
				// 	DgLog(DG_LOG_VERBOSE, "%f %f %f   %f %f   %d %d %d %d", data[i].x, data[i].y, data[i].z, data[i].u, data[i].v, data[i].r, data[i].g, data[i].b, data[i].a);
				// }
				
				GLint inPosition = ZnUseVertexAttrib(
					program,
					"inPosition",
					3,
					GL_FLOAT,
					GL_FALSE,
					sizeof(ZnVertex),
					&data[0].x
				);
				
				GLint inTextureCoords = ZnUseVertexAttrib(
					program,
					"inTextureCoords",
					2,
					GL_FLOAT,
					GL_FALSE,
					sizeof(ZnVertex),
					&data[0].u
				);
				
				GLint inColour = ZnUseVertexAttrib(
					program,
					"inColour",
					4,
					GL_UNSIGNED_BYTE,
					GL_TRUE,
					sizeof(ZnVertex),
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

DgError ZnGetFrameData(ZnContext * const this, size_t size, void *data, bool alpha) {
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
