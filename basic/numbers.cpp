#include <stdint.h>
#include <stddef.h>

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)<(y)?(y):(x))
#define ABS(x)   ((x)<0? -(x):(x))
#define SGN(x)	 ((0<(x))-((x)<0))

// Unsigned
//----------

typedef unsigned char byte;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define MAX_UINT16 0xffffu
#define MAX_UINT32 0xffffffffu
#define MAX_UINT64 0xffffffffffffffffull

inline uint64 min(uint64 a, uint64 b) {
	return a < b ? a : b;
}

inline uint64 max(uint64 a, uint64 b) {
	return a < b ? b : a;
}

uint64 log10(uint64 n) {
	if (n < 10000000000) {
		// 1..10
		return n < 100000
			? n < 1000
				? n < 100
					? n < 10 ? 1 : 2
					: 3
			
				: n < 10000 ? 4 : 5

			: n < 100000000
				? n < 10000000
					? n < 1000000 ? 6 : 7
					: 8

				: n < 1000000000 ? 9 : 10;
	}
	else {
		// 11..19
		return n < 1000000000000000
			? n < 10000000000000
				? n < 1000000000000
					? n < 100000000000 ? 11 : 12
					: 13
			
				: n < 100000000000000 ? 14 : 15

			: n < 100000000000000000
				? n < 10000000000000000 ? 16 : 17

				: n < 1000000000000000000 ? 18 : 19;
	}
}

uint64 power(uint32 base, uint32 exponent) {
	uint64 result = 1;
	uint64 base64 = base;

	while (exponent > 0) {
		if (exponent & 1) {
			result *= base64;
		}
		base64 *= base64;
		exponent >>= 1;
	}

	return result;
}

// Signed
//--------

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef ptrdiff_t ssize;

#define MIN_INT16 (-0x7fff - 1)
#define MIN_INT32 (-0x7fffffff - 1)
#define MIN_INT64 0x8000000000000000ll

#define MAX_INT16 0x7fff
#define MAX_INT32 0x7fffffff
#define MAX_INT64 0x7fffffffffffffffll

inline int64 min(int64 a, int64 b) {
	return a < b ? a : b;
}

inline int64 max(int64 a, int64 b) {
	return a < b ? b : a;
}

inline int64 sign(int64 i) {
	return (0 < i) - (i < 0);
}

// float32
//------------

typedef float float32;

inline int64 itrunc(float32 x) {
	return (int64)x;
}

inline float32 min(float32 x, float32 y) {
	return x < y ? x : y;
}

inline float32 max(float32 x, float32 y) {
	return x < y ? y : x;
}

#ifndef _MATH_ALREADY_DEFINED_

inline float32 abs(float32 x) {
	return x < 0 ? -x : x;
}

#endif

inline int64 sign(float32 x) {
	return (0 < x) - (x < 0);
}

inline float32 exp2(int32 exp) {
	uint32 exp_part = (uint32)(exp+127);
	union {uint32 u; float32 f;} cvt;
	cvt.u = exp_part << 23;
	return cvt.f;
}

inline float32 saturate(float32 value) {
	if (value < 0)
		return 0;

	if (value > 1)
		return 1;
	
	return value;
}

#ifndef _MATH_ALREADY_DEFINED_

inline float32 sqrt(float32 value) {
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
} 

inline float32 floor(float32 value) {
	return _mm_cvtss_f32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(value)));
}

inline float32 ceil(float32 value) {
	return _mm_cvtss_f32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(value)));
}

// a*b + c
inline float32 fmadd(float32 a, float32 b, float32 c) {
	return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
}

inline float32 round(float32 value) {
	return _mm_cvtss_f32(_mm_round_ss(_mm_setzero_ps(), _mm_set_ss(value),
						_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ));
}

#endif

// float64
//------------

typedef double float64;

inline int64 itrunc(float64 x) {
	return (int64)x;
}

inline float64 min(float64 x, float64 y) {
	return x < y ? x : y;
}

inline float64 max(float64 x, float64 y) {
	return x < y ? y : x;
}

#ifndef _MATH_ALREADY_DEFINED_

inline float64 abs(float64 x) {
	return x < 0 ? -x : x;
}

#endif

inline int64 sign(float64 x) {
	return (0 < x) - (x < 0);
}

float64 pow10(int64 n) {
	static float64 tab[] = {
		1.0e0, 1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e6, 1.0e7, 1.0e8, 1.0e9,
		1.0e10,1.0e11,1.0e12,1.0e13,1.0e14,1.0e15,1.0e16,1.0e17,1.0e18,1.0e19,
		1.0e20,1.0e21,1.0e22,1.0e23,1.0e24,1.0e25,1.0e26,1.0e27,1.0e28,1.0e29,
		1.0e30,1.0e31,1.0e32,1.0e33,1.0e34,1.0e35,1.0e36,1.0e37,1.0e38,1.0e39,
		1.0e40,1.0e41,1.0e42,1.0e43,1.0e44,1.0e45,1.0e46,1.0e47,1.0e48,1.0e49,
		1.0e50,1.0e51,1.0e52,1.0e53,1.0e54,1.0e55,1.0e56,1.0e57,1.0e58,1.0e59,
		1.0e60,1.0e61,1.0e62,1.0e63,1.0e64,1.0e65,1.0e66,1.0e67,1.0e68,1.0e69,
	};
	int64 m;

	if (n < 0) {
		n = -n;
		if (n < (int64)(sizeof(tab)/sizeof(tab[0])))
			return 1/tab[n];
		m = n/2;
		return pow10(-m)*pow10(m-n);
	}
	if (n < (int64)(sizeof(tab)/sizeof(tab[0])))
		return tab[n];
	m = n/2;
	return pow10(m)*pow10(n-m);
}

inline float64 round(float64 value) {
	return _mm_cvtsd_f64(_mm_round_sd(_mm_setzero_pd(), _mm_set_sd(value),
						_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ));
}

float64 power(float64 base, uint32 exponent) {
	float64 result = 1;

	while (exponent > 0) {
		if (exponent & 1) {
			result *= base;
		}
		base *= base;
		exponent >>= 1;
	}

	return result;
}
