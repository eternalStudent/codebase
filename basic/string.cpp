inline bool IsWhiteSpace(byte b) {
	return (9 <= b && b <= 13 || b == 32);
}

inline bool IsDigit(byte ch) {
	return '0' <= ch && ch <= '9';
}

inline bool IsHexDigit(byte ch) {
	return IsDigit(ch) || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
}

inline bool IsAlpha(byte ch) {
	return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || (ch == '_');
}

inline bool IsAlphaNumeric(byte ch) {
	return IsAlpha(ch) || IsDigit(ch);
}

// Null-Terminated
//-----------------

ssize UnsignedToBinary(uint64 number, byte* str) {
	int64 index = HighBit(number, 0);
	ssize numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		str[numberOfDigits - index - 1] = (number & (1ull << index)) ? '1' : '0';
		index--;
	}
	return numberOfDigits;
}

ssize UnsignedToHex(uint64 number, byte* str) {
	int64 index = HighBit(number, 0) / 4;
	ssize numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		uint32 digit = (number >> (index * 4)) & 0xf;
		str[numberOfDigits - index - 1] = digit <= 9 ? '0' + (byte)digit : ('a' - 10) + (byte)digit;
		index--;
	}
	return numberOfDigits;
}

static inline uint64 getNumberOfDecimalDigits(uint64 n) {
	static uint64 table[] = {
		9,
		99,
		999,
		9999,
		99999,
		999999,
		9999999,
		99999999,
		999999999,
		9999999999,
		99999999999,
		999999999999,
		9999999999999,
		99999999999999,
		999999999999999ull,
		9999999999999999ull,
		99999999999999999ull,
		999999999999999999ull,
		9999999999999999999ull
	};
	uint64 y = (19 * HighBit(n, 0) >> 6);
	y += n > table[y];
	return y + 1;
}

ssize UnsignedToDecimal(uint64 number, byte* str) {
	ssize numberOfDigits = getNumberOfDecimalDigits(number);
	ssize index = numberOfDigits;

	str[index] = 0;
	do {
		index--;
		str[index] = '0' + (number % 10);
		number /= 10;
	} while (index);

	return numberOfDigits;
}

// TODO: I don't love this implementation
ssize UnsignedToDecimal(uint64 high, uint64 low, byte* str) {
	if (high == 0)
		return UnsignedToDecimal(low, str);

	uint64 last;
	low = udiv10(high, low, &last, &high);

	byte* ptr = str;
	if (high == 0) {
		ptr += UnsignedToDecimal(low, ptr);
	}
	else {
		uint64 remainder;
		uint64 quotient = udiv(high, low, 10000000000000000000ull, &remainder);
		ptr += UnsignedToDecimal(quotient, ptr);
		// TODO: I might want to add a pad parameter to UnsignedToDecimal
		ssize length = UnsignedToDecimal(remainder, ptr);
		if (length < 19) {
			ssize diff = 19 - length;
			memmove(ptr + diff, ptr, length);
			memset(ptr, '0', diff);
		}
		ptr += 19;
	}
	*(ptr++) = (byte)last + '0';
	return ptr - str;
}

ssize SignedToDecimal(int64 number, byte* str) {
	if (number < 0) {
		*str = '-';
		return UnsignedToDecimal((uint32)(-number), str + 1) + 1;
	}
	return UnsignedToDecimal((uint32)number, str);
}

