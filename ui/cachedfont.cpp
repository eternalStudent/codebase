struct CachedGlyph {
	bool isTaken;
	uint16 id;

	int16 x0, y0, x1, y1;
	float32 xoff, yoff, xadvance;
};

// TODO: is 108,631 large enough?
#define GLYPH_CACHE_SIZE 108631
static CachedGlyph glyphTable[GLYPH_CACHE_SIZE];

struct {
	byte       * pixelData;
	RegionNode * atlasRoot;
	FixedSize    regionNodeAllocator;
	CachedGlyph* table;
} glyphCache;

void GlyphCacheInit() {
	glyphCache.regionNodeAllocator = CreateFixedSize(sizeof(RegionNode), KB(4));
	glyphCache.atlasRoot = (RegionNode*)FixedSizeAlloc(&glyphCache.regionNodeAllocator);
	glyphCache.atlasRoot->dim = {512, 512};
	glyphCache.pixelData = (byte*)OSAllocate(512*512);
	glyphCache.table = glyphTable;
}

//------------------------------------

#if _OS_WINDOWS
#  include "dwrite.cpp"
#  define FontFace 		    			IDWriteFontFace
#  define FontInit 						DWriteInit
#  define FontLoadFontFace				DWriteLoadFontFace
#  define FontLoadDefaultFontFace		DWriteLoadDefaultFontFace
#  define FontLoadFontFaceFromMemory 	DWriteLoadFontFaceFromMemory
#  define FontGetGlyphIndex 			DWriteGetGlyphIndex
#  define ScaledFont 					DWriteScaledFont
#  define FontGetScaledFont				DWriteGetScaledFont
#  define FontBakeGlyph 				DWriteBakeGlyph
#else
// TODO:
#endif

#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#include "../third_party/meow_hash_x64_aesni.h"
#pragma warning(pop)

uint64 GetHashCode(ScaledFont* font, uint16 glyphId) {
	uintptr_t face = (uintptr_t)font->face;
	float32 emSize = font->emSize;

	byte buffer[sizeof(face) + sizeof(emSize) + sizeof(glyphId)] = {};
	memcpy(buffer + 0, &face, sizeof(face));
	memcpy(buffer + sizeof(face), &emSize, sizeof(emSize));
	memcpy(buffer + sizeof(face) + sizeof(emSize), &glyphId, sizeof(glyphId));

    meow_u128 hash = MeowHash(MeowDefaultSeed, sizeof(buffer), buffer);
    uint64 hash64 = MeowU64From(hash, 0);
    return hash64;
}

Image FontGetImageFromAtlas(Dimensions2i dim, byte* data) {
	return Image{dim.width, dim.height, 1, data};
}

void FontReleaseCachedGlyph(CachedGlyph* glyph) {
	Point2i pos0 = {glyph->x0 - 1, glyph->y0 - 1};
	Point2i pos1 = {glyph->x1 - 1, glyph->y1 - 1};
	Dimensions2i dim = pos1 - pos0;
	if (dim.width && dim.height) {
		dim = dim + Point2i{1, 1};
		RegionNode* node = RegionGetByPosAndDim(glyphCache.atlasRoot, pos0, dim);
		if (node) {
			RegionFree(node, &glyphCache.regionNodeAllocator);
		}
		else {
			__debugbreak();
		}
	}

	glyph->isTaken = false;
}

CachedGlyph FontGetGlyph(ScaledFont* font, uint32 codepoint) {
	uint16 glyphId = FontGetGlyphIndex(font->face, codepoint);

	uint64 hashCode = GetHashCode(font, glyphId);
	uint64 index = hashCode % GLYPH_CACHE_SIZE;

	CachedGlyph* glyph = glyphCache.table + index;

	if (glyph->isTaken) {
		if (glyph->id == glyphId)
			return *glyph;

		GfxFlush();
		FontReleaseCachedGlyph(glyph);
	}

	glyph->isTaken = true;
	glyph->id = glyphId;
	FontBakeGlyph(font, 
				  glyphId, 
				  glyph, 
				  glyphCache.pixelData, 
				  glyphCache.atlasRoot, 
				  &glyphCache.regionNodeAllocator);
	Image fontAtlasImage = FontGetImageFromAtlas(glyphCache.atlasRoot->dim, glyphCache.pixelData);
	GfxSetFontAtlas(fontAtlasImage);

	return *glyph;
}

float32 FontGetGlyphWidth(ScaledFont* font, uint32 codepoint) {
	CachedGlyph glyph = FontGetGlyph(font, codepoint);
	return glyph.xadvance;
}

