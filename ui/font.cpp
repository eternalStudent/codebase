/*
 * TODO:
 * This was written as a wrapper to stb_truetype,
 * and needs to be re-written in order to support 
 * a more robust text rendering pipeline.
 */

struct BakedChar {
	int16 x0, y0, x1, y1;
	float32 xoff, yoff, xadvance;
};

struct BakedFont {
	float32 height;
	float32 lineGap;
	int32 firstChar;
	int32 lastChar;
	BakedChar chardata[96];
};

// TODO: rename pw, ph to width and height, and rename bitmap to data
// TODO: change byte* to byte[0]?
struct AtlasBitmap {
	int16 pw, ph, x, y, bottom_y;
	byte* bitmap;
};

AtlasBitmap CreateAtlasBitmap(int16 width, int16 height, Arena* arena) {
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

#if defined(_OS_WINDOWS)
#  include "dwrite.cpp"
#endif

// TODO: add #if here
#include "truetype.cpp"


Image GetImageFromFontAtlas(AtlasBitmap* atlas) {
	return Image{atlas->pw, atlas->ph, 1, atlas->bitmap};
}

float32 RenderGlyph(Point2 pos, BakedFont* font, Color color, byte b) {
	BakedChar bakedchar = font->chardata[b - font->firstChar];
	
	float32 round_y = round(pos.y + (bakedchar.yoff + font->height));
	float32 round_x = round(pos.x + bakedchar.xoff);
	
	float32 width = (float32)(bakedchar.x1 - bakedchar.x0);
	float32 height = (float32)(bakedchar.y1 - bakedchar.y0);
	pos = {round_x, round_y};
	
	Box2 crop = {
		(float32)bakedchar.x0,
		(float32)bakedchar.y0,
		(float32)bakedchar.x1,
		(float32)bakedchar.y1
	};
	
	GfxDrawGlyph(pos, {width, height}, crop, color);
	return bakedchar.xadvance;
}

float32 GetCharWidth(BakedFont* font, byte b) {
	if (font->firstChar <= b && b <= font->lastChar) {
		BakedChar bakedchar = font->chardata[b - font->firstChar];
		return bakedchar.xadvance;
	}
	if (b == 10) return font->chardata[32 - font->firstChar].xadvance;
	if (b == 9) return 4*font->chardata[32 - font->firstChar].xadvance;
	return 0;
}

// String
//------------

float32 GetWordWidth(BakedFont* font, String string, ssize i) {
	float32 width = 0;
	for (; i < string.length; i++) {
		byte b = string.data[i];
		if (IsWhiteSpace(b)) return width;
		width += GetCharWidth(font, b);
	}
	return width;
}

Point2 RenderText(Point2 pos, BakedFont* font, Color color, String string,
				bool shouldWrap, float32 wrapX) {

	bool prevCharWasWhiteSpace = false;
	float32 initialX = pos.x;
	float32 height = font->height;
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= pos.x + wordWidth) {
				pos.y += height + font->lineGap;
				pos.x = initialX;
			}
		}
		if (b == 9) {
			pos.x += 4*font->chardata[32 - font->firstChar].xadvance;
		}
		if (b == 10) {
			pos.y += height + font->lineGap;
			pos.x = initialX;
		}
		if (font->firstChar <= b && b <= font->lastChar) {
			pos.x += RenderGlyph(pos, font, color, b);
		}
		prevCharWasWhiteSpace = whiteSpace;
	}
	return pos;
}

ssize GetCharIndex(BakedFont* font, String string, 
				   float32 cursorx, float32 cursory,
				   bool shouldWrap, float32 wrapX) {

	if (cursorx <= 2 && cursory <= 0) return 0;

	float32 x = 0;
	float32 y = 0;

	bool prevCharWasWhiteSpace = false;
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {
				if (cursory <= y) return i;
				y += font->height + font->lineGap;
				x = 0;
				if (cursorx <= x && cursory <= y) return i + 1;
			}
		}
		x += GetCharWidth(font, b);

		if (b == 10) {
			if (cursory <= y) return i + 1;
			y += font->height + font->lineGap;
			x = 0;
		}
		if (cursorx <= x && cursory <= y) return i + 1;
		
		prevCharWasWhiteSpace = whiteSpace;
	}

	return string.length;
}

