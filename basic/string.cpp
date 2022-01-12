// Null-Terminated
//-----------------

int32 Uint32ToBinary(uint32 number, _Out_ char* str) {
	int32 index = HighBit(number, 0);
	int32 numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		str[numberOfDigits - index - 1] = (number & (1 << index)) ? '1' : '0';
		index--;
	}
	return numberOfDigits;
}

int32 Uint32ToHex(uint32 number, _Out_ char* str) {
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

int32 Uint32ToDecimal(uint32 number, _Out_ byte* str) {
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

int32 Int32ToDecimal(int32 number, _Out_ byte* str) {
	if (number < 0) {
		*str = '-';
		return Uint32ToDecimal((uint32)(-number), str + 1) + 1;
	}
	return Uint32ToDecimal((uint32)number, str);
}

// very naive implementation
int32 Float32ToDecimal(float32 number, int32 precision, _Out_ byte* str) {
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

#define StringCopy(source, dest) (memcpy((dest), (source), (sizeof(source)-1)), (sizeof(source)-1))

inline int32 BoolToAnsi(bool b, void* str){
	if (b){
		return StringCopy("true", str);
	}
	else {
		return StringCopy("false", str);
	}
}

// Length-based
//--------------

struct String {
	byte* data;
	int64 length;
};

#define STR(x) String{((byte*)x), (sizeof(x)-1)}

#define StringCopy2(source, dest) (memcpy((dest), (source).data, (source).length), (source).length)

bool StringEquals(String str1, String str2) {
	if (str1.length != str2.length) return false;
	return (memcmp(str1.data, str2.data, str1.length) == 0);
}

int32 StringSplit(String text, String separator, String* buffer) {
	int32 j = 0;
	buffer[0].data = text.data;
	for (int32 i = 0; i < text.length - separator.length;) {
		if (memcmp(text.data+i, separator.data, separator.length) == 0) {
			buffer[j].length = text.data+i - buffer[j].data;
			j++;
			i+=(int32)separator.length;
			buffer[j].data = text.data+i;
		}
		else i++;
	}
	buffer[j].length = text.data+text.length - buffer[j].data;
	return j+1;
}

int64 AnsiToInt64(String str) {
	int64 result = 0;
	int32 start = str.data[0] == '-' ? 1 : 0;
	for (int32 i = start; i < str.length; i++) {
		char c = str.data[i];
		result *= 10;
		result += c-'0';
	}
	if (start == 1) result = -result;
	return result;
}

float64 AnsiToFloat64(String str) {
	float64 result = 0.0;
	int32 start = str.data[0] == '-' ? 1 : 0;
	bool radix = false;
	for (int32 i = start; i < str.length; i++) {
		char c = str.data[i];
		char digit = c-'0';
		if (c == '.') {
			radix = true;
			continue;     
		}
		if (c == 'e' || c == 'E') {
			String exp;
			int32 j = str.data[i+1] == '+' ? i+2 : i+1;
			exp.data = &str.data[j];
			exp.length = str.length - j;
			result *= pow10((int32)AnsiToInt64(exp));
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
};

String GetString(StringBuilder* builder) {
	return {builder->buffer, builder->ptr-builder->buffer};
}

void PushString(StringBuilder* builder, String str) {
	builder->ptr += StringCopy2(str, builder->ptr);
}

void PushNewLine(StringBuilder* builder) {
	*(builder->ptr) = 10;
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