#include "../approx/approx.cpp"

struct complex {
	float32 re;
	float32 im;
};

inline complex operator+(complex z0, complex z1) {
	return {z0.re + z1.re, z0.im + z1.im};
}

inline complex operator-(complex z0, complex z1) {
	return {z0.re - z1.re, z0.im - z1.im};
}

inline complex operator*(complex z0, complex z1) {
	return {z0.re*z1.re - z0.im*z1.im, z0.re*z1.im + z0.im*z1.re};
}

inline float32 abs(complex z) {
	return sqrt(z.re*z.re + z.im*z.im);
}

inline complex exp(complex z) {
	float32 x = z.re;
	float32 t = z.im;
	float32 r = (float32)exp(x);
	float32 sint, cost;
	sincos(t, &sint, &cost);
	return {r*cost, r*sint};
}

inline complex polarpi(float32 t) {
	float32 sint, cost;
	sincospi(t, &sint, &cost);
	return {cost, sint};
}

complex power(complex base, uint32 exponent) {
	complex result = {1, 0};

	while (exponent > 0) {
		if (exponent & 1) {
			result = result * base;
		}
		base = base * base;
		exponent >>= 1;
	}

	return result;
}