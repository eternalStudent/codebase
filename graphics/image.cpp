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

struct Color3 {
	float32 r, g, b;
};

inline Color3 operator+(Color3 c0, Color3 c1) {
	return {c0.r + c1.r, c0.g + c1.g, c0.b + c1.b};
}

inline Color3 operator-(Color3 c0, Color3 c1) {
	return {c0.r - c1.r, c0.g - c1.g, c0.b - c1.b};
}

inline Color3 operator+(Color3 c0, float32 a) {
	return {c0.r + a, c0.g + a, c0.b + a};
}

inline Color3 operator-(Color3 c0, float32 a) {
	return {c0.r - a, c0.g - a, c0.b - a};
}

inline Color3 operator*(float32 a, Color3 c) {
	return {a*c.r, a*c.g, a*c.b};
}

struct Color {
	union {
		Color3 rgb;
		struct {float32 r, g, b;};
	};
	float32 a;
};

#define COLOR_BLACK 	Color{0, 0, 0, 1}
#define COLOR_BLUE		Color{0, 0, 1, 1}
#define COLOR_GREEN		Color{0, 1, 0, 1}
#define COLOR_CYAN		Color{0, 1, 1, 1}
#define COLOR_RED		Color{1, 0, 0, 1}
#define COLOR_YELLOW	Color{1, 0, 1, 1}
#define COLOR_MAGENTA	Color{1, 1, 0, 1}
#define COLOR_WHITE		Color{1, 1, 1, 1}

