#include <gl/GL.h>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef GLuint WINAPI type_glCreateShader(GLenum type);
typedef void WINAPI type_glShaderSource(GLuint shader, GLsizei count, GLchar** string, GLint* length);
typedef void WINAPI type_glCompileShader(GLuint shader);
typedef void WINAPI type_glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
typedef void WINAPI type_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void WINAPI type_glDeleteShader(GLuint shader);

typedef GLuint WINAPI type_glCreateProgram(void);
typedef void WINAPI type_glAttachShader(GLuint program, GLuint shader);
typedef void WINAPI type_glLinkProgram(GLuint program);
typedef void WINAPI type_glGetProgramiv(GLuint program, GLenum pname, GLint* params);
typedef void WINAPI type_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void WINAPI type_glUseProgram(GLuint program);
typedef void WINAPI type_glValidateProgram(GLuint program);  // ??

typedef void WINAPI type_glEnableVertexAttribArray(GLuint index);
typedef void WINAPI type_glDisableVertexAttribArray(GLuint index);
typedef GLint WINAPI type_glGetAttribLocation(GLuint program, const GLchar* name);
typedef void WINAPI type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void WINAPI type_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer);
typedef void WINAPI type_glBindVertexArray(GLuint array);
typedef void WINAPI type_glGenVertexArrays(GLsizei n, GLuint* arrays);
typedef void WINAPI type_glBindBuffer(GLenum target, GLuint buffer);
typedef void WINAPI type_glGenBuffers(GLsizei n, GLuint* buffers);
typedef void WINAPI type_glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void WINAPI type_glActiveTexture(GLenum texture);
typedef void WINAPI type_glDeleteProgram(GLuint program);
typedef void WINAPI type_glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
typedef void WINAPI type_glDeleteBuffers(GLsizei n, const GLuint* buffers);
typedef void WINAPI type_glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
typedef void WINAPI type_glDrawBuffers(GLsizei n, const GLenum* bufs);

typedef GLint WINAPI type_glGetUniformLocation(GLuint program, const GLchar* name);
typedef void WINAPI type_glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI type_glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
typedef void WINAPI type_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void WINAPI type_glUniform1i(GLint location, GLint v0);
typedef void WINAPI type_glUniform1f(GLint location, GLfloat v0);
typedef void WINAPI type_glUniform2f(GLint location, GLfloat v0, GLfloat v1);
typedef void WINAPI type_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v3);
typedef void WINAPI type_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v3, GLfloat v4);

typedef void (APIENTRY *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void WINAPI type_glDebugMessageCallback(DEBUGPROC callback, const void* userParam);

typedef void WINAPI type_glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void WINAPI type_glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void WINAPI type_glGenFramebuffers(GLsizei n, GLuint *framebuffers);
typedef void WINAPI type_glBindFramebuffer(GLenum target, GLuint framebuffer);
typedef void WINAPI type_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum WINAPI type_glCheckFramebufferStatus(GLenum target);
typedef void WINAPI type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void WINAPI type_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);

#define Win32DefineOpenGLFunction(Name) static type_##Name * Name

Win32DefineOpenGLFunction(glAttachShader);
Win32DefineOpenGLFunction(glCompileShader);
Win32DefineOpenGLFunction(glCreateProgram);
Win32DefineOpenGLFunction(glCreateShader);
Win32DefineOpenGLFunction(glLinkProgram);
Win32DefineOpenGLFunction(glShaderSource);
Win32DefineOpenGLFunction(glUseProgram);
Win32DefineOpenGLFunction(glGetProgramInfoLog);
Win32DefineOpenGLFunction(glGetShaderInfoLog);
Win32DefineOpenGLFunction(glValidateProgram);
Win32DefineOpenGLFunction(glGetProgramiv);
Win32DefineOpenGLFunction(glGetShaderiv);
Win32DefineOpenGLFunction(glDeleteShader);

