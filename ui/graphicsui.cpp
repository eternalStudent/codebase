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
# define GfxFlush                   OpenGLUIFlush
# define GfxDrawSolidColorQuad		OpenGLUIDrawSolidColorQuad
# define GfxDrawOutlineQuad			OpenGLUIDrawOutlineQuad
# define GfxDrawVerticalGradQuad	OpenGLUIDrawVerticalGradQuad
# define GfxDrawHorizontalGradQuad	OpenGLUIDrawHorizontalGradQuad
# define GfxDrawSphere				OpenGLUIDrawSphere
# define GfxDrawGlyph				OpenGLUIDrawGlyph
# define GfxDrawWavyLine			OpenGLUIDrawWavyLine
# define GfxDrawStraightLine		OpenGLUIDrawStraightLine
# define GfxDrawImage				OpenGLUIDrawImage
# define GfxDrawCurve				OpenGLUIDrawCurve
# define GfxDrawShadow				OpenGLUIDrawShadow
# define GfxDrawSemiSphere			OpenGLUIDrawSemiSphere
# define GfxDrawHueGrad				OpenGLUIDrawHueGrad
# define GfxDrawSLQuad				OpenGLUIDrawSLQuad
# define GfxCropScreen				OpenGLUICropScreen
# define GfxClearCrop				OpenGLUIClearCrop
# define GfxGetCurrentCrop			OpenGLUIGetCurrentCrop
# define GfxSetActiveVBuffer		OpenGLUISetActiveVBuffer

#elif defined(GFX_BACKEND_D3D11)
# include "d3d11ui.cpp"
# define GfxInit					D3D11UIInit
# define GfxBeginDrawing			D3D11UIBeginDrawing
# define GfxEndDrawing				D3D11UIEndDrawing
# define GfxSetBackgroundColor		D3D11UISetBackgroundColor
# define GfxSetFontAtlas			D3D11UISetFontAtlas
# define GfxFlush                   D3D11UIFlush
# define GfxDrawSolidColorQuad		D3D11UIDrawSolidColorQuad
# define GfxDrawOutlineQuad			D3D11UIDrawOutlineQuad
# define GfxDrawVerticalGradQuad	D3D11UIDrawVerticalGradQuad
# define GfxDrawHorizontalGradQuad	D3D11UIDrawHorizontalGradQuad
# define GfxDrawSphere				D3D11UIDrawSphere
# define GfxDrawGlyph				D3D11UIDrawGlyph
# define GfxDrawWavyLine			D3D11UIDrawWavyLine
# define GfxDrawStraightLine		D3D11UIDrawStraightLine
# define GfxDrawImage				D3D11UIDrawImage
# define GfxDrawCurve				D3D11UIDrawCurve
# define GfxDrawShadow				D3D11UIDrawShadow
# define GfxDrawSemiSphere			D3D11UIDrawSemiSphere
# define GfxDrawHueGrad				D3D11UIDrawHueGrad
# define GfxDrawSLQuad				D3D11UIDrawSLQuad
# define GfxCropScreen				D3D11UICropScreen
# define GfxClearCrop				D3D11UIClearCrop
# define GfxGetCurrentCrop			D3D11UIGetCurrentCrop
# define GfxSetActiveVBuffer		D3D11UISetActiveVBuffer
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


Point2 GfxDrawStraightLine(Point2 pos, Direction dir, float32 length, float32 thickness, Color color0, Color color1) {
	float32 half = 0.5f*thickness;
	switch (dir) {
	case Dir_Left: {
		GfxDrawHorizontalGradQuad({pos.x - length, pos.y - half}, {length, thickness}, color1, color0);
		pos.x -= length;
	} break;
	case Dir_Up: {
		GfxDrawVerticalGradQuad({pos.x - half, pos.y - length}, {thickness, length}, color1, color0);
		pos.y -= length;
	} break;
	case Dir_Right: {
		GfxDrawHorizontalGradQuad({pos.x, pos.y - half}, {length, thickness}, color0, color1);
		pos.x += length;
	} break;
	case Dir_Down: {
		GfxDrawVerticalGradQuad({pos.x - half, pos.y}, {thickness, length}, color0, color1);
		pos.y += length;
	} break;
	}
	return pos;
}

