// Compact Font Format table
//--------------------------

Stream cff_get_index(Stream* data) {
   int64 count, start;
   int32 offsize;
   start = GetPosition(data);
   count = ReadUint16BigEndian(data);
   if (count) {
      offsize = ReadByte(data);
      Assert(offsize >= 1 && offsize <= 4);
      Skip(data, offsize * count);
      Skip(data, ReadUintNBigEndian(data, offsize) - 1);
   }
   return Subrange(data, start, GetPosition(data) - start);
}

uint32 cff_int(Stream* data) {
   int32 b0 = ReadByte(data);
   if (b0 >= 32 && b0 <= 246)       return b0 - 139;
   else if (b0 >= 247 && b0 <= 250) return (b0 - 247)*256 + ReadByte(data) + 108;
   else if (b0 >= 251 && b0 <= 254) return -(b0 - 251)*256 - ReadByte(data) - 108;
   else if (b0 == 28)               return ReadUint16BigEndian(data);
   else if (b0 == 29)               return ReadUint32BigEndian(data);
   Assert(0);
   return 0;
}

void cff_skip_operand(Stream* data) {
   int32 v, b0 = PeekByte(data);
   Assert(b0 >= 28);
   if (b0 == 30) {
      Skip(data, 1);
      while (GetPosition(data) < data->length) {
         v = ReadByte(data);
         if ((v & 0xF) == 0xF || (v >> 4) == 0xF)
            break;
      }
   } else {
      cff_int(data);
   }
}

Stream dict_get(Stream* data, int32 key) {
   Seek(data, 0);
   while (GetPosition(data) < data->length) {
      int64 start = GetPosition(data), end, op;
      while (PeekByte(data) >= 28)
         cff_skip_operand(data);
      end = GetPosition(data);
      op = ReadByte(data);
      if (op == 12)  op = ReadByte(data) | 0x100;
      if (op == key) return Subrange(data, start, end-start);
   }
   return Subrange(data, 0, 0);
}

void dict_get_ints(Stream* data, int32 key, int32 outcount, uint32* out) {
   int32 i;
   Stream operands = dict_get(data, key);
   for (i = 0; i < outcount && GetPosition(operands) < operands.length; i++)
      out[i] = cff_int(&operands);
}

Stream cff_index_get(Stream data, int32 i) {
   int32 count, offsize;
   int64 start, end;
   Seek(&data, 0);
   count = ReadUint16BigEndian(&data);
   offsize = ReadByte(&data);
   Assert(i >= 0 && i < count);
   Assert(offsize >= 1 && offsize <= 4);
   Skip(&data, i*offsize);
   start = ReadUintNBigEndian(&data, offsize);
   end = ReadUintNBigEndian(&data, offsize);
   return Subrange(&data, 2+(count+1)*offsize+start, end - start);
}

Stream get_subrs(Stream cff, Stream fontdict) {
   uint32 subrsoff = 0, private_loc[2] = { 0, 0 };
   Stream pdict;
   dict_get_ints(&fontdict, 18, 2, private_loc);
   if (!private_loc[1] || !private_loc[0]) return {};
   pdict = Subrange(&cff, private_loc[1], private_loc[0]);
   dict_get_ints(&pdict, 19, 1, &subrsoff);
   if (!subrsoff) return {};
   Seek(&cff, private_loc[1]+subrsoff);
   return cff_get_index(&cff);
}

int32 cff_index_count(Stream* data) {
   Seek(data, 0);
   return ReadUint16BigEndian(data);
}

Stream get_subr(Stream idx, int n) {
   int32 count = cff_index_count(&idx);
   int32 bias = 107;
   if (count >= 33900)
      bias = 32768;
   else if (count >= 1240)
      bias = 1131;
   n += bias;
   if (n < 0 || n >= count)
      return {};
   return cff_index_get(idx, n);
}

struct FontInfo {
   byte*  data;                // pointer to .ttf file
   int32  fontstart;           // offset of start of font

   int32 numGlyphs;            // number of glyphs, needed for range checking

   int32 loca,head,glyf,hhea,hmtx,kern,gpos,svg; // table locations as offset from start of .ttf
   int32 index_map;            // a cmap mapping for our chosen character encoding
   int32 indexToLocFormat;     // format needed to map from glyph index to glyph

   Stream cff;                    // cff font data
   Stream charstrings;            // the charstring index
   Stream gsubrs;                 // global charstring subroutines index
   Stream subrs;                  // private charstring subroutines index
   Stream fontdicts;              // array of font dicts
   Stream fdselect;               // map from glyph to fontdict
};

Stream cid_get_glyph_subrs(const FontInfo* info, int32 glyph_index) {
   Stream fdselect = info->fdselect;
   int32 nranges, start, end, v, fmt, fdselector = -1, i;

   Seek(&fdselect, 0);
   fmt = ReadByte(&fdselect);
   if (fmt == 0) {
      // untested
      Skip(&fdselect, glyph_index);
      fdselector = ReadByte(&fdselect);
   } else if (fmt == 3) {
      nranges = ReadUint16BigEndian(&fdselect);
      start = ReadUint16BigEndian(&fdselect);
      for (i = 0; i < nranges; i++) {
         v = ReadByte(&fdselect);
         end = ReadUint16BigEndian(&fdselect);
         if (glyph_index >= start && glyph_index < end) {
            fdselector = v;
            break;
         }
         start = end;
      }
   }
   if (fdselector == -1) return {};
   return get_subrs(info->cff, cff_index_get(info->fontdicts, fdselector));
}