float32 FontDrawGlyph(Point2 pos, CachedGlyph glyph, Color color) {

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

float32 FontDrawGlyph(Point2 pos, ScaledFont* font, uint32 codepoint, Color color) {
	CachedGlyph glyph = FontGetGlyph(font, codepoint);

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

// String
//---------

float32 FontGetWordWidth(ScaledFont* font, byte* data, byte* end) {
	float32 width = 0;
	uint32 codepoint;
	ssize bytesToEncode;
	for (; data < end; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, end, &codepoint);
		if (IsWhiteSpace(codepoint)) 
			return width;

		width += FontGetGlyphWidth(font, codepoint);
	}
	return width;
}

Point2 FontDrawString(Point2 pos, ScaledFont* font, String string, Color color,
					  bool shouldWrap = false, 
					  float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x = pos.x;
	float32 y = pos.y + font->ascent;
	float32 x0 = pos.x;

	uint32 codepoint;
	ssize bytesToEncode;
	byte* end = string.data + string.length;
	for (byte* data = string.data; data < end; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, end, &codepoint);
		bool whiteSpace = IsWhiteSpace(codepoint);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, data, end);
			if (x1 <= pos.x + wordWidth) {
				y += font->height + font->lineGap;
				x = x0;
			}
		}

		if (codepoint == 10) {
			y += font->height + font->lineGap;
			x = x0;
		}
		else {
			CachedGlyph glyph = FontGetGlyph(font, codepoint);
			x += FontDrawGlyph({x, y}, glyph, color);
		}
	}
	return {x, y - font->ascent};
}

ssize FontGetCharIndex(Point2 cursor,
				   	   ScaledFont* font, 
				   	   String string, 
				   	   bool shouldWrap = false, 
				   	   float32 x1 = 0) {

	Point2 pos = {};

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;

	uint32 codepoint;
	ssize bytesToEncode;
	byte* end = string.data + string.length;
	for (byte* data = string.data; data < end; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, end, &codepoint);
		bool whiteSpace = IsWhiteSpace(codepoint);
		ssize i = data - string.data;

		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, data, end);
			if (x1 <= pos.x + wordWidth) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return i;
				pos.y += font->height + font->lineGap;
				pos.x = x0;
			}
		}
		if (codepoint == 10) {
			if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
				return i;
			pos.y += font->height + font->lineGap;
			pos.x = x0;
		}
		else {
			float32 width = FontGetGlyphWidth(font, codepoint);
			if (InBounds(pos, {width, font->height}, cursor))
				return i;
			pos.x += width;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}

	return string.length;
}

