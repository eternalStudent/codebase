struct Image {
	union {
		Dimensions2i dimensions;
		struct { int32 width, height;};
	};
	int32 channels;
	byte* data;
};

#include "bitmap.cpp"
#include "png.cpp"

#if !defined(IMAGE_BITMAP)
#  define IMAGE_BITMAP 	0
#endif
#define IMAGE_PNG		3

Image LoadImage(Arena* arena, byte* data, uint32 format) {
	if (format == IMAGE_BITMAP)
		return BMPLoadImage(arena, data);
	if (format == IMAGE_PNG)
		return PNGLoadImage(arena, data);
	return {};
}

typedef Point4 Color;

// TODO: I should have separate palette files
#define RGBA_BLACK          0xff000000
#define RGBA_WHITE          0xffffffff
#define RGBA_LIGHTGREY      0xff7e7872
#define RGBA_GREY           0xff5e574f
#define RGBA_DARKGREY       0xff413830
#define RGBA_BLUE           0xffcc9966
#define RGBA_GREEN          0xff94c799
#define RGBA_ORANGE         0xff58aef9
#define RGBA_LILAC          0xffc695c6
#define RGBA_RED            0xff665fec
#define RGBA_TURQUOISE      0xffefd867

inline Color RgbaToColor(uint32 rgba) {
	union {
		uint32 rgba;
		struct {byte r, g, b, a;};
	} color;
	color.rgba = rgba;
	return {color.r/255.0f,
			color.g/255.0f,
			color.b/255.0f,
			color.a/255.0f};
}