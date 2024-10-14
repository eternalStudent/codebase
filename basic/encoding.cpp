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