Win32DefineOpenGLFunction(glEnableVertexAttribArray);
Win32DefineOpenGLFunction(glDisableVertexAttribArray);
Win32DefineOpenGLFunction(glGetAttribLocation);
Win32DefineOpenGLFunction(glVertexAttribPointer);
Win32DefineOpenGLFunction(glVertexAttribIPointer);
Win32DefineOpenGLFunction(glBindVertexArray);
Win32DefineOpenGLFunction(glGenVertexArrays);
Win32DefineOpenGLFunction(glBindBuffer);
Win32DefineOpenGLFunction(glGenBuffers);
Win32DefineOpenGLFunction(glBufferData);
Win32DefineOpenGLFunction(glActiveTexture);
Win32DefineOpenGLFunction(glDeleteProgram);
Win32DefineOpenGLFunction(glDeleteFramebuffers);
Win32DefineOpenGLFunction(glDeleteBuffers);
Win32DefineOpenGLFunction(glDeleteVertexArrays);
Win32DefineOpenGLFunction(glDrawBuffers);

Win32DefineOpenGLFunction(glGetUniformLocation);
Win32DefineOpenGLFunction(glUniform1fv);
Win32DefineOpenGLFunction(glUniform4fv);
Win32DefineOpenGLFunction(glUniformMatrix3fv);
Win32DefineOpenGLFunction(glUniformMatrix4fv);
Win32DefineOpenGLFunction(glUniform1i);
Win32DefineOpenGLFunction(glUniform1f);
Win32DefineOpenGLFunction(glUniform2f);
Win32DefineOpenGLFunction(glUniform3f);
Win32DefineOpenGLFunction(glUniform4f);

Win32DefineOpenGLFunction(glDebugMessageCallback);
Win32DefineOpenGLFunction(glBlendColor);
Win32DefineOpenGLFunction(glDebugMessageControl);
Win32DefineOpenGLFunction(glGenFramebuffers);
Win32DefineOpenGLFunction(glBindFramebuffer);
Win32DefineOpenGLFunction(glFramebufferTexture2D);
Win32DefineOpenGLFunction(glCheckFramebufferStatus);
Win32DefineOpenGLFunction(glBlitFramebuffer);
Win32DefineOpenGLFunction(glTexImage3D);

typedef BOOL WINAPI type_wglSwapIntervalEXT(int interval);
typedef HGLRC WINAPI type_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int* attribList);

#define Win32GetOpenGLFunction(Name) Name = (type_##Name *)wglGetProcAddress(#Name)

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x0002

BOOL Win32CreateRenderingContext(HDC dc) {
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
	HGLRC context = wglCreateContext(dc);
	if (!context)
		return FAIL("failed to create rendering context");

	if (!wglMakeCurrent(dc, context)) 
		return FAIL("failed to make rendering context current");

	type_wglCreateContextAttribsARB* wglCreateModernContext = (type_wglCreateContextAttribsARB*)wglGetProcAddress("wglCreateContextAttribsARB");
	if (!wglCreateModernContext)
		return FAIL("failed to get address of create modern rendering context");

	int Win32OpenGLAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		// If the WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB is set in
		// WGL_CONTEXT_FLAGS_ARB, then a <forward - compatible> context will be
		// created.Forward - compatible contexts are defined only for OpenGL
		// versions 3.0 and later.They must not support functionality marked
		// as <deprecated> by that version of the API, while a
		// non - forward - compatible context must support all functionality in
		// that version, deprecated or not.
		WGL_CONTEXT_FLAGS_ARB, 
		/*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB*/ WGL_CONTEXT_DEBUG_BIT_ARB,
		
		// If the WGL_CONTEXT_CORE_PROFILE_BIT_ARB bit is set in the attribute value, 
		// then a context implementing the <core> profile of OpenGL is returned
		WGL_CONTEXT_PROFILE_MASK_ARB, 
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0,
	};
	HGLRC modern = wglCreateModernContext(dc, 0, Win32OpenGLAttribs);
	if (!modern)
		return FAIL("failed to create modern rendering context");

	if (!wglMakeCurrent(dc, modern))
		return FAIL("failed to make modern rendering context current");

	return TRUE;
}

