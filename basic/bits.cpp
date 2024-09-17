inline int64 HighBit(uint32 value, int64 onZero) {
	if (value == 0) return onZero;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return 31 - __builtin_clz(value);
#elif defined(COMPILER_MSVC)
	unsigned long index;
	_BitScanReverse(&index, value);
	return (int64)index;
#endif
}

inline int64 HighBit(uint64 value, int64 onZero) {
	if (value == 0) return onZero;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return 63 - __builtin_clzll(value);
#elif defined(COMPILER_MSVC)
	unsigned long index;
	_BitScanReverse64(&index, value);
	return (int64)index;
#endif
}

inline int64 LowBit(uint32 value) {
	if (value == 0) return 32;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return __builtin_ctz(value);
#elif defined(COMPILER_MSVC)
	unsigned long index;
	_BitScanForward(&index, value);
	return (int64)index;
#endif
}

inline int64 LowBit(uint64 value) {
	if (value == 0) return 64;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return __builtin_ctzll(value);
#elif defined(COMPILER_MSVC)
	unsigned long index;
	_BitScanForward64(&index, value);
	return (int64)index;
#endif
}

inline int64 BitCount(uint32 value) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return __builtin_popcount(value);
#elif defined(COMPILER_MSVC)
	return __popcnt(value);
#endif
}

inline int64 BitCount(uint64 value) {
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return __builtin_popcountll(value);
#elif defined(COMPILER_MSVC)
	return __popcnt64(value);
#endif
}

inline uint32 BitRotateLeft(uint32 value, int32 shift) {
#if defined(_NO_ROTATE_)
	return (value << shift) | (value >> (32 - shift));
#else
	return _rotl(value, shift);
#endif
}

inline uint32 BitRotateRight(uint32 value, int32 shift) {
#if defined(_NO_ROTATE_)
	return (value << (32 - shift)) | (value >> shift);
#else
	return _rotr(value, shift);
#endif
}

inline uint64 BitRotateLeft(uint64 value, int32 shift) {
#if (defined(COMPILER_MSVC) || defined(COMPILER_CLANG)) && !defined(_NO_ROTATE_)
	return _rotl64(value, shift);
#else
	return (value << shift) | (value >> (64 - shift));
#endif
}

inline uint64 BitRotateRight(uint64 value, int32 shift) {
#if (defined(COMPILER_MSVC) || defined(COMPILER_CLANG)) && !defined(_NO_ROTATE_)
	return _rotr64(value, shift);
#else
	return (value << (64 - shift)) | (value >> shift);
#endif
}

inline bool IsPowerOf2OrZero(uint64 value) {
	return (value & (value - 1)) == 0;
}

inline int32 ReverseBits16(int32 n) {
	n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
	return n;
}

inline int32 ReverseBits32(int32 n) {
    n = ((n & 0xAAAAAAAA) >> 1)  | ((n & 0x55555555) << 1);
    n = ((n & 0xCCCCCCCC) >> 2)  | ((n & 0x33333333) << 2);
    n = ((n & 0xF0F0F0F0) >> 4)  | ((n & 0x0F0F0F0F) << 4);
    n = ((n & 0xFF00FF00) >> 8)  | ((n & 0x00FF00FF) << 8);
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
    return n;
}

inline int32 ReverseBits(int32 n, int32 bits) {
	if (bits <= 16)
		return ReverseBits16(n) >> (16 - bits);
	else
		return ReverseBits32(n);
}