ssize FloatToDecimal(uint64 m2, int32 e2, int32 precision, byte* buffer) {

	static int32 exponentOf5[53] = {
		55, 54, 53, 53, 52, 52, 52, 51, 51, 50, 50, 
		49, 49, 49, 48, 48, 47, 47, 46, 46, 46, 45, 45, 
		44, 44, 43, 43, 43, 42, 42, 41, 41, 40, 40, 40, 
		39, 39, 38, 38, 37, 37, 37, 36, 36, 35, 35, 
		34, 34, 34, 33, 33, 32, 32
	};

	// starting with 5^32
	static struct {uint64 high, low;} powerOf5[24] = {
		{0x4ee, 0x2d6d415b85acef81},
		{0x18a6, 0xe32246c99c60ad85},
		{0x7b42, 0x6fab61f00de36399},
		{0x2684c, 0x2e58e9b04570f1fd},
		{0xc097c, 0xe7bc90715b34b9f1},
		{0x3c2f70, 0x86aed236c807a1b5},
		{0x12ced32, 0xa16a1b11e8262889},
		{0x5e0a1fd, 0x2712875988becaad},
		{0x1d6329f1, 0xc35ca4bfabb9f561},
		{0x92efd1b8, 0xd0cf37be5aa1cae5},
		{0x2deaf189c, 0x140c16b7c528f679},
		{0xe596b7b0c, 0x643c7196d9ccd05d},
		{0x47bf19673d, 0xf52e37f2410011d1},
		{0x166bb7f0435, 0xc9e717bb45005915},
		{0x701a97b150c, 0xf18376a85901bd69},
		{0x23084f676940, 0xb7915149bd08b30d},
		{0xaf298d050e43, 0x95d69670b12b7f41},
		{0x36bcfc1194751, 0xed30f03375d97c45},
		{0x111b0ec57e6499, 0xa1f4b1014d3f6d59},
		{0x558749db77f700, 0x29c77506823d22bd},
		{0x1aba4714957d300, 0xd0e549208b31adb1},
		{0x85a36366eb71f04, 0x147a6da2b7f86475},
		{0x29c30f1029939b14, 0x6664242d97d9f649},
		{0xd0cf4b50cfe20765, 0xfff4b4e3f741cf6d},
	};

	if (m2 == 0) {
		buffer[0] = '0';
		buffer[1] = '.';
		buffer[2] = '0';
		return 3;
	}

	uint32 p = (uint32)precision;
	byte* ptr = buffer;

	int32 trailingZeroes = (int32)LowBit(m2);

	// simplify the fraction
	m2 >>= trailingZeroes;
	e2 += trailingZeroes;

	int32 highBit = (int32)HighBit(m2, 0); 
	ASSERT(highBit <= 52);

	bool hasWhole = 0 <= e2 || -e2 <= highBit;
	if (hasWhole) {
		bool wholeFitsIn128 = highBit + e2 < 128;
		if (wholeFitsIn128) {
			uint64 high = 0;
			uint64 low = 0;
			if (e2 < 0) {
				low = m2 >> -e2;
			}
			else if (e2 == 0) {
				low = m2;
			}
			else /*(e2 > 0)*/ {
				low = (e2 < 64) ? m2 << e2 : 0;
				high = (e2 < 64) ? m2 >> (64 - e2) : m2 << (e2 - 64);
			}
			ptr += UnsignedToDecimal(high, low, ptr);
		}
		else {
			/*
			 * We need to reduce the exponent of 2 to 0, 
			 * but we can only shift left `127 - highBit` times
			 * and still fit into 128 bits
			 * and we know that `highBit + e2 > 127`, so it's not enough.
			 *
			 * The trick here is that x*2^n = x*2^n * 5^(-m)*5^m
			 *                              = x*2^(n-m)*2^m * 5^(-m)*5^m
			 *                              = x*2^(n-m) * 5^(-m) * 2^m*5^m
			 *                              = x*2^(n-m)*5^(-m) * 10^m
			 *
			 * So each time we divide the mantisa by 5, 
			 * we reduce the exponent of 2 by 1
			 * and increase the exponent of 10 by 1
			 * but dividing by 5 reduces precision,
			 * so whenever possible we prefer to shift left.
			 */
			uint64 low = 0;
			uint64 high = m2 << (63 - highBit);
			uint32 e10 = 0;

			for (int32 i = 0; i < e2 + highBit - 127; i++) {
				if ((high & 0x8000000000000000) == 0) {
					low = ushiftleft(high, low, 1, &high);
				}
				else {
					uint64 remainder;
					low = udiv5(high, low, &remainder, &high);
					e10++;
				}
			}

			ssize length = UnsignedToDecimal(high, low, ptr);
			uint32 exp = e10 + (uint32)length - 1;
			if (length - 2 > p) length = p + 2;
			memmove(ptr + 1, ptr, length);
			ptr[1] = '.';
			ptr += length + 1;

			// remove trailing zeroes
			while (*(ptr - 1) == '0') ptr--;

			*(ptr++) = 'e';
			ptr += UnsignedToDecimal(exp, ptr);

			// there's not going to be any fraction, so just return
			return ptr - buffer;
		}
	} else {
		*(ptr++) = '0';
	}

	*(ptr++) = '.';

	bool hasFraction = e2 < 0;
	if (hasFraction) {
		byte denominatorExp = (byte)-e2;
		if (denominatorExp < 64) {
			uint64 denominator = 1ull << denominatorExp;
			uint64 mask = denominator - 1;
			uint64 numerator = m2 & mask; // remove the whole part

			// This is just long division
			for (uint32 digits = 0; numerator && digits < p; digits++) {
				uint64 high;
				numerator = umul(0, numerator, 10, &high);
				uint64 digit = ushiftright(high, numerator, denominatorExp, &high);
				numerator = numerator & mask;
				*(ptr++) = (byte)digit + '0';
			}

			// remove trailing zeroes
			while (*(ptr - 1) == '0') ptr--;
		}
		else {
			ASSERT(!hasWhole);
			// oops we printed `0.` but we don't need it.
			ptr -= 2;

			/*
			 * We need to increase the exponent of 2 to 0.
			 *
			 * The trick here is that x*2^(-n) = x*(2^-n) * 5^m*5^(-m)
			 *                                 = x*2^(m-n)*2^m * 5^m*5^(-m)
			 *                                 = x*2^(m-n) * 5^m * 2^(-m)*5^(-m)
			 *                                 = x*2^(m-n)*5^m * 10^(-m)
			 *
			 * So each time we multiple the mantisa by 5, 
			 * we increase the exponent of 2 by 1
			 * and decrease the exponent of 10 by 1.
			 * We'll figure out how many times we can safely multiply by 5
			 * using the hight bit, and taking the worst case scenario.
			 * and then multiply by a 5 to the power of that many times.
			 * Whenever we can't multiply by 5 without overflowing, 
			 * we will shift right instead, which also increases the exponent of 2,
			 * but decreases precision.
			 */

			int32 e10 = exponentOf5[highBit];
			if (e10 > -e2) e10 = -e2;
			uint64 high;
			uint64 low = umul(powerOf5[e10 - 32].high, powerOf5[e10 - 32].low, m2, &high);
			for (int32 i = e10; i < -e2; i++) {
				if (high < 3689348814741910323ull) {
					low = umul(high, low, 5, &high);
					e10++;
				}
				else {
					low = ushiftright(high, low, 1, &high);
				}
			}

			ssize length = UnsignedToDecimal(high, low, ptr);
			int32 exp = e10 - (int32)length + 1;
			if (length - 2 > p) length = p + 2;
			memmove(ptr + 1, ptr, length);
			ptr[1] = '.';
			ptr += length + 1;

			// remove trailing zeroes
			while (*(ptr - 1) == '0') ptr--;

			*(ptr++) = 'e';
			*(ptr++) = '-';
			ptr += UnsignedToDecimal(exp, ptr);
		}
	}
	else {
		*(ptr++) = '0';
	}

	return ptr - buffer;
}

