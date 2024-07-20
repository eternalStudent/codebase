// TODO:
// This is meant to be a base for different graphics layers.
// right now I only have ui graphics layer, and it is difficult to say apriori
// what should be in a base layer and what should be in the "inherited" layers.
// Seems reasonable to assume that textures should be in the base layer.

#include "window.cpp"

#if defined(GFX_BACKEND_OPENGL)
# include "opengl.cpp"
# define GfxTexture 				GLuint
# define GfxFilter  				GLint
# define GFX_BILINEAR				GL_LINEAR
# define GFX_NEAREST				GL_NEAREST
# define GfxGenerateTexture			OpenGLGenerateTexture
# define GfxSwapBuffers				OSOpenGLSwapBuffers

#elif defined(GFX_BACKEND_D3D11)
# include "d3d11.cpp"
# define GfxTexture 				D3D11Texture
# define GfxFilter  				D3D11Filter
# define GFX_BILINEAR				D3D11_BILINEAR
# define GFX_NEAREST				D3D11_NEAREST
# define GfxGenerateTexture			D3D11GenerateTexture
# define GfxSwapBuffers				OSD3D11SwapBuffers
#endif



#define GFX_SRGB              		1 << 0
#define GFX_MULTISAMPLE       		1 << 1
#define GFX_SCISSOR           		1 << 2

// TODO:
#define GFX_PREMULTIPLIEDALPHA		1 << 3