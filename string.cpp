struct String {
	byte* data;
	int64 length;
};

typedef String Slice;

#define STR(x) String{((byte*)x), (sizeof(x)-1)}

int32 uint32ToBinary(uint32 number, _Out_ char* str) {
	int32 index = HighBit(number, 0);
	int32 numberOfDigits = index + 1;

	str[numberOfDigits] = 0;
	while (index + 1) {
		str[numberOfDigits - index - 1] = (number & (1 << index)) ? '1' : '0';
		index--;
	}
	return numberOfDigits;
}

int32 uint32ToHex(uint32 number, _Out_ char* str) {
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

inline int32 getNumberOfDecimalDigits(uint32 number) {
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

int32 uint32ToDecimal(uint32 number, _Out_ byte* str) {
	int32 numberOfDigits = getNumberOfDecimalDigits(number);
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
		return uint32ToDecimal((uint32)(-number), str + 1) + 1;
	}
	return uint32ToDecimal((uint32)number, str);
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
	int32 integerCount = uint32ToDecimal(integer, str);
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

#define CopyString(source, dest) (memcpy((dest), (source), (sizeof(source)-1)), (sizeof(source)-1))

inline int32 BoolToAnsi(bool b, char* str){
	if (b){
		return CopyString("true", str) - 1;
	}
	else{
		return CopyString("false", str) - 1;
	}
}