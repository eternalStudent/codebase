#include <gl/GL.h>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;


#define X(N, R, P) typedef R type_##N P;
#include "opengldefs"
#include "wgldefs"
#undef X

#define X(N, R, P) static type_##N * N;
#include "opengldefs"
#include "wgldefs"
#undef X


#define WGL_CONTEXT_MAJOR_VERSION_ARB			0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB			0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB				0x2093
#define WGL_CONTEXT_FLAGS_ARB					0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB			0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB				0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB	0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB		0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x0002

#define WGL_NUMBER_PIXEL_FORMATS_ARB			0x2000
#define WGL_DRAW_TO_WINDOW_ARB					0x2001
#define WGL_DRAW_TO_BITMAP_ARB					0x2002
#define WGL_ACCELERATION_ARB					0x2003
#define WGL_NEED_PALETTE_ARB					0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB				0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB				0x2006
#define WGL_SWAP_METHOD_ARB						0x2007
#define WGL_NUMBER_OVERLAYS_ARB					0x2008
#define WGL_NUMBER_UNDERLAYS_ARB				0x2009
#define WGL_TRANSPARENT_ARB						0x200A
#define WGL_SHARE_DEPTH_ARB						0x200C
#define WGL_SHARE_STENCIL_ARB					0x200D
#define WGL_SHARE_ACCUM_ARB						0x200E
#define WGL_SUPPORT_GDI_ARB						0x200F
#define WGL_SUPPORT_OPENGL_ARB					0x2010
#define WGL_DOUBLE_BUFFER_ARB					0x2011
#define WGL_STEREO_ARB							0x2012
#define WGL_PIXEL_TYPE_ARB						0x2013
#define WGL_COLOR_BITS_ARB						0x2014
#define WGL_RED_BITS_ARB						0x2015
#define WGL_RED_SHIFT_ARB						0x2016
#define WGL_GREEN_BITS_ARB						0x2017
#define WGL_GREEN_SHIFT_ARB						0x2018
#define WGL_BLUE_BITS_ARB						0x2019
#define WGL_BLUE_SHIFT_ARB						0x201a
#define WGL_ALPHA_BITS_ARB						0x201b
#define WGL_ALPHA_SHIFT_ARB						0x201c
#define WGL_ACCUM_BITS_ARB						0x201d
#define WGL_ACCUM_RED_BITS_ARB					0x201e
#define WGL_ACCUM_GREEN_BITS_ARB				0x201f
#define WGL_ACCUM_BLUE_BITS_ARB					0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB				0x2021
#define WGL_DEPTH_BITS_ARB						0x2022
#define WGL_STENCIL_BITS_ARB					0x2023
#define WGL_AUX_BUFFERS_ARB						0x2024
#define WGL_NO_ACCELERATION_ARB					0x2025
#define WGL_GENERIC_ACCELERATION_ARB			0x2026
#define WGL_FULL_ACCELERATION_ARB				0x2027
#define WGL_SWAP_EXCHANGE_ARB					0x2028
#define WGL_SWAP_COPY_ARB						0x2029
#define WGL_SWAP_UNDEFINED_ARB					0x202A
#define WGL_TYPE_RGBA_ARB						0x202B
#define WGL_TYPE_COLORINDEX_ARB					0x202C

#define WGL_TRANSPARENT_RED_VALUE_ARB			0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB			0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB			0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB			0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB			0x203B

#define WGL_SAMPLES_ARB							0x2042

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB		0x20A9


BOOL Win32OpenGLInit() {
	WNDCLASSW dummyClass = {0};
	dummyClass.lpfnWndProc = DefWindowProcW;
	dummyClass.hInstance = NULL;
	dummyClass.lpszClassName = L"dummy window class";
	if (!RegisterClassW(&dummyClass))
		return FAIL("failed to register dummy window class");

	HWND dummywindow = CreateWindowW(
		dummyClass.lpszClassName,
		NULL,
		0, 0, 0, 0, 0,
		0, 0, NULL, 0
	);
	if (!dummywindow)
		return FAIL("failed to create dummy window");

	HDC dc = GetDC(dummywindow);
	if (!dc) 
		return FAIL("failed to get DC");

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;

	if (!SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd))
		return FAIL("failed to set pixel format for dummy context");

	HGLRC rc = wglCreateContext(dc);
	if (!rc)
		return FAIL("failed to create dummy context");

	HDC pdc = wglGetCurrentDC();
	HGLRC prc = wglGetCurrentContext();

	if (!wglMakeCurrent(dc, rc)) {
		LOG("failed to make dummy context current");
		wglMakeCurrent(pdc, prc);
		wglDeleteContext(rc);
		return FALSE;
	}