#define __CHAR(p)    (* (char*) (p))
#define __BYTE(p)    (* (byte*) (p))

int16 __SHORT(byte* p) {
   return p[0]*256 + p[1]; 
}

uint16 __USHORT(byte* p) { 
   return p[0]*256 + p[1]; 
}

uint32 __ULONG(byte* p) {
   return (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3]; 
}

#define tag4(p,c0,c1,c2,c3) ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define tag(p,str)           tag4(p,str[0],str[1],str[2],str[3])

uint32 find_table(byte* data, uint32 fontstart, const char* tag) {
   int32 num_tables = __USHORT(data+fontstart+4);
   uint32 tabledir = fontstart + 12;
   int32 i;
   for (i=0; i < num_tables; ++i) {
      uint32 loc = tabledir + 16*i;
      if (tag(data+loc+0, tag))
         return __ULONG(data+loc+8);
   }
   return 0;
}

enum { 
   PLATFORM_ID_UNICODE   =0,
   PLATFORM_ID_MAC       =1,
   PLATFORM_ID_ISO       =2,
   PLATFORM_ID_MICROSOFT =3
};

enum { 
   MS_EID_SYMBOL        =0,
   MS_EID_UNICODE_BMP   =1,
   MS_EID_SHIFTJIS      =2,
   MS_EID_UNICODE_FULL  =10
};

int32 GetFontInfo(FontInfo* info, byte* buffer, int32 fontstart) {
   uint32 cmap, t;
   int32 i,numTables;

   info->data = buffer;
   info->fontstart = fontstart;
   info->cff = {};

   cmap = find_table(buffer, fontstart, "cmap");       // required
   info->loca = find_table(buffer, fontstart, "loca"); // required
   info->head = find_table(buffer, fontstart, "head"); // required
   info->glyf = find_table(buffer, fontstart, "glyf"); // required
   info->hhea = find_table(buffer, fontstart, "hhea"); // required
   info->hmtx = find_table(buffer, fontstart, "hmtx"); // required
   info->kern = find_table(buffer, fontstart, "kern"); // not required
   info->gpos = find_table(buffer, fontstart, "GPOS"); // not required

   if (!cmap || !info->head || !info->hhea || !info->hmtx)
      return 0;
   if (info->glyf) {
      // required for truetype
      if (!info->loca) return 0;
   } else {
      // initialization for CFF / Type2 fonts (OTF)
      Stream data, topdict, topdictidx;
      uint32 cstype = 2, charstrings = 0, fdarrayoff = 0, fdselectoff = 0;
      uint32 cff;

      cff = find_table(buffer, fontstart, "CFF ");
      if (!cff) return 0;

      info->fontdicts = {};
      info->fdselect = {};

      // @TODO this should use size from table (not 512MB)
      info->cff = CreateNewStream(buffer+cff, 512*1024*1024);
      data = info->cff;

      // read the header
      Skip(&data, 2);
      Seek(&data, ReadByte(&data)); // hdrsize

      // @TODO the name INDEX could list multiple fonts,
      // but we just use the first one.
      cff_get_index(&data);  // name INDEX
      topdictidx = cff_get_index(&data);
      topdict = cff_index_get(topdictidx, 0);
      cff_get_index(&data);  // string INDEX
      info->gsubrs = cff_get_index(&data);

      dict_get_ints(&topdict, 17, 1, &charstrings);
      dict_get_ints(&topdict, 0x100 | 6, 1, &cstype);
      dict_get_ints(&topdict, 0x100 | 36, 1, &fdarrayoff);
      dict_get_ints(&topdict, 0x100 | 37, 1, &fdselectoff);
      info->subrs = get_subrs(data, topdict);

      // we only support Type 2 charstrings
      if (cstype != 2) return 0;
      if (charstrings == 0) return 0;

      if (fdarrayoff) {
         // looks like a CID font
         if (!fdselectoff) return 0;
         Seek(&data, fdarrayoff);
         info->fontdicts = cff_get_index(&data);
         info->fdselect = Subrange(&data, fdselectoff, data.length-fdselectoff);
      }

      Seek(&data, charstrings);
      info->charstrings = cff_get_index(&data);
   }

   t = find_table(buffer, fontstart, "maxp");
   if (t)
      info->numGlyphs = __USHORT(buffer+t+4);
   else
      info->numGlyphs = 0xffff;

   info->svg = -1;

   // find a cmap encoding table we understand *now* to avoid searching
   // later. (todo: could make this installable)
   // the same regardless of glyph.
   numTables = __USHORT(buffer + cmap + 2);
   info->index_map = 0;
   for (i=0; i < numTables; ++i) {
      uint32 encoding_record = cmap + 4 + 8 * i;
      // find an encoding we understand:
      switch(__USHORT(buffer+encoding_record)) {
         case PLATFORM_ID_MICROSOFT:
            switch (__USHORT(buffer+encoding_record+2)) {
               case MS_EID_UNICODE_BMP:
               case MS_EID_UNICODE_FULL:
                  // MS/Unicode
                  info->index_map = cmap + __ULONG(buffer+encoding_record+4);
                  break;
            }
            break;
        case PLATFORM_ID_UNICODE:
            // Mac/iOS has these
            // all the encodingIDs are unicode, so we don't bother to check it
            info->index_map = cmap + __ULONG(buffer+encoding_record+4);
            break;
      }
   }
   if (info->index_map == 0)
      return 0;

   info->indexToLocFormat = __USHORT(buffer+info->head + 50);
   return 1;
}

