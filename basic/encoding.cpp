ssize UTF8Decode(byte* data, byte* end, uint32* codepoint) {
	static byte lengths[] = {
		1, 1, 1, 1, // 0xxxx  
		1, 1, 1, 1,         
		1, 1, 1, 1,         
		1, 1, 1, 1,         
		0, 0, 0, 0, // 10xxx
		0, 0, 0, 0,         
		2, 2, 2, 2, // 110xx  
		3, 3,       // 1110x
		4,          // 11110
		0,          // 11111
	};

	if (data == end)
		return 0;
	
	byte b = *data;
	byte l = lengths[b >> 3];
	if (l == 0 || end - data < l) {
		// NOTE: error, emit replacement character
		*codepoint = 0xFFFD;
		return 1;
	}

	static byte firstByteMask[] = {0, 0x7F, 0x1F, 0x0F, 0x07};
	static byte finalShift[]    = {0,   18,   12,    6,    0};

	// NOTE: this is an interesting trick, read the bytes in 0, 3, 2, 1 order
	//       to avoid branching

	*codepoint = (b & firstByteMask[l]) << 18;
	switch (l) {
		case 4: *codepoint |= ((data[3] & 0x3F) << 0);
		case 3: *codepoint |= ((data[2] & 0x3F) << 6);
		case 2: *codepoint |= ((data[1] & 0x3F) << 12);
		default: break;
	}
	*codepoint >>= finalShift[l];
	
	return l;
}

inline bool IsWhiteSpace(uint32 codepoint) {
	return (9 <= codepoint && codepoint <= 13 || codepoint == 32);
}

ssize CodepointToUTF8(uint32 codepoint, byte* buffer) {
	if (codepoint < 0x80) {
		buffer[0] = (byte)codepoint;
		return 1;
	}

	if (codepoint < 0x0800) {
		// 0000,0xxx,yyyy,zzzz ==>
		// 110x,xxyy,10yy,zzzz
		buffer[0] = 0xC0 | (byte)((codepoint & 0x07C0) >> 6);
		buffer[1] = 0x80 | (byte)(codepoint & 0x003F);
		return 2;
	}

	if (codepoint < 0x010000) {
		// 0000,0000, wwww,xxxx, yyyy,zzzz ==>
		// 1110,wwww, 10xx,xxyy, 10yy,zzzz
		buffer[0] = 0xE0 | (byte)((codepoint & 0xF000) >> 12);
		buffer[1] = 0x80 | (byte)((codepoint & 0x0FC0) >> 6);
		buffer[2] = 0x80 | (byte)(codepoint & 0x003F);
		return 3;
	}

	// 0000,0000, 000u,vvvv, wwww,xxxx, yyyy,zzzz ==>
	// 1111,0uvv, 10vv,wwww, 10xx,xxyy, 10yy,zzzz
	buffer[0] = 0xF0 | (byte)((codepoint & 0x1C0000) >> 18);
	buffer[1] = 0x80 | (byte)((codepoint & 0x03F000) >> 12);
	buffer[2] = 0x80 | (byte)((codepoint & 0x000FC0) >> 6);
	buffer[3] = 0x80 | (byte)(codepoint & 0x00003F);
	return 4;
}

ssize CodepointToUTF16(uint32 codepoint, byte* buffer) {
	uint16* bufferW = (uint16*)buffer;
	if (codepoint < 0x10000) {
		*bufferW = (uint16)codepoint;
		return 2;
	}

	uint32 u = codepoint - 0x10000;
	bufferW[0] = 0xD800 | (uint16)((u & 0x0FFC00) >> 10);
	bufferW[1] = 0xDC00 | (uint16)(u & 0x0003FF);
	return 4;
}

ssize UTF8ToUTF16(String str, byte* buffer) {
	ssize total = 0;
	ssize length;
	uint32 codepoint;
	for (byte* ptr = str.data; ptr < str.data + str.length; ptr += length) {
		length = UTF8Decode(ptr, str.data + str.length, &codepoint);
		total += CodepointToUTF16(codepoint, buffer + total);
	}

	return total;
}

// Escaping
//------------

bool IsEnclosing(byte b) {
	return b == '\"' || b == '\'' || b == '`';
}

ssize ParseString(String string, byte* buffer) {
	byte enclosing = string.data[0];

	if (!IsEnclosing(enclosing)) {
		// NOTE: assume verbatims
		memcpy(buffer, string.data, string.length);
		return string.length;
	}

	ssize length = 0;
	for (ssize i = 1; i < string.length; i++) {
		byte b = string.data[i];
		if (b == enclosing) {
			break;
		}

		if (b == '\\') {
			i++;
			b = string.data[i];
			switch (b) {
			case 'a': {buffer[length++] = 7;} break;
			case 'b': {buffer[length++] = 8;} break;
			case 't': {buffer[length++] = 9;} break;
			case 'n': {buffer[length++] = 10;} break;
			case 'v': {buffer[length++] = 11;} break;
			case 'f': {buffer[length++] = 12;} break;
			case 'r': {buffer[length++] = 13;} break;
			case 'e': {buffer[length++] = 27;} break;
			case 'x': {
				byte nibble1;
				if (TryParseHexDigit(string.data[i + 1], &nibble1)) {
					byte nibble2;
					if (TryParseHexDigit(string.data[i + 1], &nibble2)) {
						i += 2;
						buffer[length++] = (nibble1 << 4) | nibble2;
					}
					else {
						i++;
						buffer[length++] = nibble1;
					}
				}
				else {
					buffer[length++] = 'x';
				}
			} break;
			case 'u': {
				uint32 codepoint;
				if (TryParseHex32({string.data + i + 1, 4}, &codepoint)) {
					length += CodepointToUTF8(codepoint, buffer + length);
					i += 4;
				}
				else {
					buffer[length++] = 'u';
				}
			} break;
			case 'U': {
				uint32 codepoint;
				if (TryParseHex32({string.data + i + 1, 8}, &codepoint)) {
					length += CodepointToUTF8(codepoint, buffer + length);
					i += 8;
				}
				else {
					buffer[length++] = 'U';
				}
			} break;
			default: {
				buffer[length++] = b;
			} break;
			}
		}
		else {
			buffer[length++] = b;
		}
	}

	return length;
}

ssize EscapeString(String string, byte enclosing, byte* buffer) {
	buffer[0] = enclosing;
	ssize length = 1;
	static byte symbols[] = {'a', 'b', 't', 'n', 'v', 'f', 'r'};
	for (ssize i = 0; i < string.length; i++) {
		byte b = string.data[i];
		if (7 <= b && b <= 13) {
			buffer[length++] = '\\';
			buffer[length++] = symbols[b - 7];	
		}
		else if (b == 27) {
			buffer[length++] = '\\';
			buffer[length++] = 'e';
		} 
		else if (b == enclosing) {
			buffer[length++] = '\\';
			buffer[length++] = b;
		}
		else if (32 <= b && b <= 126) {
			buffer[length++] = b;
		}
		else {
			buffer[length++] = '\\';
			buffer[length++] = 'x';
			length += ToLowerHex(b, buffer + length);
		}
	}
	buffer[length++] = enclosing;
	return length;
}