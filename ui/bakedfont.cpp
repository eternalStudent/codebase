/*
 * NOTE:
 *  This was written for pre-baked fonts, 
 *  for the slightly more robust version see cachedfont.cpp
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

	int32 firstChar;
	int32 lastChar;

	BakedGlyph glyphs[96];
};

struct BakedAtlasBitmap {
	int32 width, height, x, y, bottom_y;
	byte* data;
};

BakedAtlasBitmap CreateAtlasBitmap(int32 width, int32 height, byte* buffer) {
	BakedAtlasBitmap atlas;
	atlas.width = width;
	atlas.height = height;
	atlas.x = 1;
	atlas.y = 1;
	atlas.bottom_y = 1;
	atlas.data = buffer;
	memset(atlas.data, 0, width*height);
	return atlas;
}

#if _OS_WINDOWS
#  include "dwrite.cpp"
#  define FontFace 		    		IDWriteFontFace
#  define FontInit 					DWriteInit
#  define FontLoadFontFace			DWriteLoadFontFace
#  define FontLoadDefaultFontFace	DWriteLoadDefaultFontFace
#  define FontBakeFont				DWriteBakeFont
#else
// TODO:
#endif

Image FontGetImageFromAtlas(BakedAtlasBitmap* atlas) {
	return Image{atlas->width, atlas->height, 1, atlas->data};
}

float32 FontDrawGlyph(Point2 pos, BakedGlyph glyph, Color color) {
	float32 round_y = round(pos.y + glyph.yoff);
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

float32 FontDrawGlyph(Point2 pos, BakedFont* font, int32 glyphIndex, Color color) {
	BakedGlyph glyph = font->glyphs[glyphIndex - font->firstChar];
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

float32 FontGetCharWidth(BakedFont* font, byte b) {
	if (font->firstChar <= b && b <= font->lastChar) {
		BakedGlyph glyph = font->glyphs[b - font->firstChar];
		return glyph.xadvance;
	}
	if (b == 10) return font->glyphs[32 - font->firstChar].xadvance;
	return 0;
}

// String
//------------

float32 FontGetWordWidth(BakedFont* font, String string, ssize i) {
	float32 width = 0;
	for (; i < string.length; i++) {
		byte b = string.data[i];
		if (IsWhiteSpace(b)) return width;
		width += FontGetCharWidth(font, b);
	}
	return width;
}

Point2 FontDrawString(Point2 pos, 
					  BakedFont* font, 
					  String string, 
					  Color color,
					  bool shouldWrap = false, 
					  float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;
	float32 x = pos.x;
	float32 y = pos.y + font->ascent;

	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, string, i);
			if (x1 <= pos.x + wordWidth) {
				y += font->height + font->lineGap;
				x = x0;
			}
		}
		if (b == 10) {
			y += font->height + font->lineGap;
			x = x0;
		}
		if (font->firstChar <= b && b <= font->lastChar) {
			BakedGlyph glyph = font->glyphs[b - font->firstChar];
			x += FontDrawGlyph({x, y}, glyph, color);
		}
		prevCharWasWhiteSpace = whiteSpace;
	}
	return {x, y - font->ascent};
}

ssize FontGetCharIndex(Point2 cursor,
					   BakedFont* font, 
					   String string, 
					   bool shouldWrap = false, 
					   float32 x1 = 0) {

	Point2 pos = {};

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;

	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, string, i);
			if (x1 <= pos.x + wordWidth) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return i;
				pos.y += font->height + font->lineGap;
				pos.x = x0;
			}
		}
		if (b == 10) {
			if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
				return i;
			pos.y += font->height + font->lineGap;
			pos.x = x0;
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

void FontDrawTextSelection(Point2 pos, 
						   BakedFont* font, 
						   String string,
						   ssize start, 
						   ssize end,  
						   Color color,
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	ssize i = 0;

	// start by finding position of start.
	for (; i < start + 1; i++) {
		byte b = string.data[i];
		bool whiteSpace = IsWhiteSpace(b);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, string, i);
			if (x1 <= x + wordWidth) {
				x = 0;
				y += font->height + font->lineGap;
			}
		}
		if (i == start) break;
		x += FontGetCharWidth(font, b);
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
			float32 wordWidth = FontGetWordWidth(font, string, i);
			if (x1 <= x + wordWidth) {

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
		x += FontGetCharWidth(font, b);
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
			float32 wordWidth = FontGetWordWidth(font, string, i);
			if (x1 <= x + wordWidth) {

				selection.x = round(pos.x);
				selection.y = round(pos.y + y + 2);
				GfxDrawSolidColorQuad(selection, {x, font->height + 2}, color);

				x = 0;
				y += font->height + font->lineGap;
			}
		}
		x += FontGetCharWidth(font, b);
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

float32 FontGetWordWidth(BakedFont* font, StringListPos pos) {
	float32 width = 0;
	for (StringNode* node = pos.node; node != NULL; node = node->next) {
		String string = node->string;
		ssize start = node == pos.node ? pos.index : 0;
		for (ssize i = start; i < string.length; i++) {
			byte b = string.data[i];
			if (IsWhiteSpace(b)) return width;
			width += FontGetCharWidth(font, b);
		}
	}
	return width;
}

Point2 FontDrawStringList(Point2 pos, 
						  BakedFont* font, 
						  StringList list, 
						  Color color,
						  bool shouldWrap = false, 
						  float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;
	float32 x = pos.x;
	float32 y = pos.y + font->ascent;

	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = FontGetWordWidth(font, {node, i});
				if (x1 <= pos.x + wordWidth) {
					y += font->height + font->lineGap;
					x = x0;
				}
			}
			if (b == 10) {
				y += font->height + font->lineGap;
				x = x0;
			}
			if (font->firstChar <= b && b <= font->lastChar) {
				BakedGlyph glyph = font->glyphs[b - font->firstChar];
				x += FontDrawGlyph({x, y}, glyph, color);
			}
			prevCharWasWhiteSpace = whiteSpace;
		}
	}
	return {x, y - font->ascent};
}

StringListPos FontGetCharPos(Point2 cursor, 
							 BakedFont* font, 
							 StringList list, 
							 bool shouldWrap = false, 
							 float32 x1 = 0) {

	Point2 pos = {};

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool whiteSpace = IsWhiteSpace(b);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = FontGetWordWidth(font, {node, i});
				if (x1 <= pos.x + wordWidth) {
					if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
						return {node, i};
					pos.y += font->height + font->lineGap;
					pos.x = x0;

				}
			}
			if (b == 10) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return {node, i};
				pos.y += font->height + font->lineGap;
				pos.x = x0;
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

void FontDrawTextSelection(Point2 pos, 
						   BakedFont* font, 
						   StringList list, 
						   StringListPos tail,
						   StringListPos head, 
						   Color color,
						   Color caretColor,
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	int32 state = 0;
	float32 startx = x;
	Point2 selection;

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
					float32 wordWidth = FontGetWordWidth(font, {node, i});
					if (x1 <= x + wordWidth) {
						x = 0;
						y += font->height + font->lineGap;
					}
				}
				if (node == start.node && i == start.index) {
					state++;
					startx = x;

					if (!onEnd) {
						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {2, font->height};
						GfxDrawSolidColorQuad(selection, dim, caretColor);
					}

					i--;
					continue;
				}

				x += FontGetCharWidth(font, b);
				if (b == 10) {
					x = 0;
					y += font->height + font->lineGap;
				}
			}

			if (state == 1) {
				// iterate till end of line
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = FontGetWordWidth(font, {node, i});
					if (x1 <= x + wordWidth) {

						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {(x - startx), font->height};
						GfxDrawSolidColorQuad(selection, dim, color);
						startx = 0;

						x = 0;
						y += font->height + font->lineGap;
						state++;
						i--;
						continue;
					}
				}
				x += FontGetCharWidth(font, b);
				if (b == 10) {

					selection.x = round(pos.x + startx);
					selection.y = round(pos.y + y);
					Dimensions2 dim = {(x - startx), font->height};
					GfxDrawSolidColorQuad(selection, dim, color);
					startx = 0;
					
					x = 0;
					y += font->height + font->lineGap;
					state++;
					continue;
				}
			}

			if (state == 2) {
				// now keep iterating until end
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = FontGetWordWidth(font, {node, i});
					if (x1 <= x + wordWidth) {

						selection.x = round(pos.x);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {x, font->height};
						GfxDrawSolidColorQuad(selection, dim, color);

						x = 0;
						y += font->height + font->lineGap;
					}
				}
				x += FontGetCharWidth(font, b);
				if (b == 10) {

					selection.x = round(pos.x);
					selection.y = round(pos.y + y);
					Dimensions2 dim = {x, font->height};
					GfxDrawSolidColorQuad(selection, dim, color);

					x = 0;
					y += font->height + font->lineGap;
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
		GfxDrawSolidColorQuad(selection, {(x - startx), font->height}, color);
	}

	if (onEnd || eq) {
		selection.x = round(pos.x + x);
		GfxDrawSolidColorQuad(selection, {2, font->height}, caretColor);
	}
}

//-----------------------------------------------

bool IsStartOfLine(TextMetrics metrics, 
				   BakedFont* font, 
				   String string,
				   ssize i, 
				   bool shouldWrap, 
				   float32 x1) {

	if (metrics.lastCharIsNewLine)
		return true;
	if (!shouldWrap || !metrics.lastCharIsWhiteSpace) 
		return false;
	byte b = string.data[i];
	if (IsWhiteSpace(b)) 
		return false;
	float32 wordWidth = FontGetWordWidth(font, string, i);
	return x1 <= metrics.x + wordWidth;
}

TextMetrics NextTextMetrics(TextMetrics metrics, 
							BakedFont* font, 
							String string,
							ssize i, 
							bool shouldWrap, 
							float32 x1) {

	if (metrics.lastCharIsNewLine) {
		metrics.x = 0;
		metrics.y += font->height + font->lineGap;
	}
	byte b = string.data[i];
	bool whiteSpace = IsWhiteSpace(b);
	if (shouldWrap && metrics.lastCharIsWhiteSpace && !whiteSpace) {
		float32 wordWidth = FontGetWordWidth(font, string, i);
		if (x1 <= metrics.x + wordWidth) {
			metrics.maxx = MAX(metrics.maxx, metrics.x);
			metrics.x = 0;
			metrics.y += font->height + font->lineGap;
		}
	}
	metrics.x += FontGetCharWidth(font, b);
	metrics.maxx = MAX(metrics.x, metrics.maxx);
	metrics.lastCharIsNewLine = b == 10;
	metrics.lastCharIsWhiteSpace = whiteSpace;
	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, 
						   String string, 
						   ssize end, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {
	
	TextMetrics metrics = {};
	if (string.length == 0) return metrics;

	metrics.y = font->height;
	for (ssize i = 0; i < end; i++)
		metrics = NextTextMetrics(metrics, font, string, i, shouldWrap, x1);

	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, 
						   String string, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	return GetTextMetrics(font, string, string.length, shouldWrap, x1);
}

//-------------------------------------------------------

bool IsStartOfLine(TextMetrics metrics, 
				   BakedFont* font,
				   StringListPos start, 
				   bool shouldWrap, 
				   float32 x1) {

	if (metrics.lastCharIsNewLine)
		return true;
	if (!shouldWrap || !metrics.lastCharIsWhiteSpace) 
		return false;
	byte b = start.node->string.data[start.index];
	if (IsWhiteSpace(b)) 
		return false;
	float32 wordWidth = FontGetWordWidth(font, start);
	return x1 <= metrics.x + wordWidth;
}

TextMetrics NextTextMetrics(TextMetrics metrics, 
							BakedFont* font, 
							StringListPos pos,
							bool shouldWrap, 
							float32 x1) {

	if (metrics.lastCharIsNewLine) {
		metrics.x = 0;
		metrics.y += font->height + font->lineGap;
	}
	byte b = pos.node->string.data[pos.index];
	bool whiteSpace = IsWhiteSpace(b);
	if (shouldWrap && metrics.lastCharIsWhiteSpace && !whiteSpace) {
		float32 wordWidth = FontGetWordWidth(font, pos);
		if (x1 <= metrics.x + wordWidth) {
			metrics.maxx = MAX(metrics.maxx, metrics.x);
			metrics.x = 0;
			metrics.y += font->height + font->lineGap;
		}
	}
	metrics.x += FontGetCharWidth(font, b);
	metrics.maxx = MAX(metrics.x, metrics.maxx);
	metrics.lastCharIsNewLine = b == 10;
	metrics.lastCharIsWhiteSpace = whiteSpace;
	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, 
						   StringList list, 
						   StringListPos end, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {
	
	TextMetrics metrics = {};
	metrics.y = font->height;

	if (list.length == 0) return metrics;

	LINKEDLIST_FOREACH(&list, StringNode, node) {
		ssize endIndex = node == end.node ? end.index : node->string.length;
		for (ssize i = 0; i < endIndex; i++) {
			metrics = NextTextMetrics(metrics, font, {node, i}, shouldWrap, x1);
		}
		if (node == end.node) break;
	}

	return metrics;
}

TextMetrics GetTextMetrics(BakedFont* font, 
						   StringList list, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	if (list.length == 0) return {};
	return GetTextMetrics(font, list, {list.last, list.last->string.length}, shouldWrap, x1);
}