float32 ScaleForPixelHeight(const FontInfo* info, float32 height) {
   int32 fheight = __SHORT(info->data + info->hhea + 4) - __SHORT(info->data + info->hhea + 6);
   return (float32) height / fheight;
}

int32 FindGlyphIndex(const FontInfo* info, int32 unicode_codepoint) {
   byte* data = info->data;
   uint32 index_map = info->index_map;

   uint16 format = __USHORT(data + index_map + 0);
   if (format == 0) { // apple byte encoding
      int32 bytes = __USHORT(data + index_map + 2);
      if (unicode_codepoint < bytes-6)
         return __BYTE(data + index_map + 6 + unicode_codepoint);
      return 0;
   } else if (format == 6) {
      uint32 first = __USHORT(data + index_map + 6);
      uint32 count = __USHORT(data + index_map + 8);
      if ((uint32) unicode_codepoint >= first && (uint32) unicode_codepoint < first+count)
         return __USHORT(data + index_map + 10 + (unicode_codepoint - first)*2);
      return 0;
   } else if (format == 2) {
      Assert(0); // @TODO: high-byte mapping for japanese/chinese/korean
      return 0;
   } else if (format == 4) { // standard mapping for windows fonts: binary search collection of ranges
      uint16 segcount = __USHORT(data+index_map+6) >> 1;
      uint16 searchRange = __USHORT(data+index_map+8) >> 1;
      uint16 entrySelector = __USHORT(data+index_map+10);
      uint16 rangeShift = __USHORT(data+index_map+12) >> 1;

      // do a binary search of the segments
      uint32 endCount = index_map + 14;
      uint32 search = endCount;

      if (unicode_codepoint > 0xffff)
         return 0;

      // they lie from endCount .. endCount + segCount
      // but searchRange is the nearest power of two, so...
      if (unicode_codepoint >= __USHORT(data + search + rangeShift*2))
         search += rangeShift*2;

      // now decrement to bias correctly to find smallest
      search -= 2;
      while (entrySelector) {
         uint16 end;
         searchRange >>= 1;
         end = __USHORT(data + search + searchRange*2);
         if (unicode_codepoint > end)
            search += searchRange*2;
         --entrySelector;
      }
      search += 2;

      {
         uint16 offset, start;
         uint16 item = (uint16) ((search - endCount) >> 1);

         Assert(unicode_codepoint <= __USHORT(data + endCount + 2*item));
         start = __USHORT(data + index_map + 14 + segcount*2 + 2 + 2*item);
         if (unicode_codepoint < start)
            return 0;

         offset = __USHORT(data + index_map + 14 + segcount*6 + 2 + 2*item);
         if (offset == 0)
            return (uint16) (unicode_codepoint + __SHORT(data + index_map + 14 + segcount*4 + 2 + 2*item));

         return __USHORT(data + offset + (unicode_codepoint-start)*2 + index_map + 14 + segcount*6 + 2 + 2*item);
      }
   } else if (format == 12 || format == 13) {
      uint32 ngroups = __ULONG(data+index_map+12);
      int32 low,high;
      low = 0; high = (int32)ngroups;
      // Binary search the right group.
      while (low < high) {
         int32 mid = low + ((high-low) >> 1); // rounds down, so low <= mid < high
         uint32 start_char = __ULONG(data+index_map+16+mid*12);
         uint32 end_char = __ULONG(data+index_map+16+mid*12+4);
         if ((uint32) unicode_codepoint < start_char)
            high = mid;
         else if ((uint32) unicode_codepoint > end_char)
            low = mid+1;
         else {
            uint32 start_glyph = __ULONG(data+index_map+16+mid*12+8);
            if (format == 12)
               return start_glyph + unicode_codepoint-start_char;
            else // format == 13
               return start_glyph;
         }
      }
      return 0; // not found
   }
   // @TODO
   Assert(0);
   return 0;
}

void GetGlyphHMetrics(const FontInfo* info, int32 glyph_index, int32* advanceWidth, 
                      int32* leftSideBearing) {
   uint16 numOfLongHorMetrics = __USHORT(info->data+info->hhea + 34);
   if (glyph_index < numOfLongHorMetrics) {
      if (advanceWidth)     *advanceWidth    = __SHORT(info->data + info->hmtx + 4*glyph_index);
      if (leftSideBearing)  *leftSideBearing = __SHORT(info->data + info->hmtx + 4*glyph_index + 2);
   } else {
      if (advanceWidth)     *advanceWidth    = __SHORT(info->data + info->hmtx + 4*(numOfLongHorMetrics-1));
      if (leftSideBearing)  *leftSideBearing = __SHORT(info->data + info->hmtx + 4*numOfLongHorMetrics + 2*(glyph_index - numOfLongHorMetrics));
   }
}

struct vertex {
   int16 x,y,cx,cy,cx1,cy1;
   byte type,padding;
};

struct csctx {
   int32 bounds;
   int32 started;
   float32 first_x, first_y;
   float32 x, y;
   int32 min_x, max_x, min_y, max_y;

   vertex* pvertices;
   int32 num_vertices;
};

#define CSCTX_INIT(bounds) {bounds,0, 0,0, 0,0, 0,0,0,0, NULL, 0}

void track_vertex(csctx* c, int32 x, int32 y) {
   if (x > c->max_x || !c->started) c->max_x = x;
   if (y > c->max_y || !c->started) c->max_y = y;
   if (x < c->min_x || !c->started) c->min_x = x;
   if (y < c->min_y || !c->started) c->min_y = y;
   c->started = 1;
}

