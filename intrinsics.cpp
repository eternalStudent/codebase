#include <emmintrin.h>
#include <memory.h>

#ifdef _MSC_VER
#include <intrin.h>
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