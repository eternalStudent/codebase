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
#define GfxDrawText 				OpenGLDrawText
#define GfxDrawBox2 				OpenGLDrawBox2
#define GfxDrawTriangle 			OpenGLDrawTriangle
#define GfxDrawLine 				OpenGLDrawLine
#define GfxDrawCircle				OpenGLDrawCircle
#define GfxDrawDisc 				OpenGLDrawDisc
#define GfxDrawCurve3				OpenGLDrawCurve3
#define GfxDrawCurve4				OpenGLDrawCurve4
#define GfxClearScreen				OpenGLClearScreen
#define GfxCropScreen				glScissor
#define GfxClearCrop				OpenGlClearCrop
#define GfxSetColor 				OpenGLSetColor
#define GfxSwapBuffers				OSOpenGLSwapBuffers


TextureId GfxLoadTexture(Arena* arena, const char* filePath, Filter filter) {
	Image image = LoadImage(arena, filePath);
	return GfxGenerateTexture(image, filter);
}

void GfxDrawBox2RoundedLines(Box2 box, float32 r, float32 lineWidth, uint32 rgba) {
	GfxDrawCircle({{box.x1-r, box.y1-r}, r}, lineWidth, rgba,   0,  90);
	GfxDrawCircle({{box.x0+r, box.y1-r}, r}, lineWidth, rgba,  90, 180);
	GfxDrawCircle({{box.x0+r, box.y0+r}, r}, lineWidth, rgba, 180, 270);
	GfxDrawCircle({{box.x1-r, box.y0+r}, r}, lineWidth, rgba, 270, 360);

	Point2 points[2];
	Line2 line = {points, 2};

	points[0] = {box.x0+r, box.y1};
	points[1] = {box.x1-r, box.y1};
	GfxDrawLine(line, lineWidth, rgba);

	points[0] = {box.x1, box.y0+r};
	points[1] = {box.x1, box.y1-r};
	GfxDrawLine(line, lineWidth, rgba);

	points[0] = {box.x0+r, box.y0};
	points[1] = {box.x1-r, box.y0};
	GfxDrawLine(line, lineWidth, rgba);

	points[0] = {box.x0, box.y0+r};
	points[1] = {box.x0, box.y1-r};
	GfxDrawLine(line, lineWidth, rgba);
}

void GfxDrawBox2Rounded(Box2 box, float32 r, uint32 rgba) {
	GfxDrawDisc({{box.x1-r, box.y1-r}, r}, rgba,   0,  90);
	GfxDrawDisc({{box.x0+r, box.y1-r}, r}, rgba,  90, 180);
	GfxDrawDisc({{box.x0+r, box.y0+r}, r}, rgba, 180, 270);
	GfxDrawDisc({{box.x1-r, box.y0+r}, r}, rgba, 270, 360);

	GfxDrawBox2({box.x0+r, box.y0, box.x1-r, box.y1}, rgba);
	GfxDrawBox2({box.x0, box.y0+r, box.x0+r, box.y1-r}, rgba);
	GfxDrawBox2({box.x1-r, box.y0+r, box.x1, box.y1-r}, rgba);
}

void GfxDrawBox2Lines(Box2 box, float32 lineWidth, uint32 rgba) {
	Point2 points[5] = { {box.x0, box.y0}, {box.x0, box.y1}, {box.x1, box.y1}, {box.x1, box.y0}, {box.x0, box.y0}};
	Line2 line = {points, 5};

	GfxDrawLine(line, lineWidth, rgba);
}