BOOL Win32SetPixelFormat(HDC dc) {
	// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor
	PIXELFORMATDESCRIPTOR requestedPixelFormat = {};
	requestedPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	requestedPixelFormat.nVersion = 1;
	requestedPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	requestedPixelFormat.cColorBits = 32;
	requestedPixelFormat.cAlphaBits = 8;
	requestedPixelFormat.iLayerType = PFD_MAIN_PLANE;

	int closestMatchPixelFormatIndex = ChoosePixelFormat(dc, &requestedPixelFormat);
	if (!closestMatchPixelFormatIndex)
		return FAIL("failed to choose pixel format");
	
	PIXELFORMATDESCRIPTOR closestMatchPixelFormat;
	int maximumPixelFormatIndex = DescribePixelFormat(dc, closestMatchPixelFormatIndex, sizeof(closestMatchPixelFormat), &closestMatchPixelFormat);
	if (!maximumPixelFormatIndex)
		return FAIL("failed to describe pixel format");
	
	BOOL success = SetPixelFormat(dc, closestMatchPixelFormatIndex, &closestMatchPixelFormat);
	if (success == FALSE)
		return FAIL("failed to set pixel format");

	return TRUE;
}

BOOL Win32OpenGLInit() {
	HWND windowHandle = Win32GetWindowHandle();
	HDC dc = GetDC(windowHandle);
	if (!dc) return FAIL("failed to get DC");
	if (!Win32SetPixelFormat(dc)) return FALSE;
	if (!Win32CreateRenderingContext(dc)) return FALSE;

	Win32GetOpenGLFunction(glAttachShader);
	Win32GetOpenGLFunction(glCompileShader);
	Win32GetOpenGLFunction(glCreateProgram);
	Win32GetOpenGLFunction(glCreateShader);
	Win32GetOpenGLFunction(glLinkProgram);
	Win32GetOpenGLFunction(glShaderSource);
	Win32GetOpenGLFunction(glUseProgram);
	Win32GetOpenGLFunction(glGetProgramInfoLog);
	Win32GetOpenGLFunction(glGetShaderInfoLog);
	Win32GetOpenGLFunction(glValidateProgram);
	Win32GetOpenGLFunction(glGetProgramiv);
	Win32GetOpenGLFunction(glGetShaderiv);
	Win32GetOpenGLFunction(glDeleteShader);

	Win32GetOpenGLFunction(glEnableVertexAttribArray);
	Win32GetOpenGLFunction(glDisableVertexAttribArray);
	Win32GetOpenGLFunction(glGetAttribLocation);
	Win32GetOpenGLFunction(glVertexAttribPointer);
	Win32GetOpenGLFunction(glVertexAttribIPointer);
	Win32GetOpenGLFunction(glBindVertexArray);
	Win32GetOpenGLFunction(glGenVertexArrays);
	Win32GetOpenGLFunction(glBindBuffer);
	Win32GetOpenGLFunction(glGenBuffers);
	Win32GetOpenGLFunction(glBufferData);
	Win32GetOpenGLFunction(glActiveTexture);
	Win32GetOpenGLFunction(glDeleteProgram);
	Win32GetOpenGLFunction(glDeleteFramebuffers);
	Win32GetOpenGLFunction(glDeleteBuffers);
	Win32GetOpenGLFunction(glDeleteVertexArrays);
	Win32GetOpenGLFunction(glDrawBuffers);

	Win32GetOpenGLFunction(glGetUniformLocation);
	Win32GetOpenGLFunction(glUniform1fv);
	Win32GetOpenGLFunction(glUniform4fv);
	Win32GetOpenGLFunction(glUniformMatrix3fv);
	Win32GetOpenGLFunction(glUniformMatrix4fv);
	Win32GetOpenGLFunction(glUniform1i);
	Win32GetOpenGLFunction(glUniform1f);
	Win32GetOpenGLFunction(glUniform2f);
	Win32GetOpenGLFunction(glUniform3f);
	Win32GetOpenGLFunction(glUniform4f);

	Win32GetOpenGLFunction(glDebugMessageCallback);
	Win32GetOpenGLFunction(glBlendColor);
	Win32GetOpenGLFunction(glDebugMessageControl);
	Win32GetOpenGLFunction(glGenFramebuffers);
	Win32GetOpenGLFunction(glBindFramebuffer);
	Win32GetOpenGLFunction(glFramebufferTexture2D);
	Win32GetOpenGLFunction(glCheckFramebufferStatus);
	Win32GetOpenGLFunction(glBlitFramebuffer);
	Win32GetOpenGLFunction(glTexImage3D);

	type_wglSwapIntervalEXT* wglSwapInteravl = (type_wglSwapIntervalEXT*)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapInteravl) wglSwapInteravl(1);
	else LOG("failed to set swap interval");

	ReleaseDC(windowHandle, dc);

	return TRUE;
}