void setvertex(vertex *v, byte type, int32 x, int32 y, int32 cx, int32 cy) {
   v->type = type;
   v->x = (int16) x;
   v->y = (int16) y;
   v->cx = (int16) cx;
   v->cy = (int16) cy;
}

enum {
   vmove=1,
   vline,
   vcurve,
   vcubic
};

void csctx_v(csctx* c, byte type, int32 x, int32 y, int32 cx, int32 cy, 
             int32 cx1, int32 cy1) {
   if (c->bounds) {
      track_vertex(c, x, y);
      if (type == vcubic) {
         track_vertex(c, cx, cy);
         track_vertex(c, cx1, cy1);
      }
   } else {
      setvertex(&c->pvertices[c->num_vertices], type, x, y, cx, cy);
      c->pvertices[c->num_vertices].cx1 = (int16) cx1;
      c->pvertices[c->num_vertices].cy1 = (int16) cy1;
   }
   c->num_vertices++;
}

void csctx_close_shape(csctx* ctx) {
   if (ctx->first_x != ctx->x || ctx->first_y != ctx->y)
      csctx_v(ctx, vline, (int32)ctx->first_x, (int32)ctx->first_y, 0, 0, 0, 0);
}

void csctx_rmove_to(csctx* ctx, float32 dx, float32 dy) {
   csctx_close_shape(ctx);
   ctx->first_x = ctx->x = ctx->x + dx;
   ctx->first_y = ctx->y = ctx->y + dy;
   csctx_v(ctx, vmove, (int32)ctx->x, (int32)ctx->y, 0, 0, 0, 0);
}

void csctx_rline_to(csctx* ctx, float32 dx, float32 dy) {
   ctx->x += dx;
   ctx->y += dy;
   csctx_v(ctx, vline, (int32)ctx->x, (int32)ctx->y, 0, 0, 0, 0);
}

void csctx_rccurve_to(csctx* ctx, 
                      float32 dx1, float32 dy1, 
                      float32 dx2, float32 dy2, 
                      float32 dx3, float32 dy3) {
   float32 cx1 = ctx->x + dx1;
   float32 cy1 = ctx->y + dy1;
   float32 cx2 = cx1 + dx2;
   float32 cy2 = cy1 + dy2;
   ctx->x = cx2 + dx3;
   ctx->y = cy2 + dy3;
   csctx_v(ctx, vcubic, (int32)ctx->x, (int32)ctx->y, (int32)cx1, (int32)cy1, (int32)cx2, (int32)cy2);
}

