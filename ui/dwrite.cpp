#include <dwrite.h>

struct {
	IDWriteFactory* factory;
	IDWriteRenderingParams* renderingParams;
	IDWriteBitmapRenderTarget* renderTarget;
} dwrite;

void DWriteInit(BOOL linearRendering) {
	const UINT32 width = 512;
	const UINT32 height = 512;
	HRESULT hr;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite.factory);
	ASSERT_HR(hr);

	IDWriteRenderingParams* baseRenderingParams;
	hr = dwrite.factory->CreateRenderingParams(&baseRenderingParams);
	ASSERT_HR(hr);
	
	FLOAT gamma = baseRenderingParams->GetGamma();
	FLOAT enhanced_contrast = baseRenderingParams->GetEnhancedContrast();
	hr = dwrite.factory->CreateCustomRenderingParams(
		linearRendering ? 1 : gamma,
		enhanced_contrast,
		0,
		DWRITE_PIXEL_GEOMETRY_FLAT,
		DWRITE_RENDERING_MODE_NATURAL,
		&dwrite.renderingParams);
	ASSERT_HR(hr);
	
	IDWriteGdiInterop* gdiInterop;
	hr = dwrite.factory->GetGdiInterop(&gdiInterop);
	ASSERT_HR(hr);
	
	hr = gdiInterop->CreateBitmapRenderTarget(NULL, width, height, &dwrite.renderTarget);
	ASSERT_HR(hr);

	baseRenderingParams->Release();
	gdiInterop->Release();
}

IDWriteFontFace* DWriteLoadFontFace(const WCHAR* filePath) {
	HRESULT hr;
	
	// DWrite Font File Reference
	IDWriteFontFile* fontFile;
	hr = dwrite.factory->CreateFontFileReference(filePath, NULL, &fontFile);
	ASSERT_HR(hr);
	
	// DWrite Font Face
	IDWriteFontFace* fontFace;
	hr = dwrite.factory->CreateFontFace(DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &fontFile, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontFace);
	ASSERT_HR(hr);

	fontFile->Release();

	return fontFace;
}

IDWriteFontFace* DWriteLoadDefaultFontFace() {
	WCHAR filePath[MAX_PATH] = {};
	LPWSTR ptr = filePath + GetEnvironmentVariableW(L"windir", (LPWSTR)filePath, MAX_PATH);
	COPY(L"\\Fonts\\consola.ttf", ptr);
	
	return DWriteLoadFontFace(filePath);
}

void DWriteGetScaledFontMetrics(
	IDWriteFontFace* fontFace, 
	FLOAT pixelSize,
	BOOL scaleForPixelHeight,

	FLOAT* height,
	FLOAT* ascent,
	FLOAT* descent,
	FLOAT* lineGap,
	FLOAT* xHeight,
	FLOAT* emSize,
	FLOAT* pixelsPerDesignUnits) {

	DWRITE_FONT_METRICS fontMetrics;
	fontFace->GetMetrics(&fontMetrics);

	if (scaleForPixelHeight) {
		UINT16 heightInDesignUnits = fontMetrics.ascent + fontMetrics.descent;

		*emSize = pixelSize*fontMetrics.designUnitsPerEm/heightInDesignUnits;
		*pixelsPerDesignUnits = pixelSize/heightInDesignUnits;
		*height = pixelSize;
	}
	else {
		*emSize = (96.f/72.f)*pixelSize;
		*pixelsPerDesignUnits = *emSize/fontMetrics.designUnitsPerEm;
		*height = *pixelsPerDesignUnits*(fontMetrics.ascent + fontMetrics.descent);
	}

	*ascent = *pixelsPerDesignUnits*fontMetrics.ascent;
	*descent = *pixelsPerDesignUnits*fontMetrics.descent;
	*lineGap = *pixelsPerDesignUnits*fontMetrics.lineGap;
	*xHeight = *pixelsPerDesignUnits*fontMetrics.xHeight;
}

UINT16 DWriteGetGlyphIndex(IDWriteFontFace* face, UINT32 codepoint) {
	UINT16 glyphId;
	HRESULT hr = face->GetGlyphIndices(&codepoint, 1, &glyphId);
	ASSERT_HR(hr);

	return glyphId;
}