struct TextMetrics {
	union {
		struct {float32 x, y;};
		Point2 pos;
	};

	float32 maxx;
	bool lastCharIsWhiteSpace;
	bool lastCharIsNewLine;
};

bool IsStartOfLine(TextMetrics metrics, BakedFont* font, String string,
				ssize i, bool shouldWrap, float32 wrapX) {

	if (metrics.lastCharIsNewLine)
		return true;
	if (!shouldWrap || !metrics.lastCharIsWhiteSpace) 
		return false;
	byte b = string.data[i];
	if (IsWhiteSpace(b)) 
		return false;
	float32 wordWidth = GetWordWidth(font, string, i);
	return wrapX <= metrics.x + wordWidth;
}

TextMetrics NextTextMetrics(TextMetrics metrics, BakedFont* font, String string,
							ssize i, bool shouldWrap, float32 wrapX) {

	if (metrics.lastCharIsNewLine) {
		metrics.x = 0;
		metrics.y += font->height + font->lineGap;
	}
	byte b = string.data[i];
	bool whiteSpace = IsWhiteSpace(b);
	if (shouldWrap && metrics.lastCharIsWhiteSpace && !whiteSpace) {
		float32 wordWidth = GetWordWidth(font, string, i);
		if (wrapX <= metrics.x + wordWidth) {
			metrics.maxx = MAX(metrics.maxx, metrics.x);
			metrics.x = 0;
			metrics.y += font->height + font->lineGap;
		}
	}
	metrics.x += GetCharWidth(font, b);
	metrics.maxx = MAX(metrics.x, metrics.maxx);
	metrics.lastCharIsNewLine = b == 10;
	metrics.lastCharIsWhiteSpace = whiteSpace;
	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, String string, ssize end, 
						   bool shouldWrap, float32 wrapX) {
	
	TextMetrics metrics = {};
	if (string.length == 0) return metrics;

	metrics.y = font->height;
	for (ssize i = 0; i < end; i++)
		metrics = NextTextMetrics(metrics, font, string, i, shouldWrap, wrapX);

	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, String string, 
						   bool shouldWrap, float32 wrapX) {

	return GetTextMetrics(font, string, string.length, shouldWrap, wrapX);
}

void RenderTextSelection(Point2 pos, BakedFont* font, Color color, String string,
						 ssize start, ssize end, 
						 bool shouldWrap, float32 wrapX) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	ssize i = 0;

	// start by finding position of start.
	for (; i < start + 1; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {
				x = 0;
				y += font->height + font->lineGap;
			}
		}
		if (i == start) break;
		x += GetCharWidth(font, b);
		if (b == 10) {
			x = 0;
			y += font->height + font->lineGap;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}

	// iterate till end of line
	Point2 selection;
	float32 startx = x;
	for (; i < end; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {

				selection.x = round(pos.x + startx);
				selection.y = round(pos.y + y + 2);
				GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
				startx = 0;

				x = 0;
				y += font->height + font->lineGap;
				i++;
				break;
			}
		}
		x += GetCharWidth(font, b);
		if (b == 10) {

			selection.x = round(pos.x + startx);
			selection.y = round(pos.y + y + 2);
			GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
			startx = 0;
			
			x = 0;
			y += font->height + font->lineGap;
			i++;
			break;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}
	
	// now keep iterating until end
	for (; i < end; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (wrapX <= x + wordWidth) {

				selection.x = round(pos.x);
				selection.y = round(pos.y + y + 2);
				GfxDrawQuad(selection, {x, font->height + 2}, color, 0, 0, {}, color);

				x = 0;
				y += font->height + font->lineGap;
			}
		}
		x += GetCharWidth(font, b);
		if (b == 10) {

			selection.x = round(pos.x);
			selection.y = round(pos.y + y + 2);
			GfxDrawQuad(selection, {x, font->height + 2}, color, 0, 0, {}, color);

			x = 0;
			y += font->height + font->lineGap;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}

	selection.x = round(pos.x + startx);
	selection.y = round(pos.y + y + 2);
	GfxDrawQuad(selection, {x - startx, font->height + 2}, color, 0, 0, {}, color);
}