void FontDrawTextSelection(Point2 pos, 
						   ScaledFont* font, 
						   String string,
						   ssize selectionStartIndex, 
						   ssize selectionEndIndex,  
						   Color color,
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	bool prevCharWasWhiteSpace = false;
	float32 x = 0;
	float32 y = 0;
	byte* data = string.data;
	byte* dataEnd = string.data + string.length;
	byte* selectionStart = string.data + selectionStartIndex;
	byte* selectionEnd = string.data + selectionEndIndex;
	uint32 codepoint;
	ssize bytesToEncode;

	// start by finding position of start.
	for (; data < selectionStart + 1; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, dataEnd, &codepoint);
		bool whiteSpace = IsWhiteSpace(codepoint);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, data, dataEnd);
			if (x1 <= x + wordWidth) {
				x = 0;
				y += font->height + font->lineGap;
			}
		}
		if (data == selectionStart) break;
		x += FontGetGlyphWidth(font, codepoint);
		if (codepoint == 10) {
			x = 0;
			y += font->height + font->lineGap;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}

	// iterate till end of line
	Point2 selection;
	float32 startx = x;
	for (; data < selectionEnd; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, dataEnd, &codepoint);
		bool whiteSpace = IsWhiteSpace(codepoint);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, data, dataEnd);
			if (x1 <= x + wordWidth) {

				selection.x = round(pos.x + startx);
				selection.y = round(pos.y + y + 2);
				GfxDrawSolidColorQuad(selection, {x - startx, font->height + 2}, color);
				startx = 0;

				x = 0;
				y += font->height + font->lineGap;
				data += bytesToEncode;
				break;
			}
		}
		x += FontGetGlyphWidth(font, codepoint);
		if (codepoint == 10) {

			selection.x = round(pos.x + startx);
			selection.y = round(pos.y + y + 2);
			GfxDrawSolidColorQuad(selection, {x - startx, font->height + 2}, color);
			startx = 0;
			
			x = 0;
			y += font->height + font->lineGap;
			data += bytesToEncode;
			break;
		}
		prevCharWasWhiteSpace = whiteSpace;
	}
	
	// now keep iterating until end
	for (; data < selectionEnd; data += bytesToEncode) {
		bytesToEncode = UTF8Decode(data, dataEnd, &codepoint);
		bool whiteSpace = IsWhiteSpace(codepoint);
		if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
			float32 wordWidth = FontGetWordWidth(font, data, dataEnd);
			if (x1 <= x + wordWidth) {

				selection.x = round(pos.x);
				selection.y = round(pos.y + y + 2);
				GfxDrawSolidColorQuad(selection, {x, font->height + 2}, color);

				x = 0;
				y += font->height + font->lineGap;
			}
		}
		x += FontGetGlyphWidth(font, codepoint);
		if (codepoint == 10) {

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

float32 FontGetWordWidth(ScaledFont* font, StringListPos pos) {
	float32 width = 0;
	for (StringNode* node = pos.node; node != NULL; node = node->next) {
		String string = node->string;
		byte* start = string.data;
		if (node == pos.node) start += pos.index;
		uint32 codepoint;
		ssize bytesToEncode;
		byte* end = string.data + string.length;
		for (byte* data = start; data < end; data += bytesToEncode) {
			bytesToEncode = UTF8Decode(data, end, &codepoint);
			if (IsWhiteSpace(codepoint)) 
				return width;

			width += FontGetGlyphWidth(font, codepoint);
		}
	}
	return width;
}

Point2 FontDrawStringList(Point2 pos, 
						  ScaledFont* font, 
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
		uint32 codepoint;
		ssize bytesToEncode;
		byte* end = string.data + string.length;
		for (byte* data = string.data; data < end; data += bytesToEncode) {
			bytesToEncode = UTF8Decode(data, end, &codepoint);
			bool whiteSpace = IsWhiteSpace(codepoint);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = FontGetWordWidth(font, {node, data - string.data});
				if (x1 <= pos.x + wordWidth) {
					y += font->height + font->lineGap;
					x = x0;
				}
			}
			if (codepoint == 10) {
				y += font->height + font->lineGap;
				x = x0;
			}
			else {
				CachedGlyph glyph = FontGetGlyph(font, codepoint);
				x += FontDrawGlyph({x, y}, glyph, color);
			}
			prevCharWasWhiteSpace = whiteSpace;
		}
	}
	return {x, y - font->ascent};
}

StringListPos FontGetCharPos(Point2 cursor, 
							 ScaledFont* font, 
							 StringList list, 
							 bool shouldWrap = false, 
							 float32 x1 = 0) {

	Point2 pos = {};

	bool prevCharWasWhiteSpace = false;
	float32 x0 = pos.x;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		uint32 codepoint;
		ssize bytesToEncode;
		byte* end = string.data + string.length;
		for (byte* data = string.data; data < end; data += bytesToEncode) {
			bytesToEncode = UTF8Decode(data, end, &codepoint);
			bool whiteSpace = IsWhiteSpace(codepoint);
			if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
				float32 wordWidth = FontGetWordWidth(font, {node, data - string.data});
				if (x1 <= pos.x + wordWidth) {
					if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
						return {node, data - string.data};
					pos.y += font->height + font->lineGap;
					pos.x = x0;

				}
			}
			if (codepoint == 10) {
				if (InBounds(pos, {x1 - pos.x, font->height}, cursor))
					return {node, data - string.data};
				pos.y += font->height + font->lineGap;
				pos.x = x0;
			}
			else {
				float32 width = FontGetGlyphWidth(font, codepoint);
				if (InBounds(pos, {width, font->height}, cursor))
					return {node, data - string.data};
				pos.x += width;
			}
			prevCharWasWhiteSpace = whiteSpace;
		}
	}

	return SL_END(list);
}

