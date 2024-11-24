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

ssize SignedToDecimal(int64 number, byte* str) {
	if (number < 0) {
		*str = '-';
		return UnsignedToDecimal((uint32)(-number), str + 1) + 1;
	}
	return UnsignedToDecimal((uint32)number, str);
}

ssize FloatToDecimal(float32 number, int64 precision, byte* str) {

	// output integer part
	int64 integer = (int64)number;
	number -= (float32)integer;
	ssize count = SignedToDecimal(integer, str);
	if (!precision) return count;

	// output fraction part
	str += count;
	*str = '.';
	str++;
	count += precision + 1;
	while (precision) {
		number *= 10.0f;
		int64 digit = (int64)number;
		*str = (byte)('0' + digit);
		number -= (float32)digit;
		str++;
		precision--;
	}
	*str = 0;
	return count;
}

ssize FloatToDecimal(float64 number, int64 precision, byte* str) {

	// output integer part
	int64 integer = (int64)number;
	number -= (float64)integer;
	ssize count = SignedToDecimal(integer, str);
	if (!precision) return count;

	// output fraction part
	str += count;
	*str = '.';
	str++;
	count += precision + 1;
	while (precision) {
		number *= 10.0f;
		int64 digit = (int64)number;
		*str = (byte)('0' + digit);
		number -= (float64)digit;
		str++;
		precision--;
	}
	*str = 0;
	return count;
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

// TODO: add TryParse variation
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

	StringBuilder operator()(char ch) {
		StringBuilder concat = *this;
		*(concat.ptr) = (byte)ch;
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
		concat.ptr += FloatToDecimal(f, precision, concat.ptr);
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