#define X(N, R, P) N = (type_##N *)wglGetProcAddress(#N);
#include "wgldefs"
#undef X


	wglMakeCurrent(pdc, prc);
	wglDeleteContext(rc);

	HWND windowHandle = Win32GetWindowHandle();
	dc = GetDC(windowHandle);

	// Choose Pixel Format
	//------------------------
	// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor

	// TODO: gain better understanding of the attributes
	int pixelFormatAttributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, TRUE,
		WGL_ACCELERATION_ARB, 	WGL_FULL_ACCELERATION_ARB,
		WGL_SWAP_METHOD_ARB, 	WGL_SWAP_EXCHANGE_ARB,
		WGL_SUPPORT_OPENGL_ARB, TRUE,
		WGL_DOUBLE_BUFFER_ARB, 	TRUE,
		WGL_PIXEL_TYPE_ARB, 	WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,		24,
		WGL_RED_BITS_ARB,		8,
		WGL_GREEN_BITS_ARB,		8,
		WGL_BLUE_BITS_ARB,		8,
		WGL_SAMPLES_ARB, 		4,
		0
	};
		  
	UINT num_formats = 0;
	int pixelFormat;
	if (!wglChoosePixelFormatARB(dc, pixelFormatAttributes, 0, 1, &pixelFormat, &num_formats))
		return FAIL("failed to choose pixel format");

	PIXELFORMATDESCRIPTOR format_desc = {sizeof(PIXELFORMATDESCRIPTOR)};
	if (!DescribePixelFormat(dc, pixelFormat, sizeof(format_desc), &format_desc))
		return FAIL("failed to retrieve description for selected pixel format");

	if (!SetPixelFormat(dc, pixelFormat, &format_desc))
		return FAIL("failed to set pixel format");

	// Create Context
	//-------------------
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt

	HGLRC context = wglCreateContext(dc);
	if (!context)
		return FAIL("failed to create rendering context");

	if (!wglMakeCurrent(dc, context)) 
		return FAIL("failed to make rendering context current");

	// TODO: gain better understanding of the attributes
	int contextAttributes[] = {
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
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0,
	};
	HGLRC modern = wglCreateContextAttribsARB(dc, 0, contextAttributes);
	if (!modern)
		return FAIL("failed to create modern rendering context");

	if (!wglMakeCurrent(dc, modern))
		return FAIL("failed to make modern rendering context current");


#define X(N, R, P) N = (type_##N *)wglGetProcAddress(#N);
#include "opengldefs"
#undef X


	wglSwapIntervalEXT(1);

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

#define GL_MULTISAMPLE					0x809D

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

#define GL_TEXTURE_2D_ARRAY				0x8C1A
#define GL_SRGB 						0x8C40
#define GL_SRGB8						0x8C41
#define GL_SRGB_ALPHA					0x8C42
#define GL_SRGB8_ALPHA8					0x8C43
#define GL_READ_FRAMEBUFFER				0x8CA8
#define GL_DRAW_FRAMEBUFFER				0x8CA9

#define GL_FRAMEBUFFER_COMPLETE							0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 			0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT	0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER			0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER			0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED						0x8CDD

#define GL_FRAMEBUFFER                  0x8D40
#define GL_FRAMEBUFFER_SRGB 			0x8DB9
#define GL_COLOR_ATTACHMENT0            0x8CE0

#define GL_DEBUG_SEVERITY_HIGH          0x9146
#define GL_DEBUG_SEVERITY_MEDIUM        0x9147
#define GL_DEBUG_SEVERITY_LOW           0x9148
#define GL_DEBUG_OUTPUT 				0x92E0
