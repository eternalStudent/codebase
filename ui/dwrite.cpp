#include <dwrite.h>

struct {
	IDWriteFactory* factory;
	IDWriteRenderingParams* renderingParams;
	IDWriteBitmapRenderTarget* renderTarget;
} dwrite;

void DWriteInit(UINT32 width, UINT32 height) {
	HRESULT hr;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite.factory);
	ASSERT_HR(hr);

	IDWriteRenderingParams* baseRenderingParams;
	hr = dwrite.factory->CreateRenderingParams(&baseRenderingParams);
	ASSERT_HR(hr);
	
	FLOAT gamma = baseRenderingParams->GetGamma();
	FLOAT enhanced_contrast = baseRenderingParams->GetEnhancedContrast();
	hr = dwrite.factory->CreateCustomRenderingParams(
		gamma,
		enhanced_contrast,
		0,
		DWRITE_PIXEL_GEOMETRY_RGB,
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

IDWriteFontFace* DWriteGetFontFace(const WCHAR* filePath) {
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

IDWriteFontFace* DWriteLoadDefaultFont() {
	WCHAR filePath[MAX_PATH] = {};
	LPWSTR ptr = filePath + GetEnvironmentVariableW(L"windir", (LPWSTR)filePath, MAX_PATH);
	COPY(L"\\Fonts\\consola.ttf", ptr);
	
	return DWriteGetFontFace(filePath);
}

BakedFont DWriteBakeFont(
	IDWriteFontFace* fontFace, 
	AtlasBitmap* atlas, 
	FLOAT pixelSize,
	BOOL scaleForPixelHeight,
	UINT32* codepoints, 
	UINT32 codepointCount, 
	int32 firstChar) {

	ASSERT(codepointCount <= 96);
	HRESULT hr;

	// get some font metrics
	DWRITE_FONT_METRICS fontMetrics;
	fontFace->GetMetrics(&fontMetrics);

	FLOAT emSize, height, pixelsPerDesignUnits;
	if (scaleForPixelHeight) {
		UINT16 heightInDesignUnits = fontMetrics.ascent + fontMetrics.descent;
		emSize = pixelSize*fontMetrics.designUnitsPerEm/heightInDesignUnits;
		pixelsPerDesignUnits = pixelSize/heightInDesignUnits;
		height = pixelSize;
	}
	else {
		emSize = (96.f/72.f)*pixelSize;
		pixelsPerDesignUnits = emSize/fontMetrics.designUnitsPerEm;
		height = pixelsPerDesignUnits*(fontMetrics.ascent + fontMetrics.descent);
	}
	FLOAT ascent = pixelsPerDesignUnits*fontMetrics.ascent;

	COLORREF background = RGB(0, 0, 0);
	COLORREF foreground = RGB(255, 255, 255);

	BakedFont font;
	font.firstChar = firstChar;
	font.lastChar = firstChar + codepointCount - 1;
	font.height = height;
	font.lineGap = pixelsPerDesignUnits*fontMetrics.lineGap;

	for (uint32 i = 0; i < codepointCount; i ++) {
		UINT16 glyphId;
		hr = fontFace->GetGlyphIndices(codepoints + i, 1, &glyphId);
		ASSERT_HR(hr);

		HDC dc = dwrite.renderTarget->GetMemoryDC();
		{
			HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
			SetDCPenColor(dc, background);
			SelectObject(dc, GetStockObject(DC_BRUSH));
			SetDCBrushColor(dc, background);
			Rectangle(dc, 0, 0, 256, 256);
			SelectObject(dc, original);
		}
		
		DWRITE_GLYPH_RUN glyphRun = {0};
		glyphRun.fontFace = fontFace;
		glyphRun.fontEmSize = emSize;
		glyphRun.glyphCount = 1;
		glyphRun.glyphIndices = &glyphId;
		RECT boundingBox = {0};

		hr = dwrite.renderTarget->DrawGlyphRun(
			0, 
			ascent,
			DWRITE_MEASURING_MODE_NATURAL, 
			&glyphRun, 
			dwrite.renderingParams, 
			foreground, 
			&boundingBox
		);
		ASSERT_HR(hr);

		int32 glyphHeight = boundingBox.bottom - boundingBox.top;
		int32 glyphWidth = boundingBox.right - boundingBox.left;

		if (atlas->x + glyphWidth + 1 >= atlas->pw)
			atlas->y = atlas->bottom_y, atlas->x = 1;
		
		// Get the Bitmap
		HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
		DIBSECTION dib = {0};
		GetObject(bitmap, sizeof(dib), &dib);
		
		// copy and convert to single channel
		{
			int32 in_pitch = dib.dsBm.bmWidthBytes;
			int32 out_pitch = atlas->pw;

			byte* in_data = (byte*)dib.dsBm.bmBits + in_pitch*(boundingBox.bottom - glyphHeight) + 4*boundingBox.left;
			byte* out_data = atlas->bitmap + atlas->x + out_pitch*(atlas->y);

			byte* in_line = in_data;
			byte* out_line = out_data;
			for (int32 y = 0; y < glyphHeight; y++) {
				byte* in_pixel = in_line;
				byte* out_pixel = out_line;
				for (int32 x = 0; x < glyphWidth; x++) {
					out_pixel[0] = in_pixel[0];
					in_pixel += 4;
					out_pixel++;
				}
				in_line += in_pitch;
				out_line += out_pitch;
			}
		}

		DWRITE_GLYPH_METRICS glyphMetrics;
		hr = fontFace->GetDesignGlyphMetrics(
			&glyphId,
			1, 
			&glyphMetrics, 
			FALSE);
		ASSERT_HR(hr);

		font.chardata[i].xoff = (float32)boundingBox.left;
		font.chardata[i].yoff = (float32)boundingBox.top - (int32)ascent;
		font.chardata[i].x0 = atlas->x;
		font.chardata[i].y0 = atlas->y;
		font.chardata[i].x1 = atlas->x + (int16)glyphWidth;
		font.chardata[i].y1 = atlas->y + (int16)glyphHeight;
		font.chardata[i].xadvance = pixelsPerDesignUnits*glyphMetrics.advanceWidth;

		atlas->x += (int16)glyphWidth + 1;
		if (atlas->y + glyphHeight + 1 > atlas->bottom_y)
			atlas->bottom_y = atlas->y + (int16)glyphHeight + 1;

	}

	return font;
}

BakedFont DWriteBakeFont(
	IDWriteFontFace* fontFace, 
	AtlasBitmap* atlas, 
	FLOAT pixelSize,
	BOOL scaleForPixelHeight) {

	static UINT32 codepoints[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5f,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7f,
	};

	return DWriteBakeFont(fontFace, atlas, pixelSize, scaleForPixelHeight, codepoints, 96, 32);
}

BakedFont DWriteLoadAndBakeDefaultFont(AtlasBitmap* atlas, FLOAT pixelSize, BOOL scaleForPixelHeight) {
	IDWriteFontFace* fontFace = DWriteLoadDefaultFont();
	BakedFont font = DWriteBakeFont(fontFace, atlas, pixelSize, scaleForPixelHeight);
	fontFace->Release();

	return font;
}
