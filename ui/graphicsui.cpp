enum Quadrant {
	Quadrant_I,
	Quadrant_II,
	Quadrant_III,
	Quadrant_IV,
};

#if defined(GFX_BACKEND_OPENGL)
# include "openglui.cpp"
# define GfxInit					OpenGLUIInit
# define GfxBeginDrawing			OpenGLUIBeginDrawing
# define GfxEndDrawing				OpenGLUIEndDrawing
# define GfxSetBackgroundColor		OpenGLUISetBackgroundColor
# define GfxSetFontAtlas			OpenGLUISetFontAtlas
# define GfxDrawSolidColorQuad		OpenGLUIDrawSolidColorQuad
# define GfxDrawOutlineQuad			OpenGLUIDrawOutlineQuad
# define GfxDrawVerticalGradQuad	OpenGLUIDrawVerticalGradQuad
# define GfxDrawHorizontalGradQuad	OpenGLUIDrawHorizontalGradQuad
# define GfxDrawSphere				OpenGLUIDrawSphere
# define GfxDrawGlyph				OpenGLUIDrawGlyph
# define GfxDrawLine				OpenGLUIDrawLine
# define GfxDrawCurve				OpenGLUIDrawCurve
# define GfxDrawShadow				OpenGLUIDrawShadow
# define GfxDrawSemiSphere			OpenGLUIDrawSemiSphere
# define GfxCropScreen				OpenGLUICropScreen
# define GfxClearCrop				OpenGLUIClearCrop

#elif defined(GFX_BACKEND_D3D11)
# include "d3d11ui.cpp"
# define GfxInit					D3D11UIInit
# define GfxBeginDrawing			D3D11UIBeginDrawing
# define GfxEndDrawing				D3D11UIEndDrawing
# define GfxSetBackgroundColor		D3D11UISetBackgroundColor
# define GfxSetFontAtlas			D3D11UISetFontAtlas
# define GfxDrawSolidColorQuad		D3D11UIDrawSolidColorQuad
# define GfxDrawOutlineQuad			D3D11UIDrawOutlineQuad
# define GfxDrawVerticalGradQuad	D3D11UIDrawVerticalGradQuad
# define GfxDrawHorizontalGradQuad	D3D11UIDrawHorizontalGradQuad
# define GfxDrawSphere				D3D11UIDrawSphere
# define GfxDrawGlyph				D3D11UIDrawGlyph
# define GfxDrawImage				D3D11UIDrawImage
# define GfxDrawLine				D3D11UIDrawLine
# define GfxDrawCurve				D3D11UIDrawCurve
# define GfxDrawShadow				D3D11UIDrawShadow
# define GfxDrawSemiSphere			D3D11UIDrawSemiSphere
# define GfxCropScreen				D3D11UICropScreen
# define GfxClearCrop				D3D11UIClearCrop
#endif

// TODO: doesn't solve corner cases
void GfxDrawPath(Point2 pos0, Point2 pos1, bool vertFirst, float32 radius, float32 thickness, Color color0, Color color1) {
	float32 x0 = MIN(pos0.x, pos1.x);
	float32 x1 = MAX(pos0.x, pos1.x);
	float32 y0 = MIN(pos0.y, pos1.y);
	float32 y1 = MAX(pos0.y, pos1.y);
	float32 dx = x1 - x0;
	float32 dy = y1 - y0;
	float32 half = 0.5f*thickness;
	float32 length = dx + 2*thickness - 4*radius + dy;
	if (vertFirst) {
		float32 midy = y0 + 0.5f*dy;
		float32 l1 = 0.5f*dy + half - radius;
		float32 t1 = l1/length;
		float32 l2 = dx + thickness - 2*radius;
		float32 t2 = (l1 + l2)/length;

		Color c1 = t1*color0 + (1 - t1)*color1;
		Color c2 = t2*color0 + (1 - t2)*color1;


		if (pos0.x < pos1.x) {
			GfxDrawVerticalGradQuad({x0 - half, y0, x0 + half, midy + half - radius}, color0, c1);
			GfxDrawHorizontalGradQuad({x0 - half + radius, midy - half, x1 + half - radius, midy + half}, c1, c2);
			GfxDrawVerticalGradQuad({x1 - half, midy - half + radius, x1 + half, y1}, c2, color1);

			GfxDrawSemiSphere({x0 - half, midy + half - radius}, radius, Quadrant_III, thickness, c1);
			GfxDrawSemiSphere({x1 + half - radius, midy - half}, radius, Quadrant_I, thickness, c2);
		}
		else {
			GfxDrawVerticalGradQuad({x0 - half, midy - half + radius, x0 + half, y1}, color0, c1);
			GfxDrawHorizontalGradQuad({x0 - half + radius, midy - half, x1 + half - radius, midy + half}, c1, c2);
			GfxDrawVerticalGradQuad({x1 - half, y0, x1 + half, midy + half - radius}, c2, color1);

			GfxDrawSemiSphere({x0 - half, midy - half}, radius, Quadrant_II, thickness, c1);
			GfxDrawSemiSphere({x1 + half - radius, midy + half - radius}, radius, Quadrant_IV, thickness, c2);
		}
	}
	else {
		float32 midx = x0 + 0.5f*dx;
		float32 l1 = 0.5f*dx + half - radius;
		float32 t1 = l1/length;
		float32 l2 = dy + thickness - 2*radius;
		float32 t2 = (l1 + l2)/length;

		Color c1 = t1*color0 + (1 - t1)*color1;
		Color c2 = t2*color0 + (1 - t2)*color1;

		if (pos0.y < pos1.y) {
			GfxDrawHorizontalGradQuad({x0, y0 - half, midx + half - radius, y0 + half}, color0, c1);
			GfxDrawVerticalGradQuad({midx - half, y0 - half + radius, midx + half, y1 + half - radius}, c1, c2);
			GfxDrawHorizontalGradQuad({midx - half + radius, y1 - half, x1, y1 + half}, c2, color1);

			GfxDrawSemiSphere({midx + half - radius, y0 - half}, radius, Quadrant_I, thickness, c1);
			GfxDrawSemiSphere({midx - half, y1 + half - radius}, radius, Quadrant_III, thickness, c2);
		}
		else {
			GfxDrawHorizontalGradQuad({x0, y1 - half, midx + half - radius, y1 + half}, color0, c1);
			GfxDrawVerticalGradQuad({midx - half, y0 - half + radius, midx + half, y1 + half - radius}, c2, c1);
			GfxDrawHorizontalGradQuad({midx - half + radius, y0 - half, x1, y0 + half}, c2, color1);

			GfxDrawSemiSphere({midx + half - radius, y1 + half - radius,}, radius, Quadrant_IV, thickness, c1);
			GfxDrawSemiSphere({midx - half, y0 - half}, radius, Quadrant_II, thickness, c2);
		}
	}
}