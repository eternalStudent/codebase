// Null-Terminated
//-----------------

int32 Uint32ToBinary(uint32 number, char* str) {
	int32 index = HighBit(number, 0);
	int32 numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		str[numberOfDigits - index - 1] = (number & (1 << index)) ? '1' : '0';
		index--;
	}
	return numberOfDigits;
}

int32 Uint32ToHex(uint32 number, char* str) {
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

inline int32 GetNumberOfDecimalDigits(uint32 number) {
	if (number < 100000) {
		// up to 5 digits
		if (number < 1000) {
			// up to 3 digits
			if (number < 100) {
				// up to 2 digits
				if (number < 10)
					return 1;
				else
					return 2;
			}
			else
				return 3;
		}
		else {
			// more than 3 digits
			if (number < 10000)
				return 4;
			else
				return 5;
		}
	}
	else {
		//more than 5 digits
		if (number < 100000000) {
			// up to 8 digits
			if (number < 10000000) {
				// up to 7 digits
				if (number < 1000000)
					return 6;
				else
					return 7;
			}
			else
				return 8;
		}
		else {
			// more than 8 digits
			if (number < 1000000000)
				return 9;
			else
				return 10;
		}
	}
}

int32 Uint32ToDecimal(uint32 number, byte* str) {
	int32 numberOfDigits = GetNumberOfDecimalDigits(number);
	int32 index = numberOfDigits - 1;

	str[numberOfDigits] = 0;
	do {
		str[index] = '0' + (number % 10);
		number /= 10;
		index--;
	} while (number);
	return numberOfDigits;
}

int32 Int32ToDecimal(int32 number, byte* str) {
	if (number < 0) {
		*str = '-';
		return Uint32ToDecimal((uint32)(-number), str + 1) + 1;
	}
	return Uint32ToDecimal((uint32)number, str);
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
	int32 integerCount = Uint32ToDecimal(integer, str);
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
	if (b){
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
		this->ptr += StringCopy(string, this->ptr);
		return *this;
	}

	StringBuilder operator()(byte ch) {
		*(this->ptr) = ch;
		this->ptr++;
		return *this;
	}

	StringBuilder operator()(int32 i) {
		this->ptr += Int32ToDecimal(i, this->ptr);
		return *this;
	}

	String operator()() {
		return {this->buffer, this->ptr - this->buffer};
	}
};

String GetString(StringBuilder* builder) {
	return {builder->buffer, builder->ptr-builder->buffer};
}

void PushString(StringBuilder* builder, String str) {
	builder->ptr += StringCopy(str, builder->ptr);
}

void PushNewLine(StringBuilder* builder) {
	*(builder->ptr) = 10;
	builder->ptr+=1;
}

void PushChar(StringBuilder* builder, byte ch) {
	*(builder->ptr) = ch;
	builder->ptr+=1;
}

void PushBool(StringBuilder* builder, bool b) {
	builder->ptr += BoolToAnsi(b, builder->ptr);
}

void PushInt32(StringBuilder* builder, int32 i) {
	builder->ptr += Int32ToDecimal(i, builder->ptr);
}

void PushFloat32(StringBuilder* builder, float32 f, int32 precision) {
	builder->ptr += Float32ToDecimal(f, precision, builder->ptr);
}

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