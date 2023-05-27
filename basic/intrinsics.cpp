// TODO: this file should be split into numbers and bits

#include <emmintrin.h>
#include <memory.h>

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#  include <intrin.h>
#else
#  include <smmintrin.h>
#endif

int32 HighBit(uint32 value, int32 onZero) {
	if (value == 0) return onZero;
#if defined(__clang__) || defined(__GNUC__)
	return 31-__builtin_clz(value);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse(&index, value);
	return (int32)index;
#else
	int32 n = 0;
	if (value >= 0x10000) {n += 16; value >>= 16;}
	if (value >= 0x00100) {n +=  8; value >>=  8;}
	if (value >= 0x00010) {n +=  4; value >>=  4;}
	if (value >= 0x00004) {n +=  2; value >>=  2;}
	if (value >= 0x00002) {n +=  1;}
	return n;
#endif
}

int32 HighBit64(uint64 value, int32 onZero) {
	if (value == 0) return onZero;
#if defined(__clang__) || defined(__GNUC__)
	return 63-__builtin_clzll(value);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse64(&index, value);
	return (int32)index;
#else
	// TODO
#endif
}

int32 LowBit(uint32 value) {
	if (value == 0) return 32;
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctz(value);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanForward(&index, value);
	return (int32)index;
#else
	int32 n = 0;
	if (!(value & 0x0000FFFF)) {n += 16; value >>= 16;}
	if (!(value & 0x000000FF)) {n +=  8; value >>=  8;}
	if (!(value & 0x0000000F)) {n +=  4; value >>=  4;}
	if (!(value & 0x00000003)) {n +=  2; value >>=  2;}
	if (!(value & 0x00000001)) {n +=  1;}
	return n;
#endif
}

int32 LowBit64(uint64 value) {
	if (value == 0) return 64;
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctzll(value);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanForward64(&index, value);
	return (int32)index;
#else
	// TODO
#endif
}

int32 BitCount(uint32 value) {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcount(value);
#elif defined(_MSC_VER)
	return __popcnt(value);
#else
	value = (value & 0x55555555) + ((value >> 1) & 0x55555555); // max 2
	value = (value & 0x33333333) + ((value >> 2) & 0x33333333); // max 4
	value = (value + (value >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
	value = (value + (value >> 8)); // max 16 per 8 bits
	value = (value + (value >> 16)); // max 32 per 8 bits
	return value & 0xff;
#endif
}

int64 BitCount64(uint64 value) {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcountll(value);
#elif defined(_MSC_VER)
	return __popcnt64(value);
#else
	// TODO
#endif
}

// TODO: these should be moved to numbers

float32 sqrt(float32 value) {
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
} 

float32 floor(float32 value) {
	return _mm_cvtss_f32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(value)));
}

float32 ceil(float32 value) {
	return _mm_cvtss_f32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(value)));
}

float32 fmadd(float32 a, float32 b, float32 c) {
	return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
}

float32 round(float32 value) {
	return _mm_cvtss_f32(_mm_round_ss(_mm_setzero_ps(), _mm_set_ss(value),
						_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ));
}

int32 ToInt(float32 value) {
	return _mm_cvtss_si32(_mm_set_ss(value));
}