HDC DWriteRasterizeGlyph(IDWriteFontFace* face, float32 emSize, float32 ascent, UINT16 glyphId, RECT* boundingBox) {
	// TODO: switch to IDWriteGlyphRunAnalysis::CreateAlphaTexture ?
	//       see here: https://github.com/google/skia/blob/main/src/ports/SkScalerContext_win_dw.cpp

	COLORREF background = RGB(0, 0, 0);
	COLORREF foreground = RGB(255, 255, 255);
	HDC dc = dwrite.renderTarget->GetMemoryDC();
	{
		HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
		SetDCPenColor(dc, background);
		SelectObject(dc, GetStockObject(DC_BRUSH));
		SetDCBrushColor(dc, background);
		Rectangle(dc, 0, 0, 256, 256);
		SelectObject(dc, original);
	}
	
	DWRITE_GLYPH_RUN glyphRun = {};
	glyphRun.fontFace = face;
	glyphRun.fontEmSize = emSize;
	glyphRun.glyphCount = 1;
	glyphRun.glyphIndices = &glyphId;

	// TODO: lhecker warns against this
	//       >> don't set your baselineOriginY to the upper most bound. 
	//       >> that's the ascentOriginY if anything
    //       >> I do recommend using the baseline as the base for your glyph coordinates, 
    //       >> because glyphs can be arbitrarily tall and that makes it easier to reason about it 
	HRESULT hr = dwrite.renderTarget->DrawGlyphRun(
		0, 
		ascent,
		DWRITE_MEASURING_MODE_NATURAL, 
		&glyphRun, 
		dwrite.renderingParams, 
		foreground, 
		boundingBox
	);
	ASSERT_HR(hr);

	return dc;
}

void DWriteCopyToAtlas(HDC dc, int32 outPitch, byte* outData, RECT boundingBox) {

	HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
	DIBSECTION dib = {};
	GetObject(bitmap, sizeof(dib), &dib);
	
	int32 inPitch = dib.dsBm.bmWidthBytes;
	byte* inData = (byte*)dib.dsBm.bmBits;
	for (int32 inY = MAX(0, boundingBox.top), outY = 0; 
		 inY < boundingBox.bottom; 
		 inY++, outY++) {

		for (int32 inX = MAX(0, boundingBox.left), outX = 0; 
			 inX < boundingBox.right; 
			 inX++, outX++) {

			outData[outPitch*outY + outX] = inData[inPitch*inY + 4*inX];
		}
	}
}

//------------------------------------
#if FONT_BAKED

BakedFont DWriteBakeFont(
	IDWriteFontFace* fontFace,
	FLOAT pixelSize,
	BOOL scaleForPixelHeight,
	UINT32* codepoints, 
	UINT32 codepointCount,
	BakedAtlasBitmap* atlas) {

	BakedFont font;
	font.firstChar = 1;
	font.lastChar = codepointCount;

	FLOAT emSize, pixelsPerDesignUnits;
	DWriteGetScaledFontMetrics(fontFace, pixelSize, scaleForPixelHeight,
		&font.height,
		&font.ascent,
		&font.descent,
		&font.lineGap,
		&font.xHeight,
		&emSize,
		&pixelsPerDesignUnits);

	HRESULT hr;

	for (uint32 i = 0; i < codepointCount; i ++) {
		UINT16 glyphId = DWriteGetGlyphIndex(fontFace, codepoints[i]);

		RECT boundingBox;
		HDC dc = DWriteRasterizeGlyph(fontFace, emSize, font.ascent, glyphId, &boundingBox);

		int32 glyphHeight = boundingBox.bottom - boundingBox.top;
		int32 glyphWidth = boundingBox.right - boundingBox.left;

		if (atlas->x + glyphWidth + 1 >= atlas->width)
			atlas->y = atlas->bottom_y, atlas->x = 1;

		byte* out_data = atlas->data + atlas->x + atlas->width*(atlas->y);
		DWriteCopyToAtlas(dc, atlas->width, out_data, boundingBox);

		DWRITE_GLYPH_METRICS glyphMetrics;
		hr = fontFace->GetDesignGlyphMetrics(
			&glyphId,
			1, 
			&glyphMetrics, 
			FALSE);
		ASSERT_HR(hr);

		font.glyphs[i].xoff = (float32)boundingBox.left;
		font.glyphs[i].yoff = (float32)boundingBox.top - font.ascent;
		font.glyphs[i].x0 = (int16)atlas->x;
		font.glyphs[i].y0 = (int16)atlas->y;
		font.glyphs[i].x1 = (int16)atlas->x + (int16)glyphWidth;
		font.glyphs[i].y1 = (int16)atlas->y + (int16)glyphHeight;
		font.glyphs[i].xadvance = pixelsPerDesignUnits*glyphMetrics.advanceWidth;

		atlas->x += (int16)glyphWidth + 1;
		if (atlas->y + glyphHeight + 1 > atlas->bottom_y)
			atlas->bottom_y = atlas->y + (int16)glyphHeight + 1;

	}

	return font;
}