ssize Float32ToDecimal(uint32 value, int32 precision, byte* buffer) {
	uint32 exp  = (value & 0x7F800000) >> 23;
	uint32 sign = (value & 0x80000000) >> 31;
	uint64 significand = (value & 0x7FFFFF);

	if (exp == 0xFF) {
		if (significand) {
			buffer[0] = 'N';
			buffer[1] = 'a';
			buffer[2] = 'N';
			return 3;
		}
		
		if (sign) {
			buffer[0] = '-';
			buffer[1] = 'i';
			buffer[2] = 'n';
			buffer[3] = 'f';
			return 4;
		}

		else {
			buffer[0] = 'i';
			buffer[1] = 'n';
			buffer[2] = 'f';
			return 3;
		}
	}

	ssize length = 0;
	if (sign) {
		buffer[0] = '-';
		buffer++;
		length++;
	}

	if (exp == 0) {
		length += FloatToDecimal(significand, -149, precision, buffer);
	}
	else {
		length += FloatToDecimal(significand | 0x800000, (int32)exp - 127 - 23, precision, buffer);
	}

	return length;
}

ssize Float32ToDecimal(float32 value, int32 precision, byte* buffer) {
	union {float32 f; uint32 u;} cvt;
	cvt.f = value;
	return Float32ToDecimal(cvt.u, precision, buffer);
}

