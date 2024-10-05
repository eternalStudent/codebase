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

union Color {
	struct {
		union {
			Color3 rgb;
			struct {float32 r, g, b;};
		};
		float32 a;
	};
	float32 e[4];
};

inline Color operator+(Color c0, Color c1) {
	return {c0.r + c1.r, c0.g + c1.g, c0.b + c1.b, c0.a + c1.a};
}

inline Color operator-(Color c0, Color c1) {
	return {c0.r - c1.r, c0.g - c1.g, c0.b - c1.b, c0.a - c1.a};
}

inline Color operator*(float32 a, Color c) {
	return {a*c.r, a*c.g, a*c.b, a*c.a};
}

#define COLOR_BLACK 	Color{0, 0, 0, 1}
#define COLOR_BLUE		Color{0, 0, 1, 1}
#define COLOR_GREEN		Color{0, 1, 0, 1}
#define COLOR_CYAN		Color{0, 1, 1, 1}
#define COLOR_RED		Color{1, 0, 0, 1}
#define COLOR_YELLOW	Color{1, 0, 1, 1}
#define COLOR_MAGENTA	Color{1, 1, 0, 1}
#define COLOR_WHITE		Color{1, 1, 1, 1}

Color HSL(float32 h, float32 s, float32 l, float32 a = 1) {

	float32 d = s*min(l, 1 - l);
	if (d == 0) 
		return {l, l, l, a};

	float32 hh = 12*h;
	// TODO: fmod, and saturate
	if (hh <  2) return {l + d, 
						 l - d*(1 - hh), 
						 l - d, 
						 a};

	if (hh <  4) return {l - d*(-3 + hh),
						 l + d,
						 l - d,
						 a};

	if (hh <  6) return {l - d,
						 l + d,
						 l - d*(5 - hh), 
						 a};

	if (hh <  8) return {l - d,
						 l - d*(-7 + hh),
						 l + d, 
						 a};

	if (hh < 10) return {l - d*(9 - hh),
						 l - d,
						 l + d, 
						 a};

	             return {l + d,
	             		 l - d, 
	             		 l - d*(-11 + hh), 
	             		 a};
}

