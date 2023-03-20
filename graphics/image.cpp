struct Image {
	union {
		Dimensions2i dimensions;
		struct {int32 width, height;};
	};
	int32 channels;
	byte* data;
};

#include "bitmap.cpp"
#include "png.cpp"

struct Color {
	float32 r, g, b, a;
};