ssize Float64ToDecimal(uint64 value, int32 precision, byte* buffer) {
	uint32 sign =        (value & 0x8000000000000000) >> 63;
	uint32 exp  =        (value & 0x7FF0000000000000) >> 52;
	uint64 significand = (value & 0x000FFFFFFFFFFFFF) >> 00;

	if (exp == 0x7FF) {
		if (significand) {
			buffer[0] = 'N';
			buffer[1] = 'a';
			buffer[2] = 'N';
			return 3;
		}
		
		if (sign) {
			buffer[0] = '-';
			buffer[1] = 'i';
			buffer[2] = 'n';
			buffer[3] = 'f';
			return 4;
		}

		else {
			buffer[0] = 'i';
			buffer[1] = 'n';
			buffer[2] = 'f';
			return 3;
		}
	}

	ssize length = 0;
	if (sign) {
		buffer[0] = '-';
		buffer++;
		length++;
	}

	if (exp == 0) {
		length += FloatToDecimal(significand, -1075, precision, buffer);
	}
	else {
		length += FloatToDecimal(significand | 0x10000000000000, (int32)exp - 1023 - 52, precision, buffer);
	}

	return length;
}

ssize Float64ToDecimal(float64 value, int32 precision, byte* buffer) {
	union {float64 f; uint64 u;} cvt;
	cvt.f = value;
	return Float64ToDecimal(cvt.u, precision, buffer);
}

#define COPY(source, dest) (memcpy(dest, source, sizeof(source)-1), sizeof(source)-1)

inline ssize BoolToAnsi(bool b, byte* str) {
	if (b) {
		return COPY("true", str);
	}
	else {
		return COPY("false", str);
	}
}

ssize ToUpperHex(byte b, byte* buffer) {
	byte nibble = (b & 0xF0) >> 4;
	buffer[0] = nibble <= 9 ? '0' + nibble : ('A' - 10) + nibble;
	nibble = b & 0xF;
	buffer[1] = nibble <= 9 ? '0' + nibble : ('A' - 10) + nibble;
	return 2;
}

ssize ToLowerHex(byte b, byte* buffer) {
	byte nibble = (b & 0xF0) >> 4;
	buffer[0] = nibble <= 9 ? '0' + nibble : ('a' - 10) + nibble;
	nibble = b & 0xF;
	buffer[1] = nibble <= 9 ? '0' + nibble : ('a' - 10) + nibble;
	return 2;
}

// Length-based
//--------------

struct String {
	byte* data;
	ssize length;
};

struct String16 {
	uint16* data;
	ssize length;
};

struct String32 {
	uint32* data;
	ssize length;
};

#define STR(x) String{((byte*)x), (sizeof(x)-1)}

#define STR16(x) String16{((uint16*)x), ((sizeof(x)/2)-1)}

ssize StringCopy(String source, byte* dest) {
	memcpy(dest, source.data, source.length);
	return source.length;
}

bool StringEquals(String str1, String str2) {
	if (str1.length != str2.length) return false;
	return memcmp(str1.data, str2.data, str1.length) == 0;
}

