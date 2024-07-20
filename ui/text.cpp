enum TextEncoding {
	TE_UTF8,
	TE_UTF16,
	TE_UTF32
};

enum FontStyle {
	FS_Normal,
	FS_Italic,
	FS_Oblique
};

struct FontFace {
	String familyName;
	int32 weight;
	FontStyle style;
};

struct FontFace16 {
	String16 familyName;
	int32 weight;
	FontStyle style;
};

enum TextDecoration {
	TD_None,
	TD_UnderLine,
	TD_StrikeThrough,
	TD_OverLine
};

struct TextFormat {
	FontFace font;
	float32 size;
	TextDecoration decoration;
	Color color;
	Color highlight;
};

struct TextRun {
	uint32 start;
	uint32 length;
	TextFormat format;
};

enum TextWrapping {
	TW_NoWrap,
	TW_Wrap
};

enum TextAlignment {
	TA_Start,
	TA_End,
	TA_Center,
	TA_Justified
};

enum TextDirection {
	TD_Ltr,
	TD_Rtl
};

struct TextSelection {
	uint32 start;
	uint32 length;
	Color color;
};