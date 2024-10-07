/*
 * TODO:
 * This was written for pre-baked fonts,
 * and needs to be re-written in order to support 
 * a more robust text rendering pipeline.
 */

struct BakedGlyph {
	int16 x0, y0, x1, y1;
	float32 xoff, yoff, xadvance;
};

struct BakedFont {
	float32 height;
	float32 ascent;
	float32 descent;
	float32 lineGap;
	float32 xHeight;

	// TODO: I'm not really using this for custom ranges
	//       Let's rename/delete/replace
	int32 firstChar;
	int32 lastChar;

	BakedGlyph glyphs[96];
};

// TODO: now that I have two method of allocating a region
//       I should maybe rename this
struct AtlasBitmap {
	int32 width, height, x, y, bottom_y;
	byte* data;
};

AtlasBitmap CreateAtlasBitmap(int32 width, int32 height, byte* buffer) {
	AtlasBitmap atlas;
	atlas.width = width;
	atlas.height = height;
	atlas.x = 1;
	atlas.y = 1;
	atlas.bottom_y = 1;
	atlas.data = buffer;
	memset(atlas.data, 0, width*height);
	return atlas;
}

#include "../graphics/region.cpp"

#if _OS_WINDOWS
#  include "dwrite.cpp"
#endif

// TODO: add #if here
#include "truetype.cpp"

void FreeBakedFont(BakedFont* font, RegionNode* atlasRoot, FixedSize* nodeAllocator) {
	for (int32 i = 0; i < 96; i++) {
		BakedGlyph* glyph = font->glyphs + i;
		Point2i pos0 = {glyph->x0 - 1, glyph->y0 - 1};
		Point2i pos1 = {glyph->x1 - 1, glyph->y1 - 1};
		Dimensions2i dim = pos1 - pos0;
		if (dim.width & dim.height) {
			dim = dim + Point2i{1, 1};
			RegionNode* node = RegionGetByPosAndDim(atlasRoot, pos0, dim);
			ASSERT(node);
			RegionFree(node, nodeAllocator);
		}
	}
}

Image GetImageFromFontAtlas(AtlasBitmap* atlas) {
	return Image{atlas->width, atlas->height, 1, atlas->data};
}

Image GetImageFromFontAtlas(Dimensions2i dim, byte* data) {
	return Image{dim.width, dim.height, 1, data};
}

float32 RenderGlyph(Point2 pos, BakedFont* font, Color color, byte b) {
	BakedGlyph glyph = font->glyphs[b - font->firstChar];
	
	float32 round_y = round(pos.y + glyph.yoff + font->ascent);
	float32 round_x = round(pos.x + glyph.xoff);
	
	float32 width = (float32)(glyph.x1 - glyph.x0);
	float32 height = (float32)(glyph.y1 - glyph.y0);
	pos = {round_x, round_y};
	
	Box2 crop = {
		(float32)glyph.x0,
		(float32)glyph.y0,
		(float32)glyph.x1,
		(float32)glyph.y1
	};
	
	GfxDrawGlyph(pos, {width, height}, crop, color);
	return glyph.xadvance;
}