int32 run_charstring(const FontInfo* info, int32 glyph_index, csctx* c) {
   int32 in_header = 1, maskbits = 0, subr_stack_height = 0, sp = 0, v, i, b0;
   int32 has_subrs = 0, clear_stack;
   float32 s[48];
   Stream subr_stack[10], subrs = info->subrs, data;
   float32 f;

   // this currently ignores the initial width value, which isn't needed if we have hmtx
   data = cff_index_get(info->charstrings, glyph_index);
   while (GetPosition(data) < data.length) {
      i = 0;
      clear_stack = 1;
      b0 = ReadByte(&data);
      switch (b0) {
      // @TODO implement hinting
      case 0x13: // hintmask
      case 0x14: // cntrmask
         if (in_header)
            maskbits += (sp / 2); // implicit "vstem"
         in_header = 0;
         Skip(&data, (maskbits + 7) / 8);
         break;

      case 0x01: // hstem
      case 0x03: // vstem
      case 0x12: // hstemhm
      case 0x17: // vstemhm
         maskbits += (sp / 2);
         break;

      case 0x15: // rmoveto
         in_header = 0;
         if (sp < 2) return Fail("rmoveto stack");
         csctx_rmove_to(c, s[sp-2], s[sp-1]);
         break;
      case 0x04: // vmoveto
         in_header = 0;
         if (sp < 1) return Fail("vmoveto stack");
         csctx_rmove_to(c, 0, s[sp-1]);
         break;
      case 0x16: // hmoveto
         in_header = 0;
         if (sp < 1) return Fail("hmoveto stack");
         csctx_rmove_to(c, s[sp-1], 0);
         break;

      case 0x05: // rlineto
         if (sp < 2) return Fail("rlineto stack");
         for (; i + 1 < sp; i += 2)
            csctx_rline_to(c, s[i], s[i+1]);
         break;

      // hlineto/vlineto and vhcurveto/hvcurveto alternate horizontal and vertical
      // starting from a different place.

      case 0x07: // vlineto
         if (sp < 1) return Fail("vlineto stack");
         goto vlineto;
      case 0x06: // hlineto
         if (sp < 1) return Fail("hlineto stack");
         for (;;) {
            if (i >= sp) break;
            csctx_rline_to(c, s[i], 0);
            i++;
      vlineto:
            if (i >= sp) break;
            csctx_rline_to(c, 0, s[i]);
            i++;
         }
         break;

      case 0x1F: // hvcurveto
         if (sp < 4) return Fail("hvcurveto stack");
         goto hvcurveto;
      case 0x1E: // vhcurveto
         if (sp < 4) return Fail("vhcurveto stack");
         for (;;) {
            if (i + 3 >= sp) break;
            csctx_rccurve_to(c, 0, s[i], s[i+1], s[i+2], s[i+3], (sp - i == 5) ? s[i + 4] : 0.0f);
            i += 4;
      hvcurveto:
            if (i + 3 >= sp) break;
            csctx_rccurve_to(c, s[i], 0, s[i+1], s[i+2], (sp - i == 5) ? s[i+4] : 0.0f, s[i+3]);
            i += 4;
         }
         break;

      case 0x08: // rrcurveto
         if (sp < 6) return Fail("rcurveline stack");
         for (; i + 5 < sp; i += 6)
            csctx_rccurve_to(c, s[i], s[i+1], s[i+2], s[i+3], s[i+4], s[i+5]);
         break;

      case 0x18: // rcurveline
         if (sp < 8) return Fail("rcurveline stack");
         for (; i + 5 < sp - 2; i += 6)
            csctx_rccurve_to(c, s[i], s[i+1], s[i+2], s[i+3], s[i+4], s[i+5]);
         if (i + 1 >= sp) return Fail("rcurveline stack");
         csctx_rline_to(c, s[i], s[i+1]);
         break;

      case 0x19: // rlinecurve
         if (sp < 8) return Fail("rlinecurve stack");
         for (; i + 1 < sp - 6; i += 2)
            csctx_rline_to(c, s[i], s[i+1]);
         if (i + 5 >= sp) return Fail("rlinecurve stack");
         csctx_rccurve_to(c, s[i], s[i+1], s[i+2], s[i+3], s[i+4], s[i+5]);
         break;

      case 0x1A: // vvcurveto
      case 0x1B: // hhcurveto
         if (sp < 4) return Fail("(vv|hh)curveto stack");
         f = 0.0;
         if (sp & 1) { f = s[i]; i++; }
         for (; i + 3 < sp; i += 4) {
            if (b0 == 0x1B)
               csctx_rccurve_to(c, s[i], f, s[i+1], s[i+2], s[i+3], 0.0);
            else
               csctx_rccurve_to(c, f, s[i], s[i+1], s[i+2], 0.0, s[i+3]);
            f = 0.0;
         }
         break;

      case 0x0A: // callsubr
         if (!has_subrs) {
            if (info->fdselect.length)
               subrs = cid_get_glyph_subrs(info, glyph_index);
            has_subrs = 1;
         }
         // fallthrough
      case 0x1D: // callgsubr
         if (sp < 1) return Fail("call(g|)subr stack");
         v = (int) s[--sp];
         if (subr_stack_height >= 10) return Fail("recursion limit");
         subr_stack[subr_stack_height++] = data;
         data = get_subr(b0 == 0x0A ? subrs : info->gsubrs, v);
         if (data.length == 0) return Fail("subr not found");
         data.current = data.begin;
         clear_stack = 0;
         break;

      case 0x0B: // return
         if (subr_stack_height <= 0) return Fail("return outside subr");
         data = subr_stack[--subr_stack_height];
         clear_stack = 0;
         break;

      case 0x0E: // endchar
         csctx_close_shape(c);
         return 1;

      case 0x0C: { // two-byte escape
         float32 dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy3, dy4, dy5, dy6;
         float32 dx, dy;
         int32 b1 = ReadByte(&data);
         switch (b1) {
         // @TODO These "flex" implementations ignore the flex-depth and resolution,
         // and always draw beziers.
         case 0x22: // hflex
            if (sp < 7) return Fail("hflex stack");
            dx1 = s[0];
            dx2 = s[1];
            dy2 = s[2];
            dx3 = s[3];
            dx4 = s[4];
            dx5 = s[5];
            dx6 = s[6];
            csctx_rccurve_to(c, dx1, 0, dx2, dy2, dx3, 0);
            csctx_rccurve_to(c, dx4, 0, dx5, -dy2, dx6, 0);
            break;

         case 0x23: // flex
            if (sp < 13) return Fail("flex stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dy3 = s[5];
            dx4 = s[6];
            dy4 = s[7];
            dx5 = s[8];
            dy5 = s[9];
            dx6 = s[10];
            dy6 = s[11];
            //fd is s[12]
            csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
            csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
            break;

         case 0x24: // hflex1
            if (sp < 9) return Fail("hflex1 stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dx4 = s[5];
            dx5 = s[6];
            dy5 = s[7];
            dx6 = s[8];
            csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, 0);
            csctx_rccurve_to(c, dx4, 0, dx5, dy5, dx6, -(dy1+dy2+dy5));
            break;

         case 0x25: // flex1
            if (sp < 11) return Fail("flex1 stack");
            dx1 = s[0];
            dy1 = s[1];
            dx2 = s[2];
            dy2 = s[3];
            dx3 = s[4];
            dy3 = s[5];
            dx4 = s[6];
            dy4 = s[7];
            dx5 = s[8];
            dy5 = s[9];
            dx6 = dy6 = s[10];
            dx = dx1+dx2+dx3+dx4+dx5;
            dy = dy1+dy2+dy3+dy4+dy5;
            if (fabs(dx) > fabs(dy))
               dy6 = -dy;
            else
               dx6 = -dx;
            csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
            csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
            break;

         default:
            return Fail("unimplemented");
         }
      } break;

      default:
         if (b0 != 255 && b0 != 28 && (b0 < 32 || b0 > 254))
            return Fail("reserved operator");

         // push immediate
         if (b0 == 255) {
            f = (float32)(int32)ReadUint32BigEndian(&data) / 0x10000;
         } else {
            Skip(&data, -1);
            f = (float32)(int16)cff_int(&data);
         }
         if (sp >= 48) return Fail("push stack overflow");
         s[sp++] = f;
         clear_stack = 0;
         break;
      }
      if (clear_stack) sp = 0;
   }
   return Fail("no endchar");
}

int32 GetGlyphInfoT2(const FontInfo* info, int32 glyph_index, 
                     int32* x0, int32* y0, int32* x1, int32* y1) {
   csctx c = CSCTX_INIT(1);
   int32 r = run_charstring(info, glyph_index, &c);
   if (x0)  *x0 = r ? c.min_x : 0;
   if (y0)  *y0 = r ? c.min_y : 0;
   if (x1)  *x1 = r ? c.max_x : 0;
   if (y1)  *y1 = r ? c.max_y : 0;
   return r ? c.num_vertices : 0;
}

int32 GetGlyfOffset(const FontInfo* info, int32 glyph_index) {
   int32 g1,g2;

   Assert(!info->cff.length);

   if (glyph_index >= info->numGlyphs) return -1; // glyph index out of range
   if (info->indexToLocFormat >= 2)    return -1; // unknown index->glyph map format

   if (info->indexToLocFormat == 0) {
      g1 = info->glyf + __USHORT(info->data + info->loca + glyph_index * 2) * 2;
      g2 = info->glyf + __USHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
   } else {
      g1 = info->glyf + __ULONG (info->data + info->loca + glyph_index * 4);
      g2 = info->glyf + __ULONG (info->data + info->loca + glyph_index * 4 + 4);
   }

   return g1==g2 ? -1 : g1; // if length is 0, return -1
}

int32 GetGlyphBox(const FontInfo* info, int32 glyph_index, 
                int32* x0, int32* y0, int32* x1, int32* y1) {
   if (info->cff.length) {
      GetGlyphInfoT2(info, glyph_index, x0, y0, x1, y1);
   } else {
      int32 g = GetGlyfOffset(info, glyph_index);
      if (g < 0) return 0;

      if (x0) *x0 = __SHORT(info->data + g + 2);
      if (y0) *y0 = __SHORT(info->data + g + 4);
      if (x1) *x1 = __SHORT(info->data + g + 6);
      if (y1) *y1 = __SHORT(info->data + g + 8);
   }
   return 1;
}

void GetGlyphBitmapBoxSubpixel(const FontInfo* font, int32 glyph, 
                               float32 scale_x, float32 scale_y, 
                               float32 shift_x, float32 shift_y, 
                               int32* ix0, int32* iy0, int32* ix1, int32* iy1) {
   int32 x0=0,y0=0,x1,y1; // =0 suppresses compiler warning
   if (!GetGlyphBox(font, glyph, &x0,&y0,&x1,&y1)) {
      // e.g. space character
      if (ix0) *ix0 = 0;
      if (iy0) *iy0 = 0;
      if (ix1) *ix1 = 0;
      if (iy1) *iy1 = 0;
   } else {
      // move to integral bboxes (treating pixels as little squares, what pixels get touched)?
      if (ix0) *ix0 = (int32) floor( x0 * scale_x + shift_x);
      if (iy0) *iy0 = (int32) floor(-y1 * scale_y + shift_y);
      if (ix1) *ix1 = (int32) ceil ( x1 * scale_x + shift_x);
      if (iy1) *iy1 = (int32) ceil (-y0 * scale_y + shift_y);
   }
}

void GetGlyphBitmapBox(const FontInfo* font, int32 glyph, 
                       float32 scale_x, float32 scale_y, 
                       int32 *ix0, int32 *iy0, int32 *ix1, int32 *iy1) {
   GetGlyphBitmapBoxSubpixel(font, glyph, scale_x, scale_y,0.0f,0.0f, ix0, iy0, ix1, iy1);
}

int32 close_shape(vertex* vertices, int32 num_vertices, 
                  int32 was_off, int32 start_off,
                  int32 sx, int32 sy, 
                  int32 scx, int32 scy, 
                  int32 cx, int32 cy) {
   if (start_off) {
      if (was_off)
         setvertex(&vertices[num_vertices++], vcurve, (cx+scx)>>1, (cy+scy)>>1, cx,cy);
      setvertex(&vertices[num_vertices++], vcurve, sx,sy,scx,scy);
   } else {
      if (was_off)
         setvertex(&vertices[num_vertices++], vcurve,sx,sy,cx,cy);
      else
         setvertex(&vertices[num_vertices++], vline,sx,sy,0,0);
   }
   return num_vertices;
}

int32 GetGlyphShape(Arena* arena, const FontInfo* info, int32 glyph_index, vertex** pvertices);

int32 GetGlyphShapeTT(Arena* arena, const FontInfo* info, int32 glyph_index, vertex** pvertices) {
   int16 numberOfContours;
   byte* endPtsOfContours;
   byte* data = info->data;
   vertex* vertices=0;
   int32 num_vertices=0;
   int32 g = GetGlyfOffset(info, glyph_index);

   *pvertices = NULL;

   if (g < 0) return 0;

   numberOfContours = __SHORT(data + g);

   if (numberOfContours > 0) {
      byte flags=0,flagcount;
      int32 ins, i,j=0,m,n, next_move, was_off=0, off, start_off=0;
      int32 x,y,cx,cy,sx,sy, scx,scy;
      byte* points;
      endPtsOfContours = (data + g + 10);
      ins = __USHORT(data + g + 10 + numberOfContours * 2);
      points = data + g + 10 + numberOfContours * 2 + 2 + ins;

      n = 1+__USHORT(endPtsOfContours + numberOfContours*2-2);

      m = n + 2*numberOfContours;  // a loose bound on how many vertices we might need
      vertices = (vertex *) ArenaAlloc(arena, m * sizeof(vertices[0]));
      if (vertices == 0)
         return 0;

      next_move = 0;
      flagcount=0;

      // in first pass, we load uninterpreted data into the allocated array
      // above, shifted to the end of the array so we won't overwrite it when
      // we create our final data starting from the front

      off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

      // first load flags

      for (i=0; i < n; ++i) {
         if (flagcount == 0) {
            flags = *points++;
            if (flags & 8)
               flagcount = *points++;
         } else
            --flagcount;
         vertices[off+i].type = flags;
      }

      // now load x coordinates
      x=0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         if (flags & 2) {
            int16 dx = *points++;
            x += (flags & 16) ? dx : -dx; // ???
         } else {
            if (!(flags & 16)) {
               x = x + (int16) (points[0]*256 + points[1]);
               points += 2;
            }
         }
         vertices[off+i].x = (int16) x;
      }

      // now load y coordinates
      y=0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         if (flags & 4) {
            int16 dy = *points++;
            y += (flags & 32) ? dy : -dy; // ???
         } else {
            if (!(flags & 32)) {
               y = y + (int16) (points[0]*256 + points[1]);
               points += 2;
            }
         }
         vertices[off+i].y = (int16) y;
      }

      // now convert them to our format
      num_vertices=0;
      sx = sy = cx = cy = scx = scy = 0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         x     = (int16) vertices[off+i].x;
         y     = (int16) vertices[off+i].y;

         if (next_move == i) {
            if (i != 0)
               num_vertices = close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);

            // now start the new one
            start_off = !(flags & 1);
            if (start_off) {
               // if we start off with an off-curve point, then when we need to find a point on the curve
               // where we can start, and we need to save some state for when we wraparound.
               scx = x;
               scy = y;
               if (!(vertices[off+i+1].type & 1)) {
                  // next point is also a curve point, so interpolate an on-point curve
                  sx = (x + (int32) vertices[off+i+1].x) >> 1;
                  sy = (y + (int32) vertices[off+i+1].y) >> 1;
               } else {
                  // otherwise just use the next point as our start point
                  sx = (int32) vertices[off+i+1].x;
                  sy = (int32) vertices[off+i+1].y;
                  ++i; // we're using point i+1 as the starting point, so skip it
               }
            } else {
               sx = x;
               sy = y;
            }
            setvertex(&vertices[num_vertices++], vmove,sx,sy,0,0);
            was_off = 0;
            next_move = 1 + __USHORT(endPtsOfContours+j*2);
            ++j;
         } else {
            if (!(flags & 1)) { // if it's a curve
               if (was_off) // two off-curve control points in a row means interpolate an on-curve midpoint
                  setvertex(&vertices[num_vertices++], vcurve, (cx+x)>>1, (cy+y)>>1, cx, cy);
               cx = x;
               cy = y;
               was_off = 1;
            } else {
               if (was_off)
                  setvertex(&vertices[num_vertices++], vcurve, x,y, cx, cy);
               else
                  setvertex(&vertices[num_vertices++], vline, x,y,0,0);
               was_off = 0;
            }
         }
      }
      num_vertices = close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);
   } else if (numberOfContours < 0) {
      // Compound shapes.
      int32 more = 1;
      byte* comp = data + g + 10;
      num_vertices = 0;
      vertices = 0;
      while (more) {
         uint16 flags, gidx;
         int32 comp_num_verts = 0, i;
         vertex *comp_verts = 0, *tmp = 0;
         float32 mtx[6] = {1,0,0,1,0,0}, m, n;

         flags = __SHORT(comp); comp+=2;
         gidx = __SHORT(comp); comp+=2;

         if (flags & 2) { // XY values
            if (flags & 1) { // shorts
               mtx[4] = __SHORT(comp); comp+=2;
               mtx[5] = __SHORT(comp); comp+=2;
            } else {
               mtx[4] = __CHAR(comp); comp+=1;
               mtx[5] = __CHAR(comp); comp+=1;
            }
         }
         else {
            // @TODO handle matching point
            Assert(0);
         }
         if (flags & (1<<3)) { // WE_HAVE_A_SCALE
            mtx[0] = mtx[3] = __SHORT(comp)/16384.0f; comp+=2;
            mtx[1] = mtx[2] = 0;
         } else if (flags & (1<<6)) { // WE_HAVE_AN_X_AND_YSCALE
            mtx[0] = __SHORT(comp)/16384.0f; comp+=2;
            mtx[1] = mtx[2] = 0;
            mtx[3] = __SHORT(comp)/16384.0f; comp+=2;
         } else if (flags & (1<<7)) { // WE_HAVE_A_TWO_BY_TWO
            mtx[0] = __SHORT(comp)/16384.0f; comp+=2;
            mtx[1] = __SHORT(comp)/16384.0f; comp+=2;
            mtx[2] = __SHORT(comp)/16384.0f; comp+=2;
            mtx[3] = __SHORT(comp)/16384.0f; comp+=2;
         }

         // Find transformation scales.
         m = (float32) sqrt(mtx[0]*mtx[0] + mtx[1]*mtx[1]);
         n = (float32) sqrt(mtx[2]*mtx[2] + mtx[3]*mtx[3]);

         // Get indexed glyph.
         comp_num_verts = GetGlyphShape(arena, info, gidx, &comp_verts);
         if (comp_num_verts > 0) {
            // Transform vertices.
            for (i = 0; i < comp_num_verts; ++i) {
               vertex* v = &comp_verts[i];
               int16 x,y;
               x=v->x; y=v->y;
               v->x = (int16)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
               v->y = (int16)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
               x=v->cx; y=v->cy;
               v->cx = (int16)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
               v->cy = (int16)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
            }
            // Append vertices.
            tmp = (vertex*)ArenaAlloc(arena, (num_vertices+comp_num_verts)*sizeof(vertex));
            if (!tmp) {
               return 0;
            }
            if (num_vertices > 0) memcpy(tmp, vertices, num_vertices*sizeof(vertex));
            memcpy(tmp+num_vertices, comp_verts, comp_num_verts*sizeof(vertex));
            vertices = tmp;
            num_vertices += comp_num_verts;
         }
         // More components ?
         more = flags & (1<<5);
      }
   } else {
      // numberOfCounters == 0, do nothing
   }

   *pvertices = vertices;
   return num_vertices;
}

