#if defined(GFX_BACKEND_OPENGL)
# include "openglui.cpp"
# define GfxInit				OpenGLUIInit
# define GfxBeginDrawing		OpenGLUIBeginDrawing
# define GfxEndDrawing			OpenGLUIEndDrawing
# define GfxSetBackgroundColor	OpenGLUISetBackgroundColor
# define GfxSetFontAtlas		OpenGLUISetFontAtlas
# define GfxDrawQuad			OpenGLUIDrawQuad
# define GfxDrawSphere			OpenGLUIDrawSphere
# define GfxDrawSolidColorQuad	OpenGLUIDrawSolidColorQuad
# define GfxDrawOutlineQuad		OpenGLUIDrawOutlineQuad
# define GfxDrawGlyph			OpenGLUIDrawGlyph
# define GfxDrawLine			OpenGLUIDrawLine
# define GfxDrawCurve			OpenGLUIDrawCurve
# define GfxCropScreen			OpenGLUICropScreen
# define GfxClearCrop			OpenGLUIClearCrop

#elif defined(GFX_BACKEND_D3D11)
# include "d3d11ui.cpp"
# define GfxInit				D3D11UIInit
# define GfxBeginDrawing		D3D11UIBeginDrawing
# define GfxEndDrawing			D3D11UIEndDrawing
# define GfxSetBackgroundColor	D3D11UISetBackgroundColor
# define GfxSetFontAtlas		D3D11UISetFontAtlas
# define GfxDrawQuad			D3D11UIDrawQuad
# define GfxDrawSphere			D3D11UIDrawSphere
# define GfxDrawSolidColorQuad	D3D11UIDrawSolidColorQuad
# define GfxDrawOutlineQuad		D3D11UIDrawOutlineQuad
# define GfxDrawGlyph			D3D11UIDrawGlyph
# define GfxDrawImage			D3D11UIDrawImage
# define GfxDrawLine			D3D11UIDrawLine
# define GfxDrawCurve			D3D11UIDrawCurve
# define GfxCropScreen			D3D11UICropScreen
# define GfxClearCrop			D3D11UIClearCrop
#endif