void FontDrawTextSelection(Point2 pos, 
						   ScaledFont* font, 
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
		ssize bytesToEncode;
		uint32 codepoint;
		byte* endSelection = node == end.node ? string.data + end.index : string.data + string.length;
		for (byte* data = string.data; data < endSelection; data += bytesToEncode) {
			bytesToEncode = UTF8Decode(data, string.data + string.length, &codepoint);
			bool whiteSpace = IsWhiteSpace(codepoint);

			if (state == 0) {
				// start by finding position of start.
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = FontGetWordWidth(font, {node, data - string.data});
					if (x1 <= x + wordWidth) {
						x = 0;
						y += font->height + font->lineGap;
					}
				}
				if (node == start.node && data == string.data + start.index) {
					state++;
					startx = x;

					if (!onEnd) {
						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {2, font->height};
						GfxDrawSolidColorQuad(selection, dim, caretColor);
					}

					data -= bytesToEncode;
					continue;
				}

				x += FontGetGlyphWidth(font, codepoint);
				if (codepoint == 10) {
					x = 0;
					y += font->height + font->lineGap;
				}
			}

			if (state == 1) {
				// iterate till end of line
				if (shouldWrap && prevCharWasWhiteSpace && !whiteSpace) {
					float32 wordWidth = FontGetWordWidth(font, {node, data - string.data});
					if (x1 <= x + wordWidth) {

						selection.x = round(pos.x + startx);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {(x - startx), font->height};
						GfxDrawSolidColorQuad(selection, dim, color);
						startx = 0;

						x = 0;
						y += font->height + font->lineGap;
						state++;
						data -= bytesToEncode;
						continue;
					}
				}
				x += FontGetGlyphWidth(font, codepoint);
				if (codepoint == 10) {

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
					float32 wordWidth = FontGetWordWidth(font, {node, data - string.data});
					if (x1 <= x + wordWidth) {

						selection.x = round(pos.x);
						selection.y = round(pos.y + y);
						Dimensions2 dim = {x, font->height};
						GfxDrawSolidColorQuad(selection, dim, color);

						x = 0;
						y += font->height + font->lineGap;
					}
				}
				x += FontGetGlyphWidth(font, codepoint);
				if (codepoint == 10) {

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

//-------------------------------------------------------

bool IsStartOfLine(TextMetrics metrics, 
				   ScaledFont* font,
				   StringListPos start, 
				   bool shouldWrap, 
				   float32 x1) {

	if (metrics.lastCharIsNewLine)
		return true;
	if (!shouldWrap || !metrics.lastCharIsWhiteSpace) 
		return false;
	String string = start.node->string;
	uint32 codepoint;
	UTF8Decode(string.data, string.data + string.length, &codepoint);
	if (IsWhiteSpace(codepoint)) 
		return false;
	float32 wordWidth = FontGetWordWidth(font, start);
	return x1 <= metrics.x + wordWidth;
}

TextMetrics NextTextMetrics(TextMetrics metrics, 
							ScaledFont* font, 
							StringListPos pos,
							bool shouldWrap, 
							float32 x1) {

	if (metrics.lastCharIsNewLine) {
		metrics.x = 0;
		metrics.y += font->height + font->lineGap;
	}
	String string = pos.node->string;
	uint32 codepoint;
	UTF8Decode(string.data + pos.index, string.data + string.length, &codepoint);
	bool whiteSpace = IsWhiteSpace(codepoint);
	if (shouldWrap && metrics.lastCharIsWhiteSpace && !whiteSpace) {
		float32 wordWidth = FontGetWordWidth(font, pos);
		if (x1 <= metrics.x + wordWidth) {
			metrics.maxx = MAX(metrics.maxx, metrics.x);
			metrics.x = 0;
			metrics.y += font->height + font->lineGap;
		}
	}
	metrics.x += FontGetGlyphWidth(font, codepoint);
	metrics.maxx = MAX(metrics.x, metrics.maxx);
	metrics.lastCharIsNewLine = codepoint == 10;
	metrics.lastCharIsWhiteSpace = whiteSpace;
	return metrics;
}

TextMetrics GetTextMetrics(ScaledFont* font, 
						   StringList list, 
						   StringListPos end, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {
	
	TextMetrics metrics = {};
	metrics.y = font->height;

	if (list.length == 0) return metrics;

	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		byte* dataEnd = node == end.node ? string.data + end.index : string.data + string.length;
		uint32 codepoint;
		ssize bytesToEncode;
		for (byte* data = string.data; data < dataEnd; data += bytesToEncode) {
			bytesToEncode = UTF8Decode(data, dataEnd, &codepoint);
			metrics = NextTextMetrics(metrics, font, {node, data - string.data}, shouldWrap, x1);
		}
		if (node == end.node) break;
	}

	return metrics;
}

TextMetrics GetTextMetrics(ScaledFont* font, 
						   StringList list, 
						   bool shouldWrap = false, 
						   float32 x1 = 0) {

	if (list.length == 0) return {};
	return GetTextMetrics(font, list, {list.last, list.last->string.length}, shouldWrap, x1);
}