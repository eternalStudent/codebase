#include "geometry.cpp"
#include "image.cpp"
#include "window.cpp"
#include "opengl.cpp"
#define TextureId 					GLuint
#define Filter						GLint
#define GFX_SMOOTH 					GL_LINEAR
#define GFX_PIXELATED				GL_NEAREST

#define GfxInit						OpenGLInit
#define GfxGenerateTexture			OpenGLGenerateTexture
#define GfxUpdateTextureData		OpenGLUpdateTextureData
#define GfxDrawImage				OpenGLDrawImage
#define GfxDrawImageMono			OpenGLDrawImageMono
#define GfxDrawBox2					OpenGLDrawBox2
#define GfxDrawLine					OpenGLDrawLine
#define GfxDrawCircle				OpenGLDrawCircle
#define GfxDrawDisc					OpenGLDrawDisc
#define GfxDrawCurve3				OpenGLDrawCurve3
#define GfxDrawCurve4				OpenGLDrawCurve4
#define GfxClearScreen 				OpenGLClearScreen
#define GfxCropScreen				glScissor
#define GfxClearCrop				OpenGlClearCrop
#define GfxSetColor					OpenGLSetColor
#define GfxSwapBuffers				OsOpenGLSwapBuffers


TextureId GfxLoadTexture(Arena* arena, const char* filePath, Filter filter) {
	Image image = LoadImage(arena, filePath);
	return GfxGenerateTexture(image, filter);
}

void GfxDrawBox2RoundedLines(uint32 rgba, float32 lineWidth, Box2 box, float32 r) {
	GfxDrawCircle(rgba, lineWidth, {{box.x1-r, box.y1-r}, r},   0,  90);
	GfxDrawCircle(rgba, lineWidth, {{box.x0+r, box.y1-r}, r},  90, 180);
	GfxDrawCircle(rgba, lineWidth, {{box.x0+r, box.y0+r}, r}, 180, 270);
	GfxDrawCircle(rgba, lineWidth, {{box.x1-r, box.y0+r}, r}, 270, 360);

	Point2 points[2];
	Line2 line = {points, 2};

	points[0] = {box.x0+r, box.y1};
	points[1] = {box.x1-r, box.y1};
	GfxDrawLine(rgba, lineWidth, line);

	points[0] = {box.x1, box.y0+r};
	points[1] = {box.x1, box.y1-r};
	GfxDrawLine(rgba, lineWidth, line);

	points[0] = {box.x0+r, box.y0};
	points[1] = {box.x1-r, box.y0};
	GfxDrawLine(rgba, lineWidth, line);

	points[0] = {box.x0, box.y0+r};
	points[1] = {box.x0, box.y1-r};
	GfxDrawLine(rgba, lineWidth, line);
}

void GfxDrawBox2Rounded(uint32 rgba, Box2 box, float32 r) {
	GfxDrawDisc(rgba, {{box.x1-r, box.y1-r}, r},   0,  90);
	GfxDrawDisc(rgba, {{box.x0+r, box.y1-r}, r},  90, 180);
	GfxDrawDisc(rgba, {{box.x0+r, box.y0+r}, r}, 180, 270);
	GfxDrawDisc(rgba, {{box.x1-r, box.y0+r}, r}, 270, 360);

	GfxDrawBox2(rgba, {box.x0+r, box.y0, box.x1-r, box.y1});
	GfxDrawBox2(rgba, {box.x0, box.y0+r, box.x0+r, box.y1-r});
	GfxDrawBox2(rgba, {box.x1-r, box.y0+r, box.x1, box.y1-r});
}

void GfxDrawBox2Lines(uint32 rgba, float32 lineWidth, Box2 box) {
	Point2 points[5] = { {box.x0, box.y0}, {box.x0, box.y1}, {box.x1, box.y1}, {box.x1, box.y0}, {box.x0, box.y0}};
	Line2 line = {points, 5};

	GfxDrawLine(rgba, lineWidth, line);
}