// if <0 str1 is lexically earlier
ssize StringCompare(String str1, String str2) {
	int32 cmp = memcmp(str1.data, str2.data, MIN(str1.length, str2.length));
	if (str1.length == str2.length || cmp != 0) return cmp;

	// one is a prefix of the other
	return str1.length - str2.length;
}

ssize StringSplit(String text, String separator, String* buffer) {
	ssize j = 0;
	buffer[0].data = text.data;
	for (ssize i = 0; i < text.length - separator.length;) {
		if (StringEquals({text.data+i, separator.length}, separator)) {
			buffer[j].length = text.data+i - buffer[j].data;
			j++;
			i+=(ssize)separator.length;
			buffer[j].data = text.data+i;
		}
		else i++;
	}
	buffer[j].length = text.data+text.length - buffer[j].data;
	return j+1;
}

ssize StringGetFirstIndexOf(String string, byte b) {
	byte* ptr = string.data;
	byte* end = ptr + string.length;

	__m128i mask = _mm_set1_epi8(b);	
	while (ptr < end - 15) {
		__m128i cmp = _mm_cmpeq_epi8(_mm_loadu_si128((__m128i*)ptr), mask);
		uint32 res = _mm_movemask_epi8(cmp);
		if (res) {
			return ptr - string.data + LowBit(res);
		}
		ptr += 16;
	}

	while (ptr < end) {
		if (*ptr == b) {
			return ptr - string.data;
		}
		ptr++;
	}
	return string.length;
}

ssize StringGetLastIndexOf(String string, byte b) {
	byte* ptr = string.data + string.length - 1;

	__m128i mask = _mm_set1_epi8(b);
	while (ptr >= string.data + 15) {
		__m128i cmp = _mm_cmpeq_epi8(_mm_loadu_si128((__m128i*)ptr - 1), mask);
		uint32 res = _mm_movemask_epi8(cmp);
		if (res) {
			return ptr - string.data - 16 + HighBit(res, 0);
		}
		ptr -= 16;
	}

	while (ptr >= string.data) {
		if (*ptr == b) {
			return ptr - string.data;
		}
		ptr--;
	}
	return -1;
}

void StringFindWord(String string, ssize index, ssize* start, ssize* end) {
	*start = 0;

	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		bool alphaNumeric = 'a' <= b && b <= 'z' || 'A' <= b && b <= 'Z' || '0' <= b && b <= '9' || b == '_';
		if (i == index && !alphaNumeric) {
			*start = i;
			*end = i + 1;
			return;
		}
		if (i < index && !alphaNumeric) {
			*start = i + 1;
		}
		if (index < i && !alphaNumeric) {
			*end = i;
			return;
		}
	}
	*end = string.length;
}

