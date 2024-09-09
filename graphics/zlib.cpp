// fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)
#define ZNSYMS 288 // number of symbols in literal/length alphabet

struct ZHuffman {
	uint16 fast[1 << ZFAST_BITS];
	uint16 firstcode[16];
	int32 maxcode[17];
	uint16 firstsymbol[16];
	byte size[ZNSYMS];
	uint16 value[ZNSYMS];
};

int32 ReverseBits16(int32 n) {
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

int32 ReverseBits(int32 v, int32 bits) {
	ASSERT(bits <= 16);
	// to bit reverse n bits, reverse 16 and shift
	// e.g. 11 bits, bit reverse and shift away 5
	return ReverseBits16(v) >> (16 - bits);
}

int32 zbuild_huffman(ZHuffman* z, const byte* sizelist, int32 num) {
	int32 i, k = 0;
	int32 code, next_code[16], sizes[17];

	// DEFLATE spec for generating codes
	memset(sizes, 0, sizeof(sizes));
	memset(z->fast, 0, sizeof(z->fast));
	for (i = 0; i < num; ++i)
		++sizes[sizelist[i]];
	sizes[0] = 0;
	for (i=1; i < 16; ++i)
		if (sizes[i] > (1 << i))
			return FAIL("bad sizes");
	code = 0;
	for (i = 1; i < 16; ++i) {
		next_code[i] = code;
		z->firstcode[i] = (uint16) code;
		z->firstsymbol[i] = (uint16) k;
		code = (code + sizes[i]);
		if (sizes[i])
			if (code-1 >= (1 << i)) return FAIL("bad codelengths");
		z->maxcode[i] = code << (16-i); // preshift for inner loop
		code <<= 1;
		k += sizes[i];
	}
	z->maxcode[16] = 0x10000; // sentinel
	for (i = 0; i < num; ++i) {
		int32 s = sizelist[i];
		if (s) {
			int32 c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
			uint16 fastv = (uint16) ((s << 9) | i);
			z->size [c] = (byte     ) s;
			z->value[c] = (uint16) i;
			if (s <= ZFAST_BITS) {
				int32 j = ReverseBits(next_code[s],s);
				while (j < (1 << ZFAST_BITS)) {
					z->fast[j] = fastv;
					j += (1 << s);
				}
			}
			++next_code[s];
		}
	}
	return 1;
}

// TODO: replace with Stream
struct ZBuf {
	byte* zbuffer;
	byte* zbuffer_end;
	int32 num_bits;
	uint32 code_buffer;

	byte* zout;
	byte* zout_start;
	byte* zout_end;
	int32 z_expandable;

	ZHuffman z_length, z_distance;
};

int32 zeof(ZBuf* z) {
	return (z->zbuffer >= z->zbuffer_end);
}

byte zget8(ZBuf* z) {
	return zeof(z) ? 0 : *z->zbuffer++;
}

void fill_bits(ZBuf* z) {
	do {
		if (z->code_buffer >= (1U << z->num_bits)) {
		  z->zbuffer = z->zbuffer_end;  /* treat this as EOF so we fail. */
		  return;
		}
		z->code_buffer |= (uint32) zget8(z) << z->num_bits;
		z->num_bits += 8;
	} while (z->num_bits <= 24);
}

uint32 zreceive(ZBuf* z, int32 n) {
	uint32 k;
	if (z->num_bits < n) fill_bits(z);
	k = z->code_buffer & ((1 << n) - 1);
	z->code_buffer >>= n;
	z->num_bits -= n;
	return k;
}

int32 ZHuffman_decode_slowpath(ZBuf* a, ZHuffman* z) {
	int32 b, s, k;
	// not resolved by fast table, so compute it the slow way
	// use jpeg approach, which requires MSbits at top
	k = ReverseBits(a->code_buffer, 16);
	for (s=ZFAST_BITS+1; ; ++s)
		if (k < z->maxcode[s])
			break;
	if (s >= 16) return -1; // invalid code!
	// code size is s, so:
	b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
	if (b >= ZNSYMS) return -1; // some data was corrupt somewhere!
	if (z->size[b] != s) return -1;  // was originally an assert, but report failure instead.
	a->code_buffer >>= s;
	a->num_bits -= s;
	return z->value[b];
}

int32 ZHuffman_decode(ZBuf* a, ZHuffman* z) {
	int32 b, s;
	if (a->num_bits < 16) {
		if (zeof(a)) {
			return -1;   /* report error for unexpected end of data. */
		}
		fill_bits(a);
	}
	b = z->fast[a->code_buffer & ZFAST_MASK];
	if (b) {
		s = b >> 9;
		a->code_buffer >>= s;
		a->num_bits -= s;
		return b & 511;
	}
	return ZHuffman_decode_slowpath(a, z);
}

int32 zexpand(Arena* arena, ZBuf* z, byte* zout, int32 n) { // need to make room for n bytes
	byte* q;
	uint32 cur, limit, old_limit;
	z->zout = zout;
	if (!z->z_expandable) return FAIL("output buffer limit");
	cur   = (uint32) (z->zout - z->zout_start);
	limit = old_limit = (uint32)(z->zout_end - z->zout_start);
	if (0xffffffff - cur < (uint32)n) return FAIL("Out of memory");
	while (cur + n > limit) {
		if(limit > 0xffffffff / 2) return FAIL("Out of memory");
		limit *= 2;
	}
	q = (byte*)ArenaReAlloc(arena, z->zout_start, old_limit, limit);
	if (q == NULL) return FAIL("Out of memory");
	z->zout_start = q;
	z->zout       = q + cur;
	z->zout_end   = q + limit;
	return 1;
}

static const int32 zlength_base[31] = {
	3,4,5,6,7,8,9,10,11,13,
	15,17,19,23,27,31,35,43,51,59,
	67,83,99,115,131,163,195,227,258,0,0 };

static const int32 zlength_extra[31]= { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int32 zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int32 zdist_extra[32] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

int32 parse_huffman_block(Arena* arena, ZBuf* a) {
	byte* zout = a->zout;
	for (;;) {
		int32 z = ZHuffman_decode(a, &a->z_length);
		if (z < 256) {
			if (z < 0) return FAIL("bad huffman code"); // error in huffman codes
			if (zout >= a->zout_end) {
				if (!zexpand(arena, a, zout, 1)) return 0;
				zout = a->zout;
			}
			*zout++ = (byte)z;
		}
		else {
			byte* p;
			int32 len,dist;
			if (z == 256) {
				a->zout = zout;
				return 1;
			}
			z -= 257;
			len = zlength_base[z];
			if (zlength_extra[z]) len += zreceive(a, zlength_extra[z]);
			z = ZHuffman_decode(a, &a->z_distance);
			if (z < 0) return FAIL("bad huffman code");
			dist = zdist_base[z];
			if (zdist_extra[z]) dist += zreceive(a, zdist_extra[z]);
			if (zout - a->zout_start < dist) return FAIL("bad dist");
			if (zout + len > a->zout_end) {
				if (!zexpand(arena, a, zout, len)) return 0;
				zout = a->zout;
			}
			p = (byte*)(zout - dist);
			if (dist == 1) { // run of one byte; common in images.
				byte v = *p;
				if (len) { do *zout++ = v; while (--len); }
			}
			else {
				if (len) { do *zout++ = *p++; while (--len); }
			}
		}
	}
}

int32 compute_huffman_codes(ZBuf* a) {
	static const byte length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
	ZHuffman z_codelength;
	byte lencodes[286+32+137];//padding for maximum single op
	byte codelength_sizes[19];
	int32 i, n;

	int32 hlit  = zreceive(a,5) + 257;
	int32 hdist = zreceive(a,5) + 1;
	int32 hclen = zreceive(a,4) + 4;
	int32 ntot  = hlit + hdist;

	memset(codelength_sizes, 0, sizeof(codelength_sizes));
	for (i = 0; i < hclen; ++i) {
		int32 s = zreceive(a,3);
		codelength_sizes[length_dezigzag[i]] = (byte) s;
	}
	if (!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

	n = 0;
	while (n < ntot) {
		int32 c = ZHuffman_decode(a, &z_codelength);
		if (c < 0 || c >= 19) return FAIL("bad codelengths");
		if (c < 16)
			lencodes[n++] = (byte) c;
		else {
			byte fill = 0;
			if (c == 16) {
				c = zreceive(a,2)+3;
				if (n == 0) return FAIL("bad codelengths");
				fill = lencodes[n-1];
			} else if (c == 17) {
				c = zreceive(a,3)+3;
			} else if (c == 18) {
				c = zreceive(a,7)+11;
			}
			else {
				return FAIL("bad codelengths");
			}
			if (ntot - n < c) return FAIL("bad codelengths");
			memset(lencodes+n, fill, c);
			n += c;
		}
	}
	if (n != ntot) return FAIL("bad codelengths");
	if (!zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
	if (!zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
	return 1;
}

int32 parse_uncompressed_block(Arena* arena, ZBuf* a) {
	byte header[4];
	int32 len, nlen, k;
	if (a->num_bits & 7)
		zreceive(a, a->num_bits & 7); // discard
	// drain the bit-packed data into header
	k = 0;
	while (a->num_bits > 0) {
		header[k++] = (byte) (a->code_buffer & 255); // suppress MSVC run-time check
		a->code_buffer >>= 8;
		a->num_bits -= 8;
	}
	if (a->num_bits < 0) 
		return FAIL("zlib corrupt");
	// now fill header the normal way
	while (k < 4)
		header[k++] = zget8(a);
	len  = header[1]*256 + header[0];
	nlen = header[3]*256 + header[2];
	if (nlen != (len ^ 0xffff)) 
		return FAIL("zlib corrupt");
	if (a->zbuffer + len > a->zbuffer_end) 
		return FAIL("read past buffer");
	if (a->zout + len > a->zout_end)
		if (!zexpand(arena, a, a->zout, len)) 
			return 0;
	memcpy(a->zout, a->zbuffer, len);
	a->zbuffer += len;
	a->zout += len;
	return 1;
}

int32 parse_zlib_header(ZBuf* a) {
	int32 cmf   = zget8(a);
	int32 cm    = cmf & 15;
	/* int32 cinfo = cmf >> 4; */
	int32 flg   = zget8(a);
	if (zeof(a)) return FAIL("bad zlib header"); // zlib spec
	if ((cmf*256+flg) % 31 != 0) return FAIL("bad zlib header"); // zlib spec
	if (flg & 32) return FAIL("no preset dict"); // preset dictionary not allowed in png
	if (cm != 8) return FAIL("bad compression"); // DEFLATE required for png
	// window = 1 << (8 + cinfo)... but who cares, we fully buffer output
	return 1;
}

static const byte zdefault_length[ZNSYMS] = {
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static const byte zdefault_distance[32] = {
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

int32 parse_zlib(Arena* arena, ZBuf* a, int32 parse_header) {
	int32 final, type;
	if (parse_header)
		if (!parse_zlib_header(a)) 
			return 0;
	a->num_bits = 0;
	a->code_buffer = 0;
	do {
		final = zreceive(a,1);
		type = zreceive(a,2);
		if (type == 0) {
			if (!parse_uncompressed_block(arena, a)) 
				return 0;
		} else if (type == 3) {
			return 0;
		}
		else {
			if (type == 1) {
				// use fixed code lengths
				if (!zbuild_huffman(&a->z_length  , zdefault_length  , ZNSYMS)) 
					return 0;
				if (!zbuild_huffman(&a->z_distance, zdefault_distance,  32)) 
					return 0;
			}
			else {
				if (!compute_huffman_codes(a)) 
					return 0;
			}
			if (!parse_huffman_block(arena, a))
			 return 0;
		}
	} while (!final);
	return 1;
}

int32 do_zlib(Arena* arena, ZBuf* a, byte* obuf, int32 olen, int32 exp, int32 parse_header) {
	a->zout_start = obuf;
	a->zout       = obuf;
	a->zout_end   = obuf + olen;
	a->z_expandable = exp;

	return parse_zlib(arena, a, parse_header);
}

byte* zlib_decode_malloc_guesssize_headerflag(Arena* arena, byte* buffer, int32 len, int32 initial_size, int32* outlen, int32 parse_header) {
	ZBuf a;
	byte* p = (byte*)ArenaAlloc(arena, initial_size);
	if (p == NULL) return NULL;
	a.zbuffer = (byte*)buffer;
	a.zbuffer_end = (byte*)buffer + len;
	if (do_zlib(arena, &a, p, initial_size, 1, parse_header)) {
		if (outlen) *outlen = (int) (a.zout - a.zout_start);
		return a.zout_start;
	}
	else {
		return NULL;
	}
}