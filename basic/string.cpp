inline bool IsWhiteSpace(byte b) {
	return (9 <= b && b <= 13 || b == 32);
}

// Null-Terminated
//-----------------

int32 Uint32ToBinary(uint32 number, byte* str) {
	int32 index = HighBit(number, 0);
	int32 numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		str[numberOfDigits - index - 1] = (number & (1 << index)) ? '1' : '0';
		index--;
	}
	return numberOfDigits;
}

int32 Uint32ToHex(uint32 number, byte* str) {
	int32 index = HighBit(number, 0) / 4;
	int32 numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		uint32 digit = (number >> (index * 4)) & 0xf;
		str[numberOfDigits - index - 1] = digit <= 9 ? '0' + (char)digit : ('a' - 10) + (char)digit;
		index--;
	}
	return numberOfDigits;
}

int32 UnsignedToDecimal(uint64 number, byte* str) {
	int32 numberOfDigits = Log10(number);
	int32 index = numberOfDigits - 1;

	str[numberOfDigits] = 0;
	do {
		str[index] = '0' + (number % 10);
		number /= 10;
		index--;
	} while (number);
	return numberOfDigits;
}

int32 SignedToDecimal(int64 number, byte* str) {
	if (number < 0) {
		*str = '-';
		return UnsignedToDecimal((uint32)(-number), str + 1) + 1;
	}
	return UnsignedToDecimal((uint32)number, str);
}

// very naive implementation
int32 Float32ToDecimal(float32 number, int32 precision, byte* str) {
	int32 count = 0;

	// output sign
	if (number < 0) {
		*str = '-';
		number = -number;
		str++;
		count++;
	}

	// output integer part
	uint32 integer = (uint32)number; //potential overflowing in the general case
	number -= integer;
	int32 integerCount = UnsignedToDecimal(integer, str);
	count += integerCount;

	// output fraction part
	if (!precision) return count;
	str[integerCount] = '.';
	str += integerCount + 1;
	count++;
	while (precision) {
		number *= 10.0f;
		int32 digit = (uint32)number;
		*str = '0' + (char)digit;
		number -= digit;
		str++;
		count++;
		precision--;
	}
	*str = 0;
	return count;
}

#define COPY(source, dest) (memcpy(dest, source, sizeof(source)-1), sizeof(source)-1)

inline int32 BoolToAnsi(bool b, byte* str){
	if (b) {
		return COPY("true", str);
	}
	else {
		return COPY("false", str);
	}
}

// Length-based
//--------------

struct String {
	byte* data;
	ssize length;
};

#define STR(x) String{((byte*)x), (sizeof(x)-1)}

ssize StringCopy(String source, byte* dest) {
	memcpy(dest, source.data, source.length);
	return source.length;
}

bool StringEquals(String str1, String str2) {
	if (str1.length != str2.length) return false;
	for (ssize i = 0; i < str1.length; i++)
		if (str1.data[i] != str2.data[i]) return false;
	return true;
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

int64 ParseInt64(String str) {
	int64 result = 0;
	ssize start = str.data[0] == '-' ? 1 : 0;
	for (ssize i = start; i < str.length; i++) {
		char c = str.data[i];
		result *= 10;
		result += c-'0';
	}
	if (start == 1) result = -result;
	return result;
}

float64 ParseFloat64(String str) {
	float64 result = 0.0;
	ssize start = str.data[0] == '-' ? 1 : 0;
	bool radix = false;
	for (ssize i = start; i < str.length; i++) {
		char c = str.data[i];
		char digit = c-'0';
		if (c == '.') {
			radix = true;
			continue;     
		}
		if (c == 'e' || c == 'E') {
			String exp;
			ssize j = str.data[i+1] == '+' ? i+2 : i+1;
			exp.data = &str.data[j];
			exp.length = str.length - j;
			result *= Pow10((int32)ParseInt64(exp));
			break;
		}
		if (radix) {
			result += digit/10.0;
		}
		else {
			result *= 10.0;
			result += digit;
		}
	}
	if (start == 1) result = -result;
	return result;
}

// String Builder
//----------------

struct StringBuilder {
	byte* buffer;
	byte* ptr;

	StringBuilder operator()(String string) {
		StringBuilder concat = *this;
		concat.ptr += StringCopy(string, concat.ptr);
		return concat;
	}

	StringBuilder operator()(byte ch) {
		StringBuilder concat = *this;
		*(concat.ptr) = ch;
		concat.ptr++;
		return concat;
	}

	StringBuilder operator()(char ch) {
		StringBuilder concat = *this;
		*(concat.ptr) = (byte)ch;
		concat.ptr++;
		return concat;
	}

	StringBuilder operator()(int64 i) {
		StringBuilder concat = *this;
		concat.ptr += SignedToDecimal(i, concat.ptr);
		return concat;
	}

	StringBuilder operator()(uint32 i, char f = 'd') {
		StringBuilder concat = *this;
		if (f == 'd') concat.ptr += UnsignedToDecimal(i, concat.ptr);
		if (f == 'b') concat.ptr += Uint32ToBinary(i, concat.ptr);
		if (f == 'x') concat.ptr += Uint32ToHex(i, concat.ptr);
		return concat;
	}

	String operator()() {
		return {this->buffer, this->ptr - this->buffer};
	}
};

// StringList
//------------

struct StringNode {
	String string;
	StringNode* prev;
	StringNode* next;
};

struct StringList {
	StringNode* first;
	StringNode* last;
	ssize totalLength;
};

StringList CreateStringList(String string, StringNode* node) {
	*node = {string, NULL, NULL};
	return {node, node, string.length};
}

void StringListAppend(StringList* list, StringNode* node) {
	if (node->string.length == 0) return;
	LINKEDLIST_ADD(list, node);
	list->totalLength += node->string.length;
}

void StringListCopy(StringList list, byte* buffer) {
	byte* ptr = buffer;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		ptr += StringCopy(string, ptr);
	}
}

byte GetChar(StringList list, ssize index) {
	ssize i = 0;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		if (index < i + string.length) {
			return string.data[index - i];
		}
		i += string.length;
	}
	return 0;
}

ssize StringListGetIndexOf(StringList list, ssize start, ssize end, byte b) {
	ssize i = 0;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		if (string.length <= start - i) {
			i += string.length;
			continue;
		}
		for (ssize j = 0; j < string.length; j++, i++) {
			if (i == end) return -1;
			if (start <= i && string.data[j] == b)
				return i;
		}
	}
	return -1;
}

ssize StringListGetLastIndexOf(StringList list, ssize start, ssize end, byte b) {
	ssize i = 0;
	ssize result = -1;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		if (string.length <= start - i) {
			i += string.length;
			continue;
		}
		for (ssize j = 0; j < string.length; j++, i++) {
			if (i == end) return result;
			if (start <= i && string.data[j] == b)
				result = i;
		}
	}
	return result;
}

void StringListFindWord(StringList list, ssize index, ssize* start, ssize* end) {
	ssize i = 0;
	*start = 0;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize j = 0; j < string.length; j++, i++) {
			byte b = string.data[j];
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
	}
	*end = list.totalLength;
}