uint64 ParseUInt64(String str) {
	byte* data = str.data;
	ssize length = str.length;
	ASSERT(str.length <= 20);

	static const uint8_t shufmask[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	};

	const __m128i zeroes =  _mm_set1_epi8('0');
	const __m128i factors1 = _mm_set_epi8(1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10);
	const __m128i factors2 = _mm_set_epi16(1, 100, 1, 100, 1, 100, 1, 100);
	const __m128i factors3 = _mm_set_epi16(1, 10000, 1, 10000, 1, 10000, 1, 10000);
	static const uint64 factors[] = {10, 100, 1000, 10000};

	uint64 result;
	if (length <= 16) {
		// if [data, data+16) crosses page boundary, but [data, data+length) does not
		// then you potentially cannot load [data] as 16-byte load
		// will need to load at previous 16-byte boundary (data rounded down to 16 factorsiple)
		// extra is amount of bytes to advance to there
		uint64 address = (uint64)data % 16;
		uint64 extra = (address + length <= 16) ? address : 0;

		__m128i shuffle = _mm_loadu_si128((const __m128i*)(shufmask + 16 + extra + length));
		__m128i mask = _mm_loadu_si128((const __m128i*)(shufmask + length));

		__m128i x = _mm_loadu_si128((const __m128i*)(data - extra));
		x = _mm_shuffle_epi8(x, shuffle);

		x = _mm_sub_epi8(x, zeroes);
		x = _mm_and_si128(x, mask);
		x = _mm_maddubs_epi16(x, factors1);
		x = _mm_madd_epi16(x, factors2);
		x = _mm_packus_epi32(x, x);
		x = _mm_madd_epi16(x, factors3);

		uint32 a = _mm_extract_epi32(x, 0);
		uint32 b = _mm_extract_epi32(x, 1);
		result = 100000000ull*a + b;
	}
	else {
		__m128i hi = _mm_loadu_si128((const __m128i*)data);

		hi = _mm_sub_epi8(hi, zeroes);
		hi = _mm_maddubs_epi16(hi, factors1);
		hi = _mm_madd_epi16(hi, factors2);
		hi = _mm_packus_epi32(hi, hi);
		hi = _mm_madd_epi16(hi, factors3);

		uint32 hi0 = _mm_extract_epi32(hi, 0);
		uint32 hi1 = _mm_extract_epi32(hi, 1);
		uint64 a = 100000000ull*hi0 + hi1;

		// TODO: this can be done in SWAR, feels like an overkill
		__m128i mask = _mm_loadu_si128((const __m128i*)(shufmask + length - 16));
		__m128i lo = _mm_loadu_si128((const __m128i*)(data + length - 16));
		lo = _mm_sub_epi8(lo, zeroes);
		lo = _mm_and_si128(lo, mask);
		lo = _mm_maddubs_epi16(lo, factors1);
		lo = _mm_madd_epi16(lo, factors2);
		uint64 b = (uint64)(uint32)_mm_extract_epi32(lo, 3);

		result = factors[length - 16 - 1]*a + b;
	}

	return result;
}

bool TryParseUInt64(String str, uint64* result) {
	if (str.length > 20)
		return false;

	*result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		if (!IsDigit(c))
			return false;

		if (i == 19 &&  (*result > 1844674407370955161ull
							||
						(*result == 1844674407370955161ull && c > '5'))) {

			return false;
		}

		*result *= 10;
		*result += c-'0';
	}
	return true;
}

// TODO: add TryParse variations
int64 ParseInt64(String str) {
	return str.data[0] == '-' 
		? - (int64)ParseUInt64({str.data+1, str.length-1})
		: (int64)ParseUInt64(str); 
}

bool TryParseInt64(String str, int64* result) {
	if (str.data[0] == '-') {
		uint64 value;
		if (!TryParseUInt64({str.data+1, str.length-1}, &value))
			return false;

		if (value > 9223372036854775808ull)
			return false;

		*result = - (int64)value;
	}
	else {
		uint64 value;
		if (!TryParseUInt64(str, &value))
			return false;

		if (value > 9223372036854775807ull)
			return false;

		*result = (int64)value;
	}

	return true;
}

uint64 ParseHex64(String str) {
	uint64 result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		result <<= 4;
		if (IsDigit(c))
			result += c-'0';
		else if ('a' <= c && c <= 'f')
			result += c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			result += c - 'A' + 10;
	}
	return result;
}

bool TryParseHexDigit(byte digit, byte* result) {
	if ('0' <= digit && digit <= '9') {
		*result = digit - '0';
		return true;
	}

	if ('a' <= digit && digit <= 'f') {
		*result = digit - 'a' + 10;
		return true;
	}

	if ('A' <= digit && digit <= 'F') {
		*result = digit - 'A' + 10;
		return true;
	}

	return false;
}

bool TryParseHex64(String str, uint64* result) {
	if (str.length > 16)
		return false;

	*result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		byte nibble;
		if (TryParseHexDigit(c, &nibble)) {
			*result <<= 4;
			*result |= nibble;
		}
		else {
			return false;
		}
	}
	return true;
}

