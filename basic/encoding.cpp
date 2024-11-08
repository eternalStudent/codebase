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

// TODO: UTF8 to UTF16
ssize ASCIIToUTF16(String str, byte* buffer) {
	uint16* ptr = (uint16*)buffer;

	for (ssize i = 0; i < str.length; i++) {
		*ptr = (uint16)str.data[i];
		ptr++;
	}

	return 2*str.length;
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