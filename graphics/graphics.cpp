#include "geometry.cpp"
#include "image.cpp"
#include "window.cpp"
#include "opengl.cpp"
#define TextureId 							GLuint
#define Filter								GLint
#define GRAPHICS_SMOOTH 					GL_LINEAR
#define GRAPHICS_PIXELATED					GL_NEAREST

#define GraphicsSwapBuffers					Win32OpenGLSwapBuffers
#define GraphicsInit						OpenGLInit
#define GenerateTexture						OpenGLGenerateTexture
#define UpdateTextureData					OpenGLUpdateTextureData
#define DrawImage							OpenGLDrawImage
#define DrawBox2							OpenGLDrawBox2
#define DrawLine							OpenGLDrawLine
#define DrawCircle							OpenGLDrawCircle
#define DrawDisc							OpenGLDrawDisc
#define DrawCurve3							OpenGLDrawCurve3
#define DrawCurve4							OpenGLDrawCurve4
#define GraphicsClearScreen 				OpenGLClearScreen
#define GraphicsCropScreen					glScissor
#define GraphicsClearCrop					OpenGlClearCrop
#define GraphicsSetColor					OpenGLSetColor


TextureId LoadTexture(Arena* arena, const char* filePath, Filter filter){
	Image image = LoadImage(arena, filePath);
	return GenerateTexture(image, filter);
}

void DrawBox2RoundedLines(uint32 rgba, float32 lineWidth, Box2 box, float32 r) {
	DrawCircle(rgba, lineWidth, {{box.x1-r, box.y1-r}, r},   0,  90);
	DrawCircle(rgba, lineWidth, {{box.x0+r, box.y1-r}, r},  90, 180);
	DrawCircle(rgba, lineWidth, {{box.x0+r, box.y0+r}, r}, 180, 270);
	DrawCircle(rgba, lineWidth, {{box.x1-r, box.y0+r}, r}, 270, 360);

	Point2 points[2];
	Line2 line = {points, 2};

	points[0] = {box.x0+r, box.y1};
	points[1] = {box.x1-r, box.y1};
	DrawLine(rgba, lineWidth, line);

	points[0] = {box.x1, box.y0+r};
	points[1] = {box.x1, box.y1-r};
	DrawLine(rgba, lineWidth, line);

	points[0] = {box.x0+r, box.y0};
	points[1] = {box.x1-r, box.y0};
	DrawLine(rgba, lineWidth, line);

	points[0] = {box.x0, box.y0+r};
	points[1] = {box.x0, box.y1-r};
	DrawLine(rgba, lineWidth, line);
}

void DrawBox2Rounded(uint32 rgba, Box2 box, float32 r) {
	DrawDisc(rgba, {{box.x1-r, box.y1-r}, r},   0,  90);
	DrawDisc(rgba, {{box.x0+r, box.y1-r}, r},  90, 180);
	DrawDisc(rgba, {{box.x0+r, box.y0+r}, r}, 180, 270);
	DrawDisc(rgba, {{box.x1-r, box.y0+r}, r}, 270, 360);

	DrawBox2(rgba, {box.x0+r, box.y0, box.x1-r, box.y1});
	DrawBox2(rgba, {box.x0, box.y0+r, box.x0+r, box.y1-r});
	DrawBox2(rgba, {box.x1-r, box.y0+r, box.x1, box.y1-r});
}

void DrawBox2Lines(uint32 rgba, float32 lineWidth, Box2 box) {
	Point2 points[5] = { {box.x0, box.y0}, {box.x0, box.y1}, {box.x1, box.y1}, {box.x1, box.y0}, {box.x0, box.y0}};
	Line2 line = {points, 5};

	DrawLine(rgba, lineWidth, line);
}