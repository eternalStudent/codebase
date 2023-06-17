typedef unsigned char byte;

#include <stdint.h>
#include <stddef.h>

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef ptrdiff_t ssize;

#define MAX_INT16 0x7fff
#define MAX_INT32 0x7fffffff
#define MAX_INT64 0x7fffffffffffffff

#define MIN_INT16 0x8000
#define MIN_INT32 0x80000000
#define MIN_INT64 0x8000000000000000

#define MAX_UINT16 0xffff
#define MAX_UINT32 0xffffffff
#define MAX_UINT64 0xffffffffffffffff

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)<(y)?(y):(x))
#define ABS(x)   ((x)<0? -(x):(x))
#define SGN(x)	 ((0<(x))-((x)<0))

inline int32 Min(int32 a, int32 b) {
	return a < b ? a : b;
}

inline int32 Max(int32 a, int32 b) {
	return a < b ? b : a;
}

inline int32 Abs(int32 i) {
	return i < 0 ? -i : i;
}

inline int32 Sign(int32 i) {
	return (0 < i) - (i < 0);
}

inline float32 Min(float32 x, float32 y) {
	return x < y ? x : y;
}

inline float32 Max(float32 x, float32 y) {
	return x < y ? y : x;
}

inline float32 Abs(float32 x) {
	return x < 0 ? -x : x;
}

inline int32 Sign(float32 x) {
	return (0 < x) - (x < 0);
}

inline float32 Exp2(int32 exp) {
	uint32 exp_part = (uint32)(exp+127);
	union {uint32 u; float32 f;} data;
	data.u = exp_part << 23;
	return data.f;
}

int32 Log10(uint64 n) {
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

float64 Pow10(int32 n) {
	static float64 tab[] = {
		1.0e0, 1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e6, 1.0e7, 1.0e8, 1.0e9,
		1.0e10,1.0e11,1.0e12,1.0e13,1.0e14,1.0e15,1.0e16,1.0e17,1.0e18,1.0e19,
		1.0e20,1.0e21,1.0e22,1.0e23,1.0e24,1.0e25,1.0e26,1.0e27,1.0e28,1.0e29,
		1.0e30,1.0e31,1.0e32,1.0e33,1.0e34,1.0e35,1.0e36,1.0e37,1.0e38,1.0e39,
		1.0e40,1.0e41,1.0e42,1.0e43,1.0e44,1.0e45,1.0e46,1.0e47,1.0e48,1.0e49,
		1.0e50,1.0e51,1.0e52,1.0e53,1.0e54,1.0e55,1.0e56,1.0e57,1.0e58,1.0e59,
		1.0e60,1.0e61,1.0e62,1.0e63,1.0e64,1.0e65,1.0e66,1.0e67,1.0e68,1.0e69,
	};
	int32 m;

	if (n < 0) {
		n = -n;
		if (n < (int32)(sizeof(tab)/sizeof(tab[0])))
			return 1/tab[n];
		m = n/2;
		return Pow10(-m)*Pow10(m-n);
	}
	if (n < (int32)(sizeof(tab)/sizeof(tab[0])))
		return tab[n];
	m = n/2;
	return Pow10(m)*Pow10(n-m);
}

uint64 power(uint32 base, uint32 exponent) {
	uint64 result = 1;

	while (exponent > 0) {
		if (exponent & 1) {
			result *= base;
		}
		base *= base;
		exponent >>= 1;
	}

	return result;
}