// StringList
//------------

float32 GetWordWidth(BakedFont* font, StringListPos pos) {
	float32 width = 0;
	for (StringNode* node = pos.node; node != NULL; node = node->next) {
		String string = node->string;
		ssize start = node == pos.node ? pos.index : 0;
		for (ssize i = start; i < string.length; i++) {
			byte b = string.data[i];
			if (IsWhiteSpace(b)) return width;
			width += GetCharWidth(font, b);
		}
	}
	return width;
}

Point2 RenderText(Point2 pos, BakedFont* font, Color color, StringList list,
				bool shouldWrap, float32 wrapX) {

	bool prevCharWasWhiteSpace = false;
	float32 initialX = pos.x;
	float32 height = font->height;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = GetWordWidth(font, {node, i});
				if (wrapX <= pos.x + wordWidth) {
					pos.y += height + font->lineGap;
					pos.x = initialX;
				}
			}
			if (b == 9) {
				pos.x += 4*font->chardata[32 - font->firstChar].xadvance;
			}
			if (b == 10) {
				pos.y += font->height + font->lineGap;
				pos.x = initialX;
			}
			if (font->firstChar <= b && b <= font->lastChar) {
				pos.x += RenderGlyph(pos, font, color, b);
			}
			prevCharWasWhiteSpace = whiteSpace;
		}
	}
	return pos;
}

StringListPos GetCharPos(BakedFont* font, StringList list, 
						 float32 cursorx, float32 cursory,
						 bool shouldWrap, float32 wrapX) {

	if (cursorx <= 2 && cursory <= 0) return {list.first, 0};

	float32 x = 0;
	float32 y = 0;

	bool prevCharWasWhiteSpace = false;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = GetWordWidth(font, {node, i});
				if (wrapX <= x + wordWidth) {
					if (cursory <= y) return {node, i};
					y += font->height + font->lineGap;
					x = 0;
					if (cursorx <= x && cursory <= y) return StringListPosInc({node, i});
				}
			}
			x += GetCharWidth(font, b);

			if (b == 10) {
				if (cursory <= y) return StringListPosInc({node, i});
				y += font->height + font->lineGap;
				x = 0;
			}
			if (cursorx <= x && cursory <= y) return StringListPosInc({node, i});
			
			prevCharWasWhiteSpace = whiteSpace;
		}
	}

	return SL_END(list);
}

bool IsStartOfLine(TextMetrics metrics, BakedFont* font,
				   StringListPos start, bool shouldWrap, float32 wrapX) {

	if (metrics.lastCharIsNewLine)
		return true;
	if (!shouldWrap || !metrics.lastCharIsWhiteSpace) 
		return false;
	byte b = start.node->string.data[start.index];
	if (IsWhiteSpace(b)) 
		return false;
	float32 wordWidth = GetWordWidth(font, start);
	return wrapX <= metrics.x + wordWidth;
}

TextMetrics NextTextMetrics(TextMetrics metrics, BakedFont* font, StringListPos pos,
							bool shouldWrap, float32 wrapX) {

	if (metrics.lastCharIsNewLine) {
		metrics.x = 0;
		metrics.y += font->height + font->lineGap;
	}
	byte b = pos.node->string.data[pos.index];
	bool whiteSpace = IsWhiteSpace(b);
	if (shouldWrap && metrics.lastCharIsWhiteSpace && !whiteSpace) {
		float32 wordWidth = GetWordWidth(font, pos);
		if (wrapX <= metrics.x + wordWidth) {
			metrics.maxx = MAX(metrics.maxx, metrics.x);
			metrics.x = 0;
			metrics.y += font->height + font->lineGap;
		}
	}
	metrics.x += GetCharWidth(font, b);
	metrics.maxx = MAX(metrics.x, metrics.maxx);
	metrics.lastCharIsNewLine = b == 10;
	metrics.lastCharIsWhiteSpace = whiteSpace;
	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, StringList list, StringListPos end, 
						   bool shouldWrap, float32 wrapX) {
	
	TextMetrics metrics = {};
	metrics.y = font->height;

	if (list.length == 0) return metrics;

	LINKEDLIST_FOREACH(&list, StringNode, node) {
		ssize endIndex = node == end.node ? end.index : node->string.length;
		for (ssize i = 0; i < endIndex; i++) {
			metrics = NextTextMetrics(metrics, font, {node, i}, shouldWrap, wrapX);
		}
		if (node == end.node) break;
	}

	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, StringList list, 
						   bool shouldWrap, float32 wrapX) {

	if (list.length == 0) return {};
	return GetTextMetrics(font, list, {list.last, list.last->string.length}, shouldWrap, wrapX);
}