BakedFont DWriteBakeFont(
	IDWriteFontFace* fontFace,
	FLOAT pixelSize,
	BOOL scaleForPixelHeight, 
	BakedAtlasBitmap* atlas) {

	BakedFont font;
	font.firstChar = 32;
	font.lastChar = 127;

	FLOAT emSize, pixelsPerDesignUnits;
	DWriteGetScaledFontMetrics(fontFace, pixelSize, scaleForPixelHeight,
		&font.height,
		&font.ascent,
		&font.descent,
		&font.lineGap,
		&font.xHeight,
		&emSize,
		&pixelsPerDesignUnits);

	HRESULT hr;

	for (uint32 i = 0; i < 96; i ++) {
		UINT16 glyphId = DWriteGetGlyphIndex(fontFace, 32 + i);

		RECT boundingBox;
		HDC dc = DWriteRasterizeGlyph(fontFace, emSize, font.ascent, glyphId, &boundingBox);

		int32 glyphHeight = boundingBox.bottom - boundingBox.top;
		int32 glyphWidth = boundingBox.right - boundingBox.left;

		if (atlas->x + glyphWidth + 1 >= atlas->width)
			atlas->y = atlas->bottom_y, atlas->x = 1;

		byte* out_data = atlas->data + atlas->x + atlas->width*(atlas->y);
		DWriteCopyToAtlas(dc, atlas->width, out_data, boundingBox);

		DWRITE_GLYPH_METRICS glyphMetrics;
		hr = fontFace->GetDesignGlyphMetrics(
			&glyphId,
			1, 
			&glyphMetrics, 
			FALSE);
		ASSERT_HR(hr);

		font.glyphs[i].xoff = (float32)boundingBox.left;
		font.glyphs[i].yoff = (float32)boundingBox.top - font.ascent;
		font.glyphs[i].x0 = (int16)atlas->x;
		font.glyphs[i].y0 = (int16)atlas->y;
		font.glyphs[i].x1 = (int16)atlas->x + (int16)glyphWidth;
		font.glyphs[i].y1 = (int16)atlas->y + (int16)glyphHeight;
		font.glyphs[i].xadvance = pixelsPerDesignUnits*glyphMetrics.advanceWidth;

		atlas->x += (int16)glyphWidth + 1;
		if (atlas->y + glyphHeight + 1 > atlas->bottom_y)
			atlas->bottom_y = atlas->y + (int16)glyphHeight + 1;

	}

	return font;
}

#endif

//------------------------------------------------

#if FONT_CACHED

struct DWriteScaledFont {
	IDWriteFontFace *face;

	float32 emSize;
	float32 pixelsPerDesignUnits;

	float32 height;
	float32 ascent;
	float32 descent;
	float32 lineGap;
	float32 xHeight;
};

DWriteScaledFont DWriteGetScaledFont(IDWriteFontFace* face, float32 size, bool scaleForPixelHeight) {
	DWriteScaledFont font;
	font.face = face;
	DWriteGetScaledFontMetrics(face, size, scaleForPixelHeight,
		&font.height,
		&font.ascent,
		&font.descent,
		&font.lineGap,
		&font.xHeight,
		&font.emSize,
		&font.pixelsPerDesignUnits);	

	return font;
}

void DWriteBakeGlyph(DWriteScaledFont* font, 
					 UINT16 glyphId, 
					 CachedGlyph* glyph,
					 byte* outPixelBuffer,
					 RegionNode* atlasRoot, 
					 FixedSize* regionNodeAllocator) {

	RECT boundingBox;
	HDC dc = DWriteRasterizeGlyph(font->face, font->emSize, font->ascent, glyphId, &boundingBox);
	
	glyph->xoff = (float32)boundingBox.left;
	glyph->yoff = (float32)boundingBox.top - font->ascent;
	int32 glyphHeight = boundingBox.bottom - boundingBox.top;
	int32 glyphWidth = boundingBox.right - boundingBox.left;

	if (glyphWidth && glyphHeight) {
		RegionNode* node = RegionAlloc(atlasRoot, regionNodeAllocator, {glyphWidth + 1, glyphHeight + 1});
		if (node == NULL)
			return;

		byte* out_data = outPixelBuffer + node->x + 1 + atlasRoot->width*(node->y + 1);
		DWriteCopyToAtlas(dc, atlasRoot->width, out_data, boundingBox);

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

	DWRITE_GLYPH_METRICS glyphMetrics;
	HRESULT hr = font->face->GetDesignGlyphMetrics(
		&glyphId,
		1, 
		&glyphMetrics, 
		FALSE);
	ASSERT_HR(hr);

	glyph->xadvance = font->pixelsPerDesignUnits*glyphMetrics.advanceWidth;
}

#endif