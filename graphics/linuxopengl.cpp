#include <EGL/egl.h>
#include <GL/glcorearb.h>

// make sure you use functions that are valid for selected GL version (specified when context is created)
#define GL_FUNCTIONS(X) \
	X(PFNGLENABLEPROC,                   glEnable                   ) \
	X(PFNGLDISABLEPROC,                  glDisable                  ) \
	X(PFNGLBLENDFUNCPROC,                glBlendFunc                ) \
	X(PFNGLVIEWPORTPROC,                 glViewport                 ) \
	X(PFNGLCLEARCOLORPROC,               glClearColor               ) \
	X(PFNGLCLEARPROC,                    glClear                    ) \
	X(PFNGLDRAWARRAYSPROC,               glDrawArrays               ) \
	X(PFNGLCREATEBUFFERSPROC,            glCreateBuffers            ) \
	X(PFNGLNAMEDBUFFERSTORAGEPROC,       glNamedBufferStorage       ) \
	X(PFNGLBINDVERTEXARRAYPROC,          glBindVertexArray          ) \
	X(PFNGLCREATEVERTEXARRAYSPROC,       glCreateVertexArrays       ) \
	X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding ) \
	X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,  glVertexArrayVertexBuffer  ) \
	X(PFNGLVERTEXARRAYATTRIBFORMATPROC,  glVertexArrayAttribFormat  ) \
	X(PFNGLENABLEVERTEXARRAYATTRIBPROC,  glEnableVertexArrayAttrib  ) \
	X(PFNGLCREATESHADERPROGRAMVPROC,     glCreateShaderProgramv     ) \
	X(PFNGLGETPROGRAMIVPROC,             glGetProgramiv             ) \
	X(PFNGLGETPROGRAMINFOLOGPROC,        glGetProgramInfoLog        ) \
	X(PFNGLGENPROGRAMPIPELINESPROC,      glGenProgramPipelines      ) \
	X(PFNGLUSEPROGRAMSTAGESPROC,         glUseProgramStages         ) \
	X(PFNGLBINDPROGRAMPIPELINEPROC,      glBindProgramPipeline      ) \
	X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC,  glProgramUniformMatrix2fv  ) \
	X(PFNGLBINDTEXTUREUNITPROC,          glBindTextureUnit          ) \
	X(PFNGLCREATETEXTURESPROC,           glCreateTextures           ) \
	X(PFNGLTEXTUREPARAMETERIPROC,        glTextureParameteri        ) \
	X(PFNGLTEXTURESTORAGE2DPROC,         glTextureStorage2D         ) \
	X(PFNGLTEXTURESUBIMAGE2DPROC,        glTextureSubImage2D        ) \
	X(PFNGLDEBUGMESSAGECALLBACKPROC,     glDebugMessageCallback     ) \
	X(PFNGLCREATESHADERPROC,			 glCreateShader   			) \
	X(PFNGLSHADERSOURCEPROC,			 glShaderSource   			) \
	X(PFNGLCOMPILESHADERPROC,			 glCompileShader   			) \
	X(PFNGLGETSHADERIVPROC,				 glGetShaderiv   			) \
	X(PFNGLGETSHADERINFOLOGPROC,		 glGetShaderInfoLog   		) \
	X(PFNGLCREATEPROGRAMPROC,			 glCreateProgram   			) \
	X(PFNGLATTACHSHADERPROC,			 glAttachShader   			) \
	X(PFNGLLINKPROGRAMPROC,				 glLinkProgram   			) \
	X(PFNGLDELETESHADERPROC,			 glDeleteShader   			) \
	X(PFNGLGENTEXTURESPROC,				 glGenTextures   			) \
	X(PFNGLBINDTEXTUREPROC,				 glBindTexture   			) \
	X(PFNGLTEXIMAGE2DPROC,				 glTexImage2D   			) \
	X(PFNGLTEXPARAMETERIPROC,			 glTexParameteri   			) \
	X(PFNGLTEXSUBIMAGE2DPROC,			 glTexSubImage2D   			) \
	X(PFNGLSCISSORPROC,					 glScissor   				) \
	X(PFNGLGENBUFFERSPROC,				 glGenBuffers   			) \
	X(PFNGLBINDBUFFERPROC,				 glBindBuffer   			) \
	X(PFNGLBUFFERDATAPROC,				 glBufferData   			) \
	X(PFNGLGENVERTEXARRAYSPROC,			 glGenVertexArrays   		) \
	X(PFNGLENABLEVERTEXATTRIBARRAYPROC,	 glEnableVertexAttribArray  ) \
	X(PFNGLVERTEXATTRIBPOINTERPROC,		 glVertexAttribPointer   	) \
	X(PFNGLPOLYGONMODEPROC,				 glPolygonMode  		 	) \
	X(PFNGLUSEPROGRAMPROC,				 glUseProgram  			 	) \
	X(PFNGLACTIVETEXTUREPROC,			 glActiveTexture 		  	) \
	X(PFNGLGETUNIFORMLOCATIONPROC,		 glGetUniformLocation 	  	) \
	X(PFNGLUNIFORM1IPROC,		 		 glUniform1i 			  	) \
	X(PFNGLDELETEVERTEXARRAYSPROC,		 glDeleteVertexArrays   	) \
	X(PFNGLDELETEBUFFERSPROC,			 glDeleteBuffers 		  	) \
	X(PFNGLUNIFORM4FPROC,				 glUniform4f  			 	) \
	X(PFNGLLINEWIDTHPROC,				 glLineWidth 			  	) 

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