void RenderTextSelection(Point2 pos, BakedFont* font, Color color, StringList list,
						 StringListPos tail, StringListPos head, 
						 bool shouldWrap, float32 wrapX,
						 Color caretColor) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	int32 state = 0;
	float32 startx = x;
	Point2 selection;
	float32 height = font->height;

	StringListPos start, end;
	bool onEnd = StringListPosCompare(tail, head);
	if (onEnd) {
		start = tail;
		end = head;
	}
	else {
		start = head;
		end = tail;
	}

	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		ssize endIndex = node == end.node ? end.index : string.length;
		for (ssize i = 0; i < endIndex; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);

			if (state == 0) {
				// start by finding position of start.
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = GetWordWidth(font, {node, i});
					if (wrapX <= x + wordWidth) {
						x = 0;
						y += height + font->lineGap;
					}
				}
				if (node == start.node && i == start.index) {
					state++;
					startx = x;

					if (!onEnd) {
						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y + 2);
						Dimensions2 dim = {1, height + 2};
						GfxDrawQuad(selection, dim, caretColor, 0, 0, {}, caretColor);
					}

					i--;
					continue;
				}

				x += GetCharWidth(font, b);
				if (b == 10) {
					x = 0;
					y += height + font->lineGap;
				}
			}

			if (state == 1) {
				// iterate till end of line
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = GetWordWidth(font, {node, i});
					if (wrapX <= x + wordWidth) {

						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y + 2);
						Dimensions2 dim = {(x - startx), height + 2};
						GfxDrawQuad(selection, dim, color, 0, 0, {}, color);
						startx = 0;

						x = 0;
						y += height + font->lineGap;
						state++;
						i--;
						continue;
					}
				}
				x += GetCharWidth(font, b);
				if (b == 10) {

					selection.x = round(pos.x + startx);
					selection.y = round(pos.y + y + 2);
					Dimensions2 dim = {(x - startx), height + 2};
					GfxDrawQuad(selection, dim, color, 0, 0, {}, color);
					startx = 0;
					
					x = 0;
					y += height + font->lineGap;
					state++;
					continue;
				}
			}

			if (state == 2) {
				// now keep iterating until end
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = GetWordWidth(font, {node, i});
					if (wrapX <= x + wordWidth) {

						selection.x = round(pos.x);
						selection.y = round(pos.y + y + 2);
						Dimensions2 dim = {x, height + 2};
						GfxDrawQuad(selection, dim, color, 0, 0, {}, color);

						x = 0;
						y += height + font->lineGap;
					}
				}
				x += GetCharWidth(font, b);
				if (b == 10) {

					selection.x = round(pos.x);
					selection.y = round(pos.y + y + 2);
					Dimensions2 dim = {x, height + 2};
					GfxDrawQuad(selection, dim, color, 0, 0, {}, color);

					x = 0;
					y += height + font->lineGap;
				}
			}

			prevCharWasWhiteSpace = whiteSpace;
		}
		if (end.node == node) break;
	}

	selection.y = round(pos.y + y + 2);
	bool eq = StringListPosEquals(tail, head);
	if (!eq) {
		selection.x = round(pos.x + startx);
		GfxDrawQuad(selection, {(x - startx), height + 2}, color, 0, 0, {}, color);
	}

	if (onEnd || eq) {
		selection.x = round(pos.x + x);
		GfxDrawQuad(selection, {1, height + 2}, caretColor, 0, 0, {}, caretColor);
	}
}