int32 GetGlyphShapeT2(Arena* arena, const FontInfo* info, int32 glyph_index, vertex** pvertices) {
   // runs the charstring twice, once to count and once to output (to avoid realloc)
   csctx count_ctx = CSCTX_INIT(1);
   csctx output_ctx = CSCTX_INIT(0);
   if (run_charstring(info, glyph_index, &count_ctx)) {
      *pvertices = (vertex*)ArenaAlloc(arena, count_ctx.num_vertices*sizeof(vertex));
      output_ctx.pvertices = *pvertices;
      if (run_charstring(info, glyph_index, &output_ctx)) {
         Assert(output_ctx.num_vertices == count_ctx.num_vertices);
         return output_ctx.num_vertices;
      }
   }
   *pvertices = NULL;
   return 0;
}

int32 GetGlyphShape(Arena* arena, const FontInfo* info, int32 glyph_index, vertex** pvertices) {
   if (!info->cff.length)
      return GetGlyphShapeTT(arena, info, glyph_index, pvertices);
   else
      return GetGlyphShapeT2(arena, info, glyph_index, pvertices);
}

#include "truetype_rasterizer.cpp"

void MakeGlyphBitmapSubpixel(Arena* arena, const FontInfo* info, byte* output, 
                             int32 out_w, int32 out_h, int32 out_stride, 
                             float32 scale_x, float32 scale_y, 
                             float32 shift_x, float32 shift_y, int32 glyph) {
   int32 ix0,iy0;
   vertex *vertices;
   int32 num_verts = GetGlyphShape(arena, info, glyph, &vertices);
   Image gbm;

   GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x, shift_y, &ix0,&iy0,0,0);
   gbm.data = output;
   gbm.width = out_w;
   gbm.height = out_h;
   gbm.channels = out_stride;

   if (gbm.width && gbm.height)
      Rasterize(arena, &gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x, shift_y, ix0,iy0, 1);
}

