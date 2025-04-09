#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

struct {
	FT_Library  library;
} ftype;

void FTypeInit(FontAlphaMode alphaMode) {
	(void)alphaMode;
	FT_Error error = FT_Init_FreeType(&ftype.library);
	ASSERT(!error);
}

FT_Face FTypeLoadFontFace(const char* filePath) {
	FT_Face face;
	FT_Error error = FT_New_Face(ftype.library, filePath, 0, &face);
	ASSERT(!error);

	return face;
}

FT_Face FTypeLoadDefaultFontFace() {
	FcPattern* pattern = FcNameParse((const FcChar8*)"monospace");
    FcBool success = FcConfigSubstitute(0,pattern,FcMatchPattern);
    if (!success) {
        LOG("failed to find font");
        return {};
    }
    
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern* fontMatch = FcFontMatch(NULL, pattern, &result);
    if (!fontMatch) {
        LOG("failed to match font");
        return {};
    }

    FcChar8 *filename;
    if (FcPatternGetString(fontMatch, FC_FILE, 0, &filename) != FcResultMatch) {
        LOG("failed to get font file name");
        return {};
    }

    return FTypeLoadFontFace((const char*)filename);
}

FT_Face FTypeLoadFontFaceFromMemory(byte* buffer, ssize size) {
	FT_Face face;
	FT_Error error = FT_New_Memory_Face(ftype.library, buffer, size, 0, &face);
	ASSERT(!error);

	return face;
}

void FTypeGetScaledFontMetrics(
	FT_Face fontFace, 
	float32 pixelSize,
	bool scaleForPixelHeight,

	float32* height,
	float32* ascent,
	float32* descent,
	float32* lineGap,
	//float32* xHeight,
	float32* emSize,
	float32* pixelsPerDesignUnits) {

	if (scaleForPixelHeight) {
		uint16 heightInDesignUnits = fontFace->ascender - fontFace->descender;

		*emSize = pixelSize*fontFace->units_per_EM/heightInDesignUnits;
		*pixelsPerDesignUnits = pixelSize/heightInDesignUnits;
		*height = pixelSize;
	}
	else {
		*emSize = (96.f/72.f)*pixelSize;
		*pixelsPerDesignUnits = *emSize/fontFace->units_per_EM;
		*height = *pixelsPerDesignUnits*(fontFace->ascender + fontFace->descender);
	}

	float32 lineHeight = *pixelsPerDesignUnits*fontFace->height;

	*ascent = *pixelsPerDesignUnits*fontFace->ascender;
	*descent = *pixelsPerDesignUnits*-fontFace->descender;
	*lineGap = lineHeight - *height;
	//*xHeight = *pixelsPerDesignUnits*fontMetrics.xHeight;
}


FT_UInt FTypeGetGlyphIndex(FT_Face face, FT_ULong codepoint) {
	return FT_Get_Char_Index(face, codepoint);
} 

//------------------------------------
#if FONT_BAKED

