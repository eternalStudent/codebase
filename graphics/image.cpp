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
	float32 r, g, b;
	if (hh <  2) {
		r = l + d;
		g = l - d*(1 - hh); 
		b = l - d;
	}

	else if (hh <  4) {
		r = l - d*(-3 + hh);
		g = l + d;
		b = l - d;
	}

	else if (hh <  6) {
		r = l - d;
		g = l + d;
		b = l - d*(5 - hh); 
	}

	else if (hh <  8) {
		r = l - d;
		g = l - d*(-7 + hh);
		b = l + d;
	}

	else if (hh < 10) {
		r = l - d*(9 - hh);
		g = l - d;
		b = l + d; 
	}

	else {
		r = l + d;
		g = l - d; 
		b = l - d*(-11 + hh);
	}

	return {saturate(r), saturate(g), saturate(b), a};
}

void ToHSL(Color color, float32* h, float32* s, float32* l) {
	float32 maxC = max(max(color.r, color.g), color.b);
	float32 minC = min(min(color.r, color.g), color.b);

	*l = (maxC + minC) / 2;

	if (maxC == minC) {
		*h = 0;
		*s = 0;
		return;
	}

	float32 d = maxC - minC;
	*s = *l > 0.5f 
		? d / (2 - maxC - minC) 
		: d / (maxC + minC);

	if (maxC == color.r) { 
		*h = (color.g - color.b) / d + (color.g < color.b ? 6 : 0); 
	}
	else if (maxC == color.g) { 
		*h = (color.b - color.r) / d + 2; 
	}
	else {
		*h = (color.r - color.g) / d + 4; 
	} 
	*h /= 6;
}

Color HSL(uint32 h, uint32 s, uint32 l) {
	return HSL(h/360.f, s/100.f, l/100.f);
}

Color sRGB(uint32 r, uint32 g, uint32 b) {
	return {saturate(r/255.f), saturate(g/255.f), saturate(b/255.f), 1};
}