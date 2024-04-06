#include "window.cpp"
#if defined(GFX_BACKEND_OPENGL)
# include "opengl.cpp"
//# define TextureId 				GLuint
//# define Filter					GLint
//# define GFX_BILINEAR				GL_LINEAR
//# define GFX_NEAREST				GL_NEAREST
//# define GfxGenerateTexture		OpenGLGenerateTexture
# define GfxSwapBuffers				OSOpenGLSwapBuffers
#elif defined(GFX_BACKEND_D3D11)
# include "d3d11.cpp"
//# define GfxGenerateTexture		D3D11GenerateTexture
# define GfxSwapBuffers				OSD3D11SwapBuffers
#endif