bool TryParseHex32(String str, uint32* result) {
	if (str.length > 8)
		return false;

	uint64 value64;
	if (TryParseHex64(str, &value64)) {
		*result = (uint32)value64;
		return true;
	}

	return false;
}

uint64 ParseBinary64(String str) {
	uint64 result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		result <<= 1;
		if (c == '1') result |= 1;
	}
	return result;
}

bool TryParseBinary64(String str, uint64* result) {
	if (str.length > 64)
		return false;

	*result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		if (c != '0' && c != '1')
			return false;

		*result <<= 1;
		if (c == '1') *result |= 1;
	}
	return true;
}

float64 ParseFloat64(String str) {
	float64 result = 0.0;
	ssize start = str.data[0] == '-' ? 1 : 0;
	bool radix = false;
	float64 term = 1.0;
	for (ssize i = start; i < str.length; i++) {
		byte c = str.data[i];
		byte digit = c-'0';
		if (c == '.') {
			radix = true;
			continue;     
		}
		if (c == 'e' || c == 'E') {
			String exp;
			ssize j = str.data[i+1] == '+' ? i+2 : i+1;
			exp.data = &str.data[j];
			exp.length = str.length - j;
			result *= pow10((int32)ParseInt64(exp));
			break;
		}
		if (radix) {
			term *= 10.0;
			result += digit/term;
		}
		else {
			result *= 10.0;
			result += digit;
		}
	}
	if (start == 1) result = -result;
	return result;
}

bool TryParseFloat64(String str, float64* result) {
	*result = 0.0;
	ssize start = str.data[0] == '-' ? 1 : 0;
	bool radix = false;
	float64 term = 1.0;
	for (ssize i = start; i < str.length; i++) {
		byte c = str.data[i];
		if (c == '.') {
			if (radix) 
				return false;

			radix = true;
			continue;     
		}
		if (c == 'e' || c == 'E') {
			String exp;
			ssize j = str.data[i+1] == '+' ? i+2 : i+1;
			exp.data = &str.data[j];
			exp.length = str.length - j;
			int64 exp64;
			if (!TryParseInt64(exp, &exp64))
				return false;

			if (exp64 > MAX_INT32 || exp64 < MIN_INT32)
				return false;

			*result *= pow10((int32)exp64);
			break;
		}

		if (c < '0' || c > '9')
			return false;

		byte digit = c - '0';

		if (radix) {
			term *= 10.0;
			*result += digit/term;
		}
		else {
			*result *= 10.0;
			*result += digit;
		}
	}
	if (start == 1) *result = -(*result);
	return true;
}

// TODO: add TryParse variation
float32 ParseFloat32(String str) {
	float32 result = 0.0f;
	ssize start = str.data[0] == '-' ? 1 : 0;
	bool radix = false;
	float32 term = 1.0f;
	for (ssize i = start; i < str.length; i++) {
		byte c = str.data[i];
		byte digit = c-'0';
		if (c == '.') {
			radix = true;
			continue;     
		}
		if (c == 'e' || c == 'E') {
			String exp;
			ssize j = str.data[i+1] == '+' ? i+2 : i+1;
			exp.data = &str.data[j];
			exp.length = str.length - j;
			result *= (float32)pow10((int32)ParseInt64(exp));
			break;
		}
		if (radix) {
			term *= 10.0f;
			result += digit/term;
		}
		else {
			result *= 10.0f;
			result += digit;
		}
	}
	if (start == 1) result = -result;
	return result;
}

// StringList
//------------

/*
 * NOTE: I'm including the definition here so the StringBuilder can make use of it, 
 *       but the rest is in a separate file.
 */

struct StringNode {
	String string;
	StringNode* prev;
	StringNode* next;
};

struct StringList {
	StringNode* first;
	StringNode* last;
	ssize length;
};

ssize StringListCopy(StringList list, byte* buffer) {
	byte* ptr = buffer;
	for (StringNode* node = list.first; node != NULL; node = node->next) {
		ptr += StringCopy(node->string, ptr);
	}
	return ptr - buffer;
}

