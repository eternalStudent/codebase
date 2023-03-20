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

void RenderText(float32 initialX, Point2* pos, BakedFont* font, Color color, String string) {
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		if (font->firstChar <= b && b <= font->lastChar) {
			pos->x += RenderGlyph(*pos, font, color, b);
		}
		else {
			if (b == 9) {
				pos->x += font->chardata[0].xadvance * 4;
			}
			if (b == 10) {
				pos->y += font->height;
				pos->x = initialX;
			}
		}
	}	
}

void RenderText(Point2 pos, BakedFont* font, Color color, String string) {
	RenderText(pos.x, &pos, font, color, string);
}

void RenderText(Point2 pos, BakedFont* font, Color color, StringList list) {
	float32 initialX = pos.x;
	LINKEDLIST_FOREACH(&list, StringNode, node) RenderText(initialX, &pos, font, color, node->string);
}

float32 GetCharWidth(BakedFont* font, byte b) {
	if (b == 9) return font->chardata[0].xadvance * 4;
	if (font->firstChar <= b && b <= font->lastChar) {
		BakedChar* bakedchar = font->chardata + (b - font->firstChar);
		return bakedchar->xadvance;
	}
	return 0.0f;
}

ssize GetCharIndex(BakedFont* font, StringList list, float32 x_end, float32 y_end) {
	float32 x = 0;
	float32 y = 0;

	ssize i = 0;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize j = 0; j < string.length; j++) {
			byte b = string.data[j];
			if (b == 10) {
				y += font->height;
				x = 0;
			}
			x += GetCharWidth(font, b);
			if (x_end <= x - 2 && y_end <= y) return i;
			i++;
		}
	}

	return list.totalLength;
}

struct TextMetrics {
	float32 maxx;
	float32 endx;
	float32 endy;
};

TextMetrics GetTextMetrics(BakedFont* font, StringList list) {
	if (list.totalLength == 0) return {};
	
	float32 maxx = 0;
	float32 x = 0;
	float32 y = font->height * 1.5f;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			if (b == 10) {
				maxx = MAX(maxx, x);
				x = 0;
				y += font->height;
			}
			x += GetCharWidth(font, b);
		}
	}

	maxx = MAX(x, maxx);
	return {maxx, x, y};
}

TextMetrics GetTextMetrics(BakedFont* font, StringList list, ssize stop) {
	if (list.totalLength == 0) return {};
	
	float32 maxx = 0;
	float32 x = 0;
	float32 y = font->height * 1.5f;
	ssize i = 0;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize j = 0; j < string.length; j++) {
			if (i == stop) break;
			byte b = string.data[j];
			if (b == 10) {
				maxx = MAX(maxx, x);
				x = 0;
				y += font->height;
			}
			x += GetCharWidth(font, b);
			i++;
		}
		if (i == stop) break;
	}

	maxx = MAX(x, maxx);
	return {maxx, x, y};
}