struct BakedChar {
	union {
		Box2i box;
		struct {Point2i p0, p1;};
		struct {int32 x0, y0, x1, y1;};
	};
	float32 xoff, yoff, xadvance;
};

struct Font {
	BakedChar chardata[96];
	TextureId texture;
	float32 height;
};

#include "truetype.cpp"

Font LoadFont(Arena* arena, const char* filePath, float32 height){
	byte* data = OSReadAll(filePath, arena).data;
	return TTLoadFont(arena, data, height);
}

Font LoadDefaultFont(Arena* arena, float32 height) {
	char buffer[260] = {}; 
	return LoadFont(arena, OSGetDefaultFontPath(buffer), height);
}

// NOTE: adopted from stbtt_GetBakedQuad
void RenderText(Font* font, StringList list, float32 x, float32 y, uint32 color){
	byte first_char = 32;
	byte last_char = 127;
	float32 ipw = 1.0f / 512.0f;
	float32 iph = 1.0f / 512.0f;

	float32 original_x = x;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			if (b == 9) {
				x += font->chardata[0].xadvance * 4;
			}
			if (b == 10) {
				y -= font->height;
				x = original_x;
			}
			if (first_char <= b && b <= last_char) {
				BakedChar* bakedchar = font->chardata + (b - first_char);
	
				float32 round_y = floor(y + bakedchar->yoff + 0.5f);
				float32 y1 = round_y + bakedchar->y1 - bakedchar->y0;
				float32 printy = 2*y - y1;
	
				float32 round_x = floor(x + bakedchar->xoff + 0.5f);
	
				float32 width = (float32)(bakedchar->x1 - bakedchar->x0);
				float32 height = (float32)(bakedchar->y1 - bakedchar->y0);
				Box2 pos = {round_x, printy, round_x + width, printy + height};
	
				Box2 crop = {
					bakedchar->x0 * ipw,
					bakedchar->y1 * iph,
					bakedchar->x1 * ipw,
					bakedchar->y0 * iph};
	
				GfxDrawText(font->texture, crop, pos, 0, color);
	
				x += bakedchar->xadvance;
			}
		}
	}
}

float32 GetCharWidth(Font* font, byte b) {
	byte first_char = 32;
	byte last_char = 127;

	if (b == 9) return font->chardata[0].xadvance * 4;
	if (first_char <= b && b <= last_char) {
		BakedChar* bakedchar = font->chardata + (b - first_char);
		return bakedchar->xadvance;
	}
	return 0.0f;
}

ssize GetCharIndex(Font* font, StringList list, float32 x_end, float32 y_end) {
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

TextMetrics GetTextMetrics(Font* font, StringList list) {
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

TextMetrics GetTextMetrics(Font* font, StringList list, ssize stop) {
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