void MakeGlyphBitmap(Arena* arena, const FontInfo* info, byte* output, 
                     int32 out_w, int32 out_h, int32 out_stride, 
                     float32 scale_x, float32 scale_y, int32 glyph) {
   MakeGlyphBitmapSubpixel(arena, info, output, out_w, out_h, out_stride, scale_x, scale_y, 0.0f,0.0f, glyph);
}

Font TTLoadFont(Arena* arena, byte* data, float32 height, uint32 rgb) {
   int32 pw = 512;
   int32 ph = 512;
   int32 first_char = 32;
   int32 num_chars = 96;

   FontInfo f;
   if (!GetFontInfo(&f, data, 0))
      return {};

   byte* mono_bitmap = (byte*)ArenaAlloc(arena, pw*ph);
   memset(mono_bitmap, 0, pw*ph); // background of 0 around pixels
   int32 x = 1;
   int32 y = 1;
   int32 bottom_y = 1;

   float32 scale = ScaleForPixelHeight(&f, height);

   Font font;
   for (int32 i=0; i < num_chars; ++i) {
      int32 advance, lsb, x0, y0, x1, y1, gw, gh;
      int32 g = FindGlyphIndex(&f, first_char + i);
      GetGlyphHMetrics(&f, g, &advance, &lsb);
      GetGlyphBitmapBox(&f, g, scale, scale, &x0, &y0, &x1, &y1);
      gw = x1 - x0;
      gh = y1 - y0;
      if (x + gw + 1 >= pw)
      y = bottom_y, x = 1; // advance to next row
      if (y + gh + 1 >= ph) // check if it fits vertically AFTER potentially moving to next row
         return {};
      Assert(x + gw < pw);
      Assert(y + gh < ph);
      MakeGlyphBitmap(arena, &f, mono_bitmap + x + y*pw, gw, gh, pw, scale, scale, g);
      font.chardata[i].x0 = x;
      font.chardata[i].y0 = y;
      font.chardata[i].x1 = x + gw;
      font.chardata[i].y1 = y + gh;
      font.chardata[i].xadvance = scale * advance;
      font.chardata[i].xoff     = (float32) x0;
      font.chardata[i].yoff     = (float32) y0;
      x = x + gw + 1;
      if (y + gh + 1 > bottom_y)
         bottom_y = y + gh + 1;
   }

   byte* bitmap = ExpandChannels(arena, mono_bitmap, pw*ph, rgb);

   font.texture = GenerateTexture(Image{pw, ph, 4, bitmap}, Smooth);
   font.height = height;

   return font;
}