void GfxDrawPath(Point2 start, Walk* walks, int32 count, float32 maxRadius, float32 thickness, Color color0, Color color1) {
	float32 half = 0.5f*thickness;
	Point2 pos = start;

	if (count == 1) {
		GfxDrawStraightLine(pos, walks->dir, walks->length, thickness, color0, color1);
		return;
	}

	float32 totalLength = 0;
	for (int32 i = 0; i < count; i++) {
		Walk* walk = walks + i;

		if (i == 0 || i == count - 1) {
			float32 length = walk->length + half - maxRadius;
			totalLength += MAX(0, length);
		}
		else {
			float32 length = walk->length + thickness - 2*maxRadius;
			totalLength += MAX(0, length);
		}
	}
	float32 t = 0;

	float32 radius;
	{
		float32 minLength = MIN(walks->length, 0.5f*walks[1].length);
		radius = minLength + half >= maxRadius
			? maxRadius
			: minLength + half;

		float32 length = walks->length + half - radius;
		t += length/totalLength;
		Color c1 = (1 - t)*color0 + t*color1;
		pos = GfxDrawStraightLine(pos, walks->dir, length, thickness, color0, c1);
	}
		
	Direction prev = walks->dir;

	for (int32 i = 1; i < count; i++) {
		Walk* walk = walks + i;

		float32 nextRadius, length;
		if (i == count - 1) {
			length = walk->length + half - radius;
			nextRadius = 0;
		}
		else {
			float32 minLength = MIN(0.5f*walk->length, 0.5f*walks[i + 1].length);
			nextRadius = minLength + half >= maxRadius
				? maxRadius
				: minLength + half;

			length = walk->length + thickness - nextRadius - radius;
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

		pos = GfxDrawStraightLine(pos, walk->dir, length, thickness, c0, c1);

		prev = walk->dir;
		radius = nextRadius;
	}
}

void GfxDrawPath(Point2 start, Point2 end, float32 mid, bool vertFirst, float32 maxRadius, float32 thickness, Color color0, Color color1) {
	if (vertFirst) {
		if (start.x == end.x) {
			GfxDrawStraightLine(start, start.y < end.y ? Dir_Down : Dir_Up, ABS(start.y - end.y), thickness, color0, color1);
		}
		else {
			Walk walk[] = {
				{mid < start.y ? Dir_Up : Dir_Down, ABS(mid - start.y)},
				{start.x < end.x ? Dir_Right : Dir_Left, ABS(start.x - end.x)},
				{mid < end.y ? Dir_Down : Dir_Up, ABS(mid - end.y)}
			};
			GfxDrawPath(start, walk, 3, maxRadius, thickness, color0, color1);
		}
	}
	else {
		if (start.y == end.y) {
			GfxDrawStraightLine(start, start.x < end.x ? Dir_Left : Dir_Right, ABS(start.x - end.x), thickness, color0, color1);
		}
		else {
			Walk walk[] = {
				{mid < start.x ? Dir_Left : Dir_Right, ABS(mid - start.x)},
				{start.y < end.y ? Dir_Down : Dir_Up, ABS(start.y - end.y)},
				{mid < end.x ? Dir_Left : Dir_Right, ABS(mid - end.x)}
			};
			GfxDrawPath(start, walk, 3, maxRadius, thickness, color0, color1);
		}
	}
}

void GfxDrawPath(Point2 start, Point2 end, bool vertFirst, float32 maxRadius, float32 thickness, Color color0, Color color1) {
	float32 mid = vertFirst 
		? round((start.y + end.y)/2)
		: round((start.x + end.x)/2);

	GfxDrawPath(start, end, mid, vertFirst, maxRadius, thickness, color0, color1);
}