EGLBoolean LinuxOpenGLInit() {
	WindowHandle handle = OsGetWindowHandle();

	// initialize EGL
	EGLDisplay display;
	{
		display = eglGetDisplay((EGLNativeDisplayType)handle.display);
		if (display == EGL_NO_DISPLAY)
			return FAIL("Failed to get EGL display");

		EGLint major, minor;
		if (!eglInitialize(display, &major, &minor)) {
			return FAIL("Cannot initialize EGL display");
		}
		if (major < 1 || (major == 1 && minor < 5)) {
			return FAIL("EGL version 1.5 or higher required");
		}
	}

	// choose OpenGL API for EGL, by default it uses OpenGL ES
	EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
	if (!ok)
		return FAIL("Failed to select OpenGL API for EGL");

	// choose EGL configuration
	EGLConfig config;
	{
		EGLint attr[] = {
			EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
			EGL_CONFORMANT,        EGL_OPENGL_BIT,
			EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
			EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

			EGL_RED_SIZE,      8,
			EGL_GREEN_SIZE,    8,
			EGL_BLUE_SIZE,     8,
			EGL_DEPTH_SIZE,   24,
			EGL_STENCIL_SIZE,  8,

			// uncomment for multisampled framebuffer
			//EGL_SAMPLE_BUFFERS, 1,
			//EGL_SAMPLES,        4, // 4x MSAA

			EGL_NONE,
		};

		EGLint count;
		if (!eglChooseConfig(display, attr, &config, 1, &count) || count != 1) {
			return FAIL("Cannot choose EGL config");
		}
	}

	// create EGL surface
	EGLSurface surface;
	{
		EGLint attr[] = {
			EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR, // or use EGL_GL_COLORSPACE_SRGB for sRGB framebuffer
			EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
			EGL_NONE,
		};

		surface = eglCreateWindowSurface(display, config, handle.window, attr);
		if (surface == EGL_NO_SURFACE) {
			return FAIL("Cannot create EGL surface");
		}
	}

	// create EGL context
	EGLContext context;
	{
		EGLint attr[] = {
			EGL_CONTEXT_MAJOR_VERSION, 4,
			EGL_CONTEXT_MINOR_VERSION, 5,
			EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
			EGL_NONE,
		};

		context = eglCreateContext(display, config, EGL_NO_CONTEXT, attr);
		if (context == EGL_NO_CONTEXT) {
			return FAIL("Cannot create EGL context, OpenGL 4.5 not supported?");
		}
	}

	ok = eglMakeCurrent(display, surface, surface, context);
	if (!ok)
		return FAIL("Failed to make context current");

	// load OpenGL functions
#define X(type, name) name = (type)eglGetProcAddress(#name); ASSERT(name);
	GL_FUNCTIONS(X)
#undef X

	// use 0 to disable vsync
	int vsync = 1;
	ok = eglSwapInterval(display, vsync);
	if (!ok)
		LOG("Failed to set swap interval");

	return 1;
}

EGLBoolean LinuxOpenGLSwapBuffers() {
	WindowHandle handle = OsGetWindowHandle();
	EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)handle.display);
	if (display == EGL_NO_DISPLAY)
		return FAIL("Failed to get EGL display");

	return eglSwapBuffers(display, eglGetCurrentSurface(EGL_READ));
}