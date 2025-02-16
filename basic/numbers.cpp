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
		// 11..20
		return n < 1000000000000000ull
			? n < 10000000000000ull
				? n < 1000000000000ull
					? n < 100000000000ull ? 11 : 12
					: 13
			
				: n < 100000000000000ull ? 14 : 15

			: n < 1000000000000000000ull
				? n < 100000000000000000ull
					? n < 10000000000000000ull ? 16 : 17
					: 18

				: n < 10000000000000000000ull ? 19 : 20;
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

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
typedef unsigned long long carry_t;
#elif defined(COMPILER_MSVC)
typedef unsigned char carry_t;
#endif

inline uint64 uadd(carry_t carry, uint64 a, uint64 b, carry_t* high_c) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return __builtin_addcll(a, b, carry, high_c);
#elif defined(COMPILER_MSVC)
	uint64 c;
	*high_c = _addcarry_u64(carry, a, b, &c);
	return c;
#endif
}

inline uint64 ushiftright(uint64 high, uint64 low, byte shift, uint64* high_shifted) {
	*high_shifted = high >> shift;
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return (high << (64 - shift)) | (low >> shift);
#elif defined(COMPILER_MSVC)
	return __shiftright128(low, high, shift);
#endif
}

inline uint64 ushiftleft(uint64 high, uint64 low, byte shift, uint64* high_shifted) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	*high_shifted = (high << shift) | (low >> (64 - shift));
#elif defined(COMPILER_MSVC)
	*high_shifted =  __shiftleft128(low, high, shift);
#endif
	return low << shift;
}

// NOTE: don't use if divisor is known at compile time
inline uint64 udiv(uint64 high, uint64 low, uint64 divisor, uint64* remainder) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	uint64 quotient;
	uint64 rem;

	asm("divq %4"
		: "=a"(quotient), "=d"(rem)
		: "d"(high), "a"(low), "r"(divisor));

	*remainder = rem;

	return quotient;
#elif defined(COMPILER_MSVC)
	return _udiv128(high, low, divisor, remainder);
#endif
}

// NOTE: don't use if divisor is known at compile time
inline uint64 udiv(uint64 high, uint64 low, uint64 divisor, uint64* remainder, uint64* high_quotient) {
	uint64 high_r = high % divisor;
	*high_quotient = high / divisor;
	return udiv(high_r, low, divisor, remainder);
}

inline uint64 udiv5(uint64 high, uint64 low, uint64* remainder, uint64* high_quotient) {
	// Use 2^64 = 1 mod 5 to compute remainder:
	// (high * 2^64 + low) 
	// = high + low mod 5
	// = high + low + carry mod 5 (the high + low might overflow, carry = 2^64 = 1 mod 5)
	//
	// Write high * 2^64 + low - remainder = highSub * 2^64 + lowSub
	// Note that highSub + lowSub = highSub * 2^64 + lowSub = 0 mod 5
	// Compute low bits of the divide:
	// lowSub * (2^66+1)/5 (division is exact!)
	// = lowSub/5 * 2^66 + lowSub/5
	// = lowSub*4/5 * 2^64 + lowSub/5
	// = highSub/5 * 2^64 + lowSub/5 mod 2^64 (lowSub * 4 = -lowSub = highSub mod 5)
	// = (highSub * 2^64 + lowSub)/5 mod 2^64
	// = flowor((high * 2^64 + low)/5) mod 2^64

	carry_t carry;
	uint64 merged = uadd(0, high, low, &carry);
	merged = uadd(carry, merged, 0, &carry);
	uint64 rem = merged % 5;
	uint64 lowSub = low - rem;
	uint64 low_quotient = lowSub * 14757395258967641293ull;
	*high_quotient = high / 5;

	*remainder = rem;

	return low_quotient;
}

inline uint64 udiv10(uint64 high, uint64 low, uint64* remainder, uint64* quotient_high) {
    uint64 high2;
	uint64 low2 = ushiftright(high, low, 1, &high2);

	carry_t carry;
	uint64 merged = uadd(0, high2, low2, &carry);
	merged = uadd(carry, merged, 0, &carry);

    uint64 rem = merged % 5;
    uint64 lowSub = low2 - rem;
    uint64 low_quotient = lowSub * 14757395258967641293ull;

    *remainder = low - 2*lowSub;
    *quotient_high = high / 10;

    return low_quotient;
}

inline uint64 umul(uint64 a, uint64 b, uint64 *high_c) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	__uint128_t product = (__uint128_t)a * (__uint128_t)b;
	*high_c = product >> 64;
	return product;
#elif defined(COMPILER_MSVC)
	return _umul128(a, b, high_c);
#endif
}

inline uint64 umul(uint64 high_a, uint64 low_a, uint64 b, uint64 *high_c) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	__uint128_t a = ((__uint128_t)high_a << 64) | (__uint128_t)low_a;
	__uint128_t c = (__uint128_t)a * (__uint128_t)b;
	*high_c = c >> 64;
	return c;
#elif defined(COMPILER_MSVC)
	uint64 low_c = _umul128(low_a, b, high_c);
	*high_c += high_a*b;
	return low_c;
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

inline int32 min(int32 a, int32 b) {
	return a < b ? a : b;
}

inline int32 max(int32 a, int32 b) {
	return a < b ? b : a;
}

inline int32 sign(int32 i) {
	return (0 < i) - (i < 0);
}

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