BakedFont FTypeBakeFont(
	FT_Face fontFace,
	float32 pixelSize,
	bool scaleForPixelHeight, 
	BakedAtlasBitmap* atlas) {

	BakedFont font;
	font.firstChar = 32;
	font.lastChar = 127;

	float32 emSize, pixelsPerDesignUnits;
	FTypeGetScaledFontMetrics(fontFace, pixelSize, scaleForPixelHeight,
		&font.height,
		&font.ascent,
		&font.descent,
		&font.lineGap,
		&emSize,
		&pixelsPerDesignUnits);

	FT_Set_Char_Size(fontFace, (FT_F26Dot6)(font.height*64), 0, 72, 0);

	for (uint32 i = 0; i < 96; i ++) {
		FT_UInt glyphId = FTypeGetGlyphIndex(fontFace, 32 + i);
		FT_Load_Glyph(fontFace, glyphId, FT_LOAD_RENDER);

		FT_Bitmap bitmap = fontFace->glyph->bitmap;
		int32 glyphHeight = bitmap.rows;
		int32 glyphWidth = bitmap.width;

		if (atlas->x + glyphWidth + 1 >= atlas->width)
			atlas->y = atlas->bottom_y, atlas->x = 1;

		byte* outData = atlas->data + atlas->x + atlas->width*(atlas->y);
		int32 outPitch = atlas->width;
		int32 inPitch = bitmap.pitch;
		byte* inData = bitmap.buffer;
		for (int32 y = 0; y < glyphHeight; y++) {
			memcpy(outData + outPitch*y, inData + inPitch*y, glyphWidth);
		}

		font.glyphs[i].xoff = (float32)fontFace->glyph->bitmap_left;
		font.glyphs[i].yoff = font.ascent - (float32)fontFace->glyph->bitmap_top;
		font.glyphs[i].x0 = (int16)atlas->x;
		font.glyphs[i].y0 = (int16)atlas->y;
		font.glyphs[i].x1 = (int16)atlas->x + (int16)glyphWidth;
		font.glyphs[i].y1 = (int16)atlas->y + (int16)glyphHeight;
		font.glyphs[i].xadvance = (float32)(fontFace->glyph->advance.x >> 6);

		atlas->x += (int16)glyphWidth + 1;
		if (atlas->y + glyphHeight + 1 > atlas->bottom_y)
			atlas->bottom_y = atlas->y + (int16)glyphHeight + 1;

	}

	return font;
}

#endif

//------------------------------------------------

#if FONT_CACHED

struct FTypeScaledFont {
	FT_Face face;

	float32 emSize;
	float32 pixelsPerDesignUnits;

	float32 height;
	float32 ascent;
	float32 descent;
	float32 lineGap;
	float32 xHeight;
};

FTypeScaledFont FTypeGetScaledFont(FT_Face face, float32 size, bool scaleForPixelHeight) {
	FTypeScaledFont font;
	font.face = face;
	FTypeGetScaledFontMetrics(face, size, scaleForPixelHeight,
		&font.height,
		&font.ascent,
		&font.descent,
		&font.lineGap,
		&font.emSize,
		&font.pixelsPerDesignUnits);

	FT_Set_Char_Size(face, (FT_F26Dot6)(font.height*64), 0, 72, 0);
	return font;
}

void FTypeBakeGlyph(FTypeScaledFont* font, 
					FT_UInt glyphId, 
					CachedGlyph* glyph,
					byte* outPixelBuffer,
					RegionNode* atlasRoot, 
					FixedSize* regionNodeAllocator) {

	FT_Load_Glyph(font->face, glyphId, FT_LOAD_RENDER);
	
	glyph->xoff = (float32)font->face->glyph->bitmap_left;
	glyph->yoff = font->ascent - (float32)font->face->glyph->bitmap_top;
	FT_Bitmap bitmap = font->face->glyph->bitmap;
	int32 glyphHeight = bitmap.rows;
	int32 glyphWidth = bitmap.width;

	if (glyphWidth && glyphHeight) {
		RegionNode* node = RegionAlloc(atlasRoot, regionNodeAllocator, {glyphWidth + 1, glyphHeight + 1});
		if (node == NULL)
			return;

		byte* outData = outPixelBuffer + node->x + 1 + atlasRoot->width*(node->y + 1);
		int32 outPitch = atlasRoot->width;
		int32 inPitch = bitmap.pitch;
		byte* inData = bitmap.buffer;
		for (int32 y = 0; y < glyphHeight; y++) {
			memcpy(outData + outPitch*y, inData + inPitch*y, glyphWidth);
		}

		glyph->x0 = (int16)node->x + 1;
		glyph->y0 = (int16)node->y + 1;
		glyph->x1 = (int16)node->x + 1 + (int16)glyphWidth;
		glyph->y1 = (int16)node->y + 1 + (int16)glyphHeight;
	}
	else {
		glyph->x0 = 0;
		glyph->y0 = 0;
		glyph->x1 = 0;
		glyph->y1 = 0;
	}

	glyph->xadvance = (float32)(font->face->glyph->advance.x >> 6);
	
}

#endif