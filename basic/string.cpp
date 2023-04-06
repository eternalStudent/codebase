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
	union {
		ssize totalLength;
		ssize length;
	};
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

struct StringListPos {
	StringNode* node;
	ssize index;
};

#define SL_START(list)  StringListPos{list.first, 0}
#define SL_END(list)  	StringListPos{list.last, list.last->string.length}

bool StringListIsStart(StringListPos pos) {
	return pos.node->prev == NULL && pos.index == 0;
}

bool StringListIsEnd(StringListPos pos) {
	return pos.node->next == NULL && pos.index == pos.node->string.length;
}

StringListPos StringListPosInc(StringListPos pos) {
	if (pos.index < pos.node->string.length - 1 ||
		 pos.index == pos.node->string.length - 1 && pos.node->next == NULL)

		pos.index++;
	else {
		pos.node = pos.node->next;
		pos.index = 0;
	}
	return pos;
}

void StringListPosInc(StringListPos* pos) {
	if (pos->index < pos->node->string.length - 1 ||
		 pos->index == pos->node->string.length - 1 && pos->node->next == NULL)

		pos->index++;
	else {
		pos->node = pos->node->next;
		pos->index = 0;
	}
}

StringListPos StringListPosDec(StringListPos pos) {
	if (pos.index > 0 ||
		 pos.index == 0 && pos.node->prev == NULL)

		pos.index--;
	else {
		pos.node = pos.node->prev;
		pos.index = pos.node->string.length - 1;
	}
	return pos;
}

void StringListPosDec(StringListPos* pos) {
	if (pos->index > 0 ||
		 pos->index == 0 && pos->node->prev == NULL)
		pos->index--;
	else {
		pos->node = pos->node->prev;
		pos->index = pos->node->string.length - 1;
	}
}

// NOTE: returns true if and only if a < b
bool StringListPosCompare(StringListPos a, StringListPos b) {
	if (a.node == b.node)
		return a.index < b.index;

	for (StringNode* node = a.node; node != NULL; node = node->next) {
		if (node == b.node) return true;
	}

	return false;
}

ssize StringListCopy(StringListPos start, StringListPos end, byte* buffer) {
	byte* ptr = buffer;
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;
		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		ptr += StringCopy({string.data + startIndex, endIndex - startIndex}, ptr);

		if (node == end.node) break;
	}
	return ptr - buffer;
}

StringListPos StringListGetFirstPosOf(StringListPos start, StringListPos end, byte b) {
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;
		
		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		String substring = {string.data + startIndex, endIndex - startIndex};

		ssize index = StringGetFirstIndexOf(substring, b);
		if (index != string.length) return {node, startIndex + index};

		if (node == end.node) break;
	}

	return end;
}

StringListPos StringListGetLastPosOf(StringListPos start, StringListPos end, byte b) {
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;

		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		String substring = {string.data + startIndex, endIndex - startIndex};
		
		ssize index = StringGetLastIndexOf(substring, b);
		if (index != -1) return {node, startIndex + index};

		if (node == end.node) break;
	}

	return StringListPosDec(start);
}

void StringListFindWord(StringList list, StringListPos pos, StringListPos* start, StringListPos* end) {
	*start = SL_START(list);
	bool afterPos = false;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool alphaNumeric = 'a' <= b && b <= 'z' || 'A' <= b && b <= 'Z' || '0' <= b && b <= '9' || b == '_';
			if (pos.node == node && pos.index == i) {
				if (!alphaNumeric) {
					*start = {node, i};
					*end = {node, i + 1};
					return;
				}
				afterPos = true;
				continue;
			}

			if (!afterPos && !alphaNumeric) {
				*start = {node, i + 1};
			}
			if (afterPos && !alphaNumeric) {
				*end = {node, i};
				return;
			}
		}
	}
	*end = SL_END(list);
}