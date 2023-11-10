inline int64 HighBit(uint32 value, int64 onZero) {
	if (value == 0) return onZero;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return 31-__builtin_clz(value);
#elif defined(COMPILER_MSVC)
	unsigned long index;
	_BitScanReverse(&index, value);
	return (int64)index;
#endif
}

inline int64 HighBit(uint64 value, int64 onZero) {
	if (value == 0) return onZero;

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	return 63-__builtin_clzll(value);
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