// String Builder
//----------------

typedef ssize Stringify(void* data, byte* buffer);

struct StringBuilder {
	byte* buffer;
	byte* ptr;

	StringBuilder operator()(void* data, ssize length) {
		StringBuilder concat = *this;
		memcpy(concat.ptr, data, length);
		concat.ptr += length;
		return concat;
	}

	StringBuilder operator()(char* str) {
		StringBuilder concat = *this;
		if (str == NULL)
			return concat;
		
		for (; *str; str++, concat.ptr++) *(concat.ptr) = *str;
		return concat;
	}

	StringBuilder operator()(String string) {
		StringBuilder concat = *this;
		concat.ptr += StringCopy(string, concat.ptr);
		return concat;
	}

	StringBuilder operator()(StringList list) {
		StringBuilder concat = *this;
		concat.ptr += StringListCopy(list, concat.ptr);
		return concat;
	}

	StringBuilder operator()(byte ch, char f = 'c') {
		StringBuilder concat = *this;
		if (f == 'c') *(concat.ptr++) = ch;
		if (f == 'd') concat.ptr += UnsignedToDecimal((uint64)ch, concat.ptr);
		if (f == 'h' || f == 'x') concat.ptr += ToLowerHex(ch, concat.ptr);
		if (f == 'H' || f == 'X') concat.ptr += ToUpperHex(ch, concat.ptr);
		return concat;
	}

	StringBuilder operator()(char ch, char f = 'a') {
		StringBuilder concat = *this;
		if (f == 'a' || (32 <= ch && ch <= 126)) *(concat.ptr) = (byte)ch;
		else *(concat.ptr) = '.';
		concat.ptr++;
		return concat;
	}

	StringBuilder operator()(int16 i) {
		StringBuilder concat = *this;
		concat.ptr += SignedToDecimal(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(int32 i) {
		StringBuilder concat = *this;
		concat.ptr += SignedToDecimal(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(int64 i) {
		StringBuilder concat = *this;
		concat.ptr += SignedToDecimal(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(uint16 i, char f = 'd') {
		StringBuilder concat = *this;
		if (f == 'd') concat.ptr += UnsignedToDecimal(i, concat.ptr);
		if (f == 'b') concat.ptr += UnsignedToBinary(i, concat.ptr);
		if (f == 'x') concat.ptr += UnsignedToHex(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(uint32 i, char f = 'd') {
		StringBuilder concat = *this;
		if (f == 'd') concat.ptr += UnsignedToDecimal(i, concat.ptr);
		if (f == 'b') concat.ptr += UnsignedToBinary(i, concat.ptr);
		if (f == 'x') concat.ptr += UnsignedToHex(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(uint64 i, char f = 'd', ssize pad = 0) {
		StringBuilder concat = *this;
		ssize length = 0;
		if (f == 'd') length = UnsignedToDecimal(i, concat.ptr);
		if (f == 'b') length = UnsignedToBinary(i, concat.ptr);
		if (f == 'x') length = UnsignedToHex(i, concat.ptr);
		if (pad > length) {
			ssize diff = pad - length;
			memmove(concat.ptr + diff, concat.ptr, length);
			memset(concat.ptr, '0', diff);
			length = pad;
		}
		concat.ptr += length;
		return concat;
	}

	StringBuilder operator()(float32 f, int32 precision) {
		StringBuilder concat = *this;
		concat.ptr += Float32ToDecimal(f, precision, concat.ptr);
		return concat;
	}

	StringBuilder operator()(float64 f, int32 precision) {
		StringBuilder concat = *this;
		concat.ptr += Float64ToDecimal(f, precision, concat.ptr);
		return concat;
	}

	StringBuilder operator()(void* data, Stringify* stringify) {
		StringBuilder concat = *this;
		concat.ptr += stringify(data, concat.ptr);
		return concat;
	}

	String operator()() {
		return {this->buffer, this->ptr - this->buffer};
	}
};