BOOL Win32OpenGLSwapBuffers() {
	HDC dc = wglGetCurrentDC();
	if (dc == NULL)
		return FAIL("failed to get current DC");

	BOOL ok = SwapBuffers(dc);
	if (!ok)
		LOG("failed to swap buffers");
	return ok;
}

#define GL_CONSTANT_COLOR               0x8001

#define GL_CLAMP_TO_EDGE 				0x812F

#define GL_FRAMEBUFFER_UNDEFINED        0x8219
#define GL_RG 							0x8227
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 	0x8242
#define GL_DEBUG_SEVERITY_NOTIFICATION  0x826B

#define GL_TEXTURE0 					0x84C0
#define GL_TEXTURE1 					0x84C1
#define GL_TEXTURE2 					0x84C2
#define GL_TEXTURE3 					0x84C3
#define GL_TEXTURE4 					0x84C4
#define GL_TEXTURE5 					0x84C5
#define GL_TEXTURE6 					0x84C6
#define GL_TEXTURE7 					0x84C7

#define GL_ARRAY_BUFFER 				0x8892
#define GL_ELEMENT_ARRAY_BUFFER 		0x8893
#define GL_STREAM_DRAW					0x88E0
#define GL_STREAM_READ					0x88E1
#define GL_STREAM_COPY					0x88E2
#define GL_STATIC_DRAW					0x88E4
#define GL_STATIC_READ					0x88E5
#define GL_STATIC_COPY					0x88E6
#define GL_DYNAMIC_DRAW 				0x88E8
#define GL_DYNAMIC_READ 				0x88E9
#define GL_DYNAMIC_COPY 				0x88EA

#define GL_FRAGMENT_SHADER				0x8B30
#define GL_VERTEX_SHADER				0x8B31
#define GL_COMPILE_STATUS				0x8B81
#define GL_LINK_STATUS					0x8B82
#define GL_VALIDATE_STATUS				0x8B83

#define GL_TEXTURE_2D_ARRAY             0x8C1A
#define GL_SRGB 						0x8C40
#define GL_SRGB8						0x8C41
#define GL_SRGB_ALPHA					0x8C42
#define GL_SRGB8_ALPHA8					0x8C43
#define GL_READ_FRAMEBUFFER             0x8CA8
#define GL_DRAW_FRAMEBUFFER             0x8CA9
#define GL_FRAMEBUFFER_COMPLETE                      0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT         0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER        0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER        0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED                   0x8CDD

#define GL_FRAMEBUFFER                  0x8D40
#define GL_FRAMEBUFFER_SRGB 			0x8DB9
#define GL_COLOR_ATTACHMENT0            0x8CE0


#define GL_DEBUG_SEVERITY_HIGH          0x9146
#define GL_DEBUG_SEVERITY_MEDIUM        0x9147
#define GL_DEBUG_SEVERITY_LOW           0x9148
#define GL_DEBUG_OUTPUT 				0x92E0