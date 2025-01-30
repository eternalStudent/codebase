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

ssize UnsignedToDecimal(uint64 number, byte* str) {
	ssize numberOfDigits = log10(number);
	ssize index = numberOfDigits;

	str[index] = 0;
	do {
		index--;
		str[index] = '0' + (number % 10);
		number /= 10;
	} while (index);

	return numberOfDigits;
}

ssize UnsignedToDecimal(uint64 high, uint64 low, byte* str) {
	if (high == 0)
		return UnsignedToDecimal(low, str);

	// get the last decimal digit
	uint64 high_r = high % 10;
	high = high / 10;
	uint64 last;
	low = udiv(high_r, low, 10, &last);

	byte* ptr = str;
	if (high == 0) {
		ptr += UnsignedToDecimal(low, ptr);
	}
	else {
		uint64 remainder;
		uint64 quotient = udiv(high, low, 10000000000000000000, &remainder);
		ptr += UnsignedToDecimal(quotient, ptr);
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

static int32 bitIndexToExp[53] = {
	55, 54, 53, 53, 52, 52, 52, 51, 51, 50, 50, 
	49, 49, 49, 48, 48, 47, 47, 46, 46, 46, 45, 45, 
	44, 44, 43, 43, 43, 42, 42, 41, 41, 40, 40, 40, 
	39, 39, 38, 38, 37, 37, 37, 36, 36, 35, 35, 
	34, 34, 34, 33, 33, 32, 32
};

static struct {uint64 high, low;} powersOf5[24] = {
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

ssize FloatToDecimal(uint64 m2, int32 e2, int32 precision, byte* buffer) {
	if (m2 == 0) {
		buffer[0] = '0';
		buffer[1] = '.';
		buffer[2] = '0';
		return 3;
	}

	uint32 p = (uint32)precision;
	byte* ptr = buffer;

	int32 trailingZeroes = (int32)LowBit(m2);
	m2 >>= trailingZeroes;
	e2 += trailingZeroes;

	int32 highBit = (int32)HighBit(m2, 0);
	ASSERT(highBit <= 52);
	bool hasWhole = 0 <= e2 || -e2 <= highBit;
	if (hasWhole) {
		bool wholeFitsIn128 = highBit + 1 + e2 <= 128;
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
			// TODO:
			ASSERT(0);
		}
	} else {
		*(ptr++) = '0';
	}

	*(ptr++) = '.';

	bool hasFraction = e2 < 0;
	if (hasFraction) {
		int32 denominatorExp = -e2;
		if (denominatorExp < 64) {
			uint64 denominator = 1ull << denominatorExp;
			uint64 numerator = m2 & (denominator - 1);

			for (uint32 digits = 0; numerator && digits < p; digits++) {
				uint64 high;
				numerator = umul(numerator, 10, &high);
				uint64 digit = udiv(high, numerator, denominator, &numerator);
				*(ptr++) = (byte)digit + '0';
			}
		}
		else {
			ASSERT(!hasWhole);
			ptr -= 2;

			// we need to either multiply m2 by 5, or shift it by 1, -e2 times
			// start by multiplying m2 by 5^n, where n can be determined by the high-bit
			int32 e10 = bitIndexToExp[highBit];
			if (e10 > -e2) e10 = -e2;
			uint64 high;
			uint64 low = umul(powersOf5[e10 - 32].high, powersOf5[e10 - 32].low, m2, &high);
			
			// we already multipled by 5 e10 times, so we don't start with 0
			for (int32 i = e10; i < -e2; i++) {
				// if we can safely multiply by 5, then this is our preference
				if (high < 3689348814741910323) {
					low = umul(high, low, 5, &high);
					e10++;
				}
				else {
					low >>= 1;
					low |= ((high & 1) << 63);
					high >>= 1;
				}
			}

			// TODO: remove trailing zeroes
			ssize length = UnsignedToDecimal(high, low, ptr);
			int32 exp = e10 - (int32)length + 1;
			if (length - 2 > p) length = p + 2;
			memmove(ptr + 1, ptr, length);
			ptr[1] = '.';
			ptr += length + 1;
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
	uint64 result = 0;
	for (ssize i = 0; i < str.length; i++) {
		byte c = str.data[i];
		result *= 10;
		result += c-'0';
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

		if (i == 19 &&  (*result > 1844674407370955161
							||
						(*result == 1844674407370955161 && c > '5'))) {

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

		if (value > 9223372036854775808)
			return false;

		*result = - (int64)value;
	}
	else {
		uint64 value;
		if (!TryParseUInt64(str, &value))
			return false;

		if (value > 9223372036854775807)
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