float32 GetCharWidth(BakedFont* font, byte b) {
	if (font->firstChar <= b && b <= font->lastChar) {
		BakedGlyph glyph = font->glyphs[b - font->firstChar];
		return glyph.xadvance;
	}
	if (b == 10) return font->glyphs[32 - font->firstChar].xadvance;
	if (b == 9) return 4*font->glyphs[32 - font->firstChar].xadvance;
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
			pos.x += 4*font->glyphs[32 - font->firstChar].xadvance;
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

ssize GetCharIndex(Point2 cursor,
				   BakedFont* font, String string, 
				   bool shouldWrap, float32 x1) {

	Point2 pos = {};

	bool prevCharWasWhiteSpace = false;
	float32 initialX = pos.x;
	float32 height = font->height;

	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = GetWordWidth(font, string, i);
			if (x1 <= pos.x + wordWidth) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return i;
				pos.y += height + font->lineGap;
				pos.x = initialX;
			}
		}
		if (b == 10) {
			if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
				return i;
			pos.y += font->height + font->lineGap;
			pos.x = initialX;
		}
		if (font->firstChar <= b && b <= font->lastChar) {
			float32 width = font->glyphs[b - font->firstChar].xadvance;
			if (InBounds(pos, {width, font->height}, cursor))
				return i;
			pos.x += width;
		}
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
				GfxDrawSolidColorQuad(selection, {x - startx, font->height + 2}, color);
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
			GfxDrawSolidColorQuad(selection, {x - startx, font->height + 2}, color);
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
				GfxDrawSolidColorQuad(selection, {x, font->height + 2}, color);

				x = 0;
				y += font->height + font->lineGap;
			}
		}
		x += GetCharWidth(font, b);
		if (b == 10) {

			selection.x = round(pos.x);
			selection.y = round(pos.y + y + 2);
			GfxDrawSolidColorQuad(selection, {x, font->height + 2}, color);

			x = 0;
			y += font->height + font->lineGap;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}

	selection.x = round(pos.x + startx);
	selection.y = round(pos.y + y + 2);
	GfxDrawSolidColorQuad(selection, {x - startx, font->height + 2}, color);
}

// StringList
//------------

#include "stringlist.cpp"

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
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = GetWordWidth(font, {node, i});
				if (wrapX <= pos.x + wordWidth) {
					pos.y += font->height + font->lineGap;
					pos.x = initialX;
				}
			}
			if (b == 9) {
				pos.x += 4*font->glyphs[32 - font->firstChar].xadvance;
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

StringListPos GetCharPos(Point2 cursor, 
						 BakedFont* font, StringList list, 
						 bool shouldWrap, float32 x1) {

	Point2 pos = {};

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
				if (x1 <= pos.x + wordWidth) {
					if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
						return {node, i};
					pos.y += height + font->lineGap;
					pos.x = initialX;

				}
			}
			if (b == 10) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return {node, i};
				pos.y += font->height + font->lineGap;
				pos.x = initialX;
			}
			if (font->firstChar <= b && b <= font->lastChar) {
				float32 width = font->glyphs[b - font->firstChar].xadvance;
				if (InBounds(pos, {width, font->height}, cursor))
					return {node, i};
				pos.x += width;
			}
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
						selection.y = round(pos.y + y);
						Dimensions2 dim = {2, height};
						GfxDrawSolidColorQuad(selection, dim, caretColor);
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
						selection.y = round(pos.y + y);
						Dimensions2 dim = {(x - startx), height};
						GfxDrawSolidColorQuad(selection, dim, color);
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
					selection.y = round(pos.y + y);
					Dimensions2 dim = {(x - startx), height};
					GfxDrawSolidColorQuad(selection, dim, color);
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
						selection.y = round(pos.y + y);
						Dimensions2 dim = {x, height};
						GfxDrawSolidColorQuad(selection, dim, color);

						x = 0;
						y += height + font->lineGap;
					}
				}
				x += GetCharWidth(font, b);
				if (b == 10) {

					selection.x = round(pos.x);
					selection.y = round(pos.y + y);
					Dimensions2 dim = {x, height};
					GfxDrawSolidColorQuad(selection, dim, color);

					x = 0;
					y += height + font->lineGap;
				}
			}

			prevCharWasWhiteSpace = whiteSpace;
		}
		if (end.node == node) break;
	}

	selection.y = round(pos.y + y);
	bool eq = StringListPosEquals(tail, head);
	if (!eq) {
		selection.x = round(pos.x + startx);
		GfxDrawSolidColorQuad(selection, {(x - startx), height}, color);
	}

	if (onEnd || eq) {
		selection.x = round(pos.x + x);
		GfxDrawSolidColorQuad(selection, {2, height}, caretColor);
	}
}
