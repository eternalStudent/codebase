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


enum Direction {
	Dir_Left,
	Dir_Up,
	Dir_Right,
	Dir_Down
};

struct Walk {
	Direction dir;
	float32 length;
};


void DrawStraightLine(Point2* pos, Direction dir, float32 length, float32 half, Color color0, Color color1) {
	switch (dir) {
	case Dir_Left: {
		GfxDrawHorizontalGradQuad({pos->x - length, pos->y - half}, {length, 2*half}, color1, color0);
		pos->x -= length;
	} break;
	case Dir_Up: {
		GfxDrawVerticalGradQuad({pos->x - half, pos->y - length}, {2*half, length}, color1, color0);
		pos->y -= length;
	} break;
	case Dir_Right: {
		GfxDrawHorizontalGradQuad({pos->x, pos->y - half}, {length, 2*half}, color0, color1);
		pos->x += length;
	} break;
	case Dir_Down: {
		GfxDrawVerticalGradQuad({pos->x - half, pos->y}, {2*half, length}, color0, color1);
		pos->y += length;
	} break;
	}
}

// TODO: Add gradient
void GfxDrawPath(Point2 start, Walk* walks, int32 count, float32 maxRadius, float32 thickness, Color color0, Color color1) {
	float32 half = 0.5f*thickness;
	Point2 pos = start;

	if (count == 1) {
		DrawStraightLine(&pos, walks->dir, walks->length, half, color0, color1);
		return;
	}

	float32 totalLength = 0;
	for (int32 i = 0; i < count; i++) {
		if (i == 0 || i == count - 1) {
			totalLength += MAX(0, walks->length + half - maxRadius);
		}
		else {
			totalLength += MAX(0, walks->length + thickness - 2*maxRadius);
		}
	}
	float32 t = 0;

	{
		float32 radius = walks->length + half >= maxRadius
			? maxRadius
			: walks->length + half;

		float32 length = walks->length + half - radius;
		t += length/totalLength;
		Color c1 = (1 - t)*color0 + t*color1;
		DrawStraightLine(&pos, walks->dir, length, half, color0, c1);
	}
		
	Direction prev = walks->dir;

	for (int32 i = 1; i < count; i++) {
		Walk* walk = walks + i;

		float32 radius, length;
		if (i == count - 1) {
			radius = walk->length + half >= maxRadius
				? maxRadius
				: walk->length + half;

			length = walk->length + half - radius;
		}
		else {
			radius = walk->length + thickness >= 2*maxRadius
				? maxRadius
				: 0.5f*walk->length + half;

			length = walk->length + thickness - 2*radius;
		}

		Color c0 = (1 - t)*color0 + t*color1;
		t += length/totalLength;
		Color c1 = (1 - t)*color0 + t*color1;

		Quadrant quadrant = {};
		Point2 offset = {};
		Point2 advancement = {};

		switch (prev) {
		case Dir_Left: {
			advancement.x = -radius + half;
			offset.x = -radius;
			if (walk->dir == Dir_Up) {
				quadrant = Quadrant_III;
				advancement.y = -radius + half;
				offset.y = -radius + half;
			}
			else {
				quadrant = Quadrant_II;
				advancement.y = radius - half;
				offset.y = -half;
			}
		} break; 
		case Dir_Up: {
			advancement.y = -radius + half;
			offset.y = -radius;

			if (walk->dir == Dir_Left) {
				quadrant = Quadrant_I;
				advancement.x = -radius + half;
				offset.x = -radius + half;
			}
			else {
				quadrant = Quadrant_II;
				advancement.x = radius - half;
				offset.x = -half;
			}
		} break; 
		case Dir_Right: {
			advancement.x = radius - half;
			offset.x = 0;
			if (walk->dir == Dir_Up) {
				quadrant = Quadrant_IV;
				advancement.y = -radius + half;
				offset.y = -radius + half;
			}
			else {
				quadrant = Quadrant_I;
				advancement.y = radius - half;
				offset.y = -half;
			}
		} break; 
		case Dir_Down: {
			advancement.y = radius - half;
			offset.y = 0;
			if (walk->dir == Dir_Left) {
				quadrant = Quadrant_IV;
				advancement.x = -radius + half;
				offset.x = -radius + half;
			}
			else {
				quadrant = Quadrant_III;
				advancement.x = radius - half;
				offset.x = -half;
			}
		} break; 
		}

		GfxDrawSemiSphere(pos + offset, radius, quadrant, thickness, c0);
		pos = pos + advancement;

		DrawStraightLine(&pos, walk->dir, length, half, c0, c1);

		prev = walk->dir;
	}
}


void GfxDrawPath(Point2 start, Point2 end, bool vertFirst, float32 maxRadius, float32 thickness, Color color0, Color color1) {
	if (vertFirst) {
		float32 midy = round((start.y + end.y)/2);
		Walk walk[] = {
			{midy < start.x ? Dir_Down : Dir_Up, ABS(midy - start.y)},
			{start.x < end.x ? Dir_Left : Dir_Left, ABS(start.x - end.x)},
			{midy < end.y ? Dir_Down : Dir_Up, ABS(midy - end.y)}
		};
		GfxDrawPath(start, walk, 3, maxRadius, thickness, color0, color1);
	}
	else {
		float32 midx = round((start.y + end.y)/2);
		Walk walk[] = {
			{midx < start.x ? Dir_Left : Dir_Right, ABS(midx - start.x)},
			{start.y < end.y ? Dir_Down : Dir_Up, ABS(start.y - end.y)},
			{midx < end.x ? Dir_Left : Dir_Right, ABS(midx - end.x)}
		};
		GfxDrawPath(start, walk, 3, maxRadius, thickness, color0, color1);
	}
}