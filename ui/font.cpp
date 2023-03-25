struct BakedChar {
	int16 x0, y0, x1, y1;
	float32 xoff, yoff, xadvance;
};

struct BakedFont {
	BakedChar chardata[96];
	float32 height;
	int32 firstChar;
	int32 lastChar;
};

// TODO: change byte* to byte[0]?
struct AtlasBitmap {
	int16 pw, ph, x, y, bottom_y;
	byte* bitmap;
};

AtlasBitmap CreateAtlasBitmap(Arena* arena, int16 width, int16 height) {
	AtlasBitmap atlas;
	atlas.pw = width;
	atlas.ph = height;
	atlas.x = 1;
	atlas.y = 1;
	atlas.bottom_y = 1;
	atlas.bitmap = (byte*)ArenaAlloc(arena, width*height);
	memset(atlas.bitmap, 0, width*height);
	return atlas;
}

#include "truetype.cpp"

FontInfo LoadDefaultFont(Arena* arena) {
	byte* data = OSReadAll(OSGetDefaultFontFile(), arena).data;
	return TTLoadFont(data);
}

BakedFont LoadAndBakeFont(Arena* arena, AtlasBitmap* atlas, byte* data, float32 height) {
	FontInfo fontInfo = TTLoadFont(data);

	return TTBakeFont(arena, fontInfo, atlas, height);
}

BakedFont LoadAndBakeDefaultFont(Arena* arena, AtlasBitmap* atlas, float32 height) {
	byte* data = OSReadAll(OSGetDefaultFontFile(), arena).data;
	FontInfo fontInfo = TTLoadFont(data);

	return TTBakeFont(arena, fontInfo, atlas, height);
}

TextureId GenerateFontAtlas(AtlasBitmap* atlas) {
	return GfxGenerateTexture(Image{atlas->pw, atlas->ph, 1, atlas->bitmap}, GFX_SMOOTH);
}

// NOTE: adopted from stbtt_GetBakedQuad
float32 RenderGlyph(Point2 pos, BakedFont* font, Color color, byte b) {
	float32 ipw = 1.0f/512.0f; // TODO: shouldn't be hard-coded, maybe move to graphics layer?
	float32 iph = 1.0f/512.0f;

	BakedChar bakedchar = font->chardata[b - font->firstChar];
	
	float32 round_y = floor(pos.y + bakedchar.yoff + font->height + 0.5f);
	float32 round_x = floor(pos.x + bakedchar.xoff + 0.5f);
	
	float32 width = (float32)(bakedchar.x1 - bakedchar.x0);
	float32 height = (float32)(bakedchar.y1 - bakedchar.y0);
	pos = {round_x, round_y};
	
	Box2 crop = {
		bakedchar.x0 * ipw,
		bakedchar.y0 * iph,
		bakedchar.x1 * ipw,
		bakedchar.y1 * iph
	};
	
	GfxDrawGlyph(pos, {width, height}, crop, color);
	return bakedchar.xadvance;
}

float32 GetCharWidth(BakedFont* font, byte b) {
	if (font->firstChar <= b && b <= font->lastChar) {
		BakedChar bakedchar = font->chardata[b - font->firstChar];
		return bakedchar.xadvance;
	}
	if (b == 9) return 4*font->chardata[32 - font->firstChar].xadvance;
	return 0;
}

float32 GetWordWidth(BakedFont* font, String string, ssize i) {
	float32 width = 0;
	for (; i < string.length; i++) {
		byte b = string.data[i];
		if (IsWhiteSpace(b)) return width;
		width += GetCharWidth(font, b);
	}
	return width;
}

void RenderText(Point2 pos, BakedFont* font, Color color, String string,
				float32 wrapX = -1) {

	bool prevCharWasWhiteSpace = false;
	float32 initialX = pos.x;
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= pos.x + wordWidth) {
				pos.y += font->height;
				pos.x = initialX;
			}
		}
		if (b == 9) {
			pos.x += 4*font->chardata[32 - font->firstChar].xadvance;
		}
		if (b == 10) {
			pos.y += font->height;
			pos.x = initialX;
		}
		if (font->firstChar <= b && b <= font->lastChar) {
			pos.x += RenderGlyph(pos, font, color, b);
		}
		prevCharWasWhiteSpace = whiteSpace;
	}
}

ssize GetCharIndex(BakedFont* font, String string, float32 x_end, float32 y_end,
				   float32 wrapX = -1) {

	float32 x = 0;
	float32 y = 0;

	bool prevCharWasWhiteSpace = false;
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {
				y += font->height;
				x = 0;
			}
		}
		if (b == 10) {
			y += font->height;
			x = 0;
		}
		x += GetCharWidth(font, b);
		if (x_end <= x - 2 && y_end <= y) return i;
		prevCharWasWhiteSpace = whiteSpace;
	}

	return string.length;
}

struct TextMetrics {
	float32 maxx;
	float32 endx;
	float32 endy;
};

TextMetrics GetTextMetrics(BakedFont* font, String string, 
						   ssize end, float32 wrapX = -1) {
	
	if (string.length == 0) return {};
	
	bool prevCharWasWhiteSpace = false;
	float32 maxx = 0;
	float32 x = 0;
	float32 y = font->height;
	for (ssize i = 0; i < end; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {
				maxx = MAX(maxx, x);
				x = 0;
				y += font->height;
			}
		}
		if (b == 10) {
			maxx = MAX(maxx, x);
			x = 0;
			y += font->height;
		}
		x += GetCharWidth(font, b);
		prevCharWasWhiteSpace = whiteSpace;
	}

	maxx = MAX(x, maxx);
	return {maxx, x, y};
}

TextMetrics GetTextMetrics(BakedFont* font, String string, 
						   float32 wrapX = -1) {

	return GetTextMetrics(font, string, string.length, wrapX);
}

void RenderTextSelection(Point2 pos, BakedFont* font, Color color, String string,
						 ssize start, ssize end, float32 wrapX = -1) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	ssize i = 0;

	// start by finding position of start.
	for (; i < start; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {
				x = 0;
				y += font->height;
			}
		}
		if (b == 10) {
			x = 0;
			y += font->height;
		}
		x += GetCharWidth(font, b);
		prevCharWasWhiteSpace = whiteSpace;
	}

	// iterate till end of line
	Point2 selection;
	float32 startx = x;
	for (; i < end; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {

				selection.x = round(pos.x + startx);
				selection.y = round(pos.y + y + 2);
				GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
				startx = 0;

				x = 0;
				y += font->height;
				break;
			}
		}
		if (b == 10) {

			selection.x = round(pos.x + x);
			selection.y = round(pos.y + y + 2);
			GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
			startx = 0;
			
			x = 0;
			y += font->height;
			break;
		}
		x += GetCharWidth(font, b);
		prevCharWasWhiteSpace = whiteSpace;
	}
	
	// now keep iterating until end
	for (; i < end; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (wrapX != -1 && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {

				selection.x = round(pos.x);
				selection.y = round(pos.y + y + 2);
				GfxDrawQuad(selection, {x, font->height + 2}, color, 0, 0, {}, color);

				x = 0;
				y += font->height;
			}
		}
		if (b == 10) {

			selection.x = round(pos.x);
			selection.y = round(pos.y + y + 2);
			GfxDrawQuad(selection, {x, font->height + 2}, color, 0, 0, {}, color);

			x = 0;
			y += font->height;
		}
		x += GetCharWidth(font, b);
		prevCharWasWhiteSpace = whiteSpace;
	}

	selection.x = round(pos.x + startx);
	selection.y = round(pos.y + y + 2);
	GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
}