struct PNG {
   uint32 img_x, img_y;
   int32 img_n, img_out_n;
   byte *idata, *expanded, *out;
   int32 depth;
};

struct PNGChunk {
   uint32 length;
   uint32 type;
};

enum {
   SCAN_load = 0,
   SCAN_type,
   SCAN_header
};

enum {
   F_none = 0,
   F_sub = 1,
   F_up = 2,
   F_avg = 3,
   F_paeth = 4,
   // synthetic filters used for first scanline to avoid needing a dummy row of 0s
   F_avg_first,
   F_paeth_first
};

static byte first_row_filter[5] = {
   F_none,
   F_sub,
   F_none,
   F_avg_first,
   F_paeth_first
};

static int32 unpremultiply_on_load = 0;
static int32 de_iphone_flag = 0;

int32 check_png_header(byte** data) {
   static const byte png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   for (int32 i = 0; i < 8; ++i)
      if (ReadByte(data) != png_sig[i]) return FAIL("bad png sig Not a PNG");
   return 1;
}

PNGChunk get_chunk_header(byte** data) {
   PNGChunk c;
   c.length = ReadUint32BigEndian(data);
   c.type   = ReadUint32BigEndian(data);
   return c;
}

int32 paeth(int32 a, int32 b, int32 c) {
   int32 p = a + b - c;
   int32 pa = ABS(p - a);
   int32 pb = ABS(p - b);
   int32 pc = ABS(p - c);
   if (pa <= pb && pa <= pc) return a;
   if (pb <= pc) return b;
   return c;
}

static byte depth_scale_table[9] = {0, 0xff, 0x55, 0, 0x11, 0,0,0, 0x01};

int32 create_png_image_raw(Arena* arena, byte** data, PNG* png, byte*raw, uint32 raw_len, int32 out_n, uint32 x, uint32 y, int32 depth, int32 color) {
   int32 bytes = (depth == 16 ? 2 : 1);
   uint32 i,j,stride = x*out_n*bytes;
   uint32 img_len, img_width_bytes;
   int32 k;
   int32 img_n = png->img_n; // copy it into a local for later

   int32 output_bytes = out_n*bytes;
   int32 filter_bytes = img_n*bytes;
   int32 width = x;

   ASSERT(out_n == png->img_n || out_n == png->img_n + 1);
   png->out = (byte*)ArenaAlloc(arena, x*y*output_bytes); // extra bytes to write off the end into
   if (!png->out) return FAIL("Out of memory");

   //if (!mad3sizes_valid(img_n, x, depth, 7)) return FAIL("too large Corrupt PNG");
   img_width_bytes = (((img_n * x * depth) + 7) >> 3);
   img_len = (img_width_bytes + 1) * y;

   // we used to check for exact match between raw_len and img_len on non-interlaced PNGs,
   // but issue #276 reported a PNG in the wild that had extra data at the end (all zeros),
   // so just check for raw_len < img_len always.
   if (raw_len < img_len) return FAIL("not enough pixels Corrupt PNG");

   for (j = 0; j < y; ++j) {
      byte*cur = png->out + stride*j;
      byte*prior;
      int32 filter = *raw++;

      if (filter > 4)
         return FAIL("invalid filter Corrupt PNG");

      if (depth < 8) {
         if (img_width_bytes > x) return FAIL("invalid width Corrupt PNG");
         cur += x*out_n - img_width_bytes; // store output to the rightmost img_len bytes, so we can decode in place
         filter_bytes = 1;
         width = img_width_bytes;
      }
      prior = cur - stride; // bugfix: need to compute this after 'cur +=' computation above

      // if first row, use special filter that doesn't sample previous row
      if (j == 0) filter = first_row_filter[filter];

      // handle first byte explicitly
      for (k = 0; k < filter_bytes; ++k) {
         switch (filter) {
            case F_none       : cur[k] = raw[k]; break;
            case F_sub        : cur[k] = raw[k]; break;
            case F_up         : cur[k] = (byte)((raw[k] + prior[k]) & 0xff); break;
            case F_avg        : cur[k] = (byte)((raw[k] + (prior[k]>>1)) & 0xff); break;
            case F_paeth      : cur[k] = (byte)((raw[k] + paeth(0,prior[k],0)) & 0xff); break;
            case F_avg_first  : cur[k] = raw[k]; break;
            case F_paeth_first: cur[k] = raw[k]; break;
         }
      }

      if (depth == 8) {
         if (img_n != out_n)
            cur[img_n] = 255; // first pixel
         raw += img_n;
         cur += out_n;
         prior += out_n;
      }
      else if (depth == 16) {
         if (img_n != out_n) {
            cur[filter_bytes]   = 255; // first pixel top byte
            cur[filter_bytes + 1] = 255; // first pixel bottom byte
         }
         raw += filter_bytes;
         cur += output_bytes;
         prior += output_bytes;
      }
      else {
         raw += 1;
         cur += 1;
         prior += 1;
      }

      // this is a little gross, so that we don't switch per-pixel or per-component
      if (depth < 8 || img_n == out_n) {
         int32 nk = (width - 1)*filter_bytes;
         #define CASE(f) \
             case f:     \
                for (k = 0; k < nk; ++k)
         switch (filter) {
            // "none" filter turns into a memcpy here; make that explicit.
            case F_none:         memcpy(cur, raw, nk); break;
            CASE(F_sub)          {cur[k] = (byte)((raw[k] + cur[k-filter_bytes]) & 0xff); } break;
            CASE(F_up)           {cur[k] = (byte)((raw[k] + prior[k]) & 0xff); } break;
            CASE(F_avg)          {cur[k] = (byte)((raw[k] + ((prior[k] + cur[k-filter_bytes])>>1) & 0xff)); } break;
            CASE(F_paeth)        {cur[k] = (byte)((raw[k] + paeth(cur[k-filter_bytes],prior[k],prior[k-filter_bytes])) & 0xff); } break;
            CASE(F_avg_first)    {cur[k] = (byte)((raw[k] + (cur[k-filter_bytes] >> 1)) & 0xff); } break;
            CASE(F_paeth_first)  {cur[k] = (byte)((raw[k] + paeth(cur[k-filter_bytes],0,0)) & 0xff); } break;
         }
         #undef CASE
         raw += nk;
      }
      else {
         ASSERT(img_n + 1 == out_n);
         #define CASE(f) \
             case f:     \
                for (i = x - 1; i >= 1; --i, cur[filter_bytes]=255,raw+=filter_bytes,cur+=output_bytes,prior+=output_bytes) \
                   for (k = 0; k < filter_bytes; ++k)
         switch (filter) {
            CASE(F_none)         {cur[k] = raw[k]; } break;
            CASE(F_sub)          {cur[k] = (byte)((raw[k] + cur[k- output_bytes]) & 0xff); } break;
            CASE(F_up)           {cur[k] = (byte)((raw[k] + prior[k]) & 0xff); } break;
            CASE(F_avg)          {cur[k] = (byte)((raw[k] + ((prior[k] + cur[k- output_bytes])>>1)) & 0xff); } break;
            CASE(F_paeth)        {cur[k] = (byte)((raw[k] + paeth(cur[k- output_bytes],prior[k],prior[k- output_bytes])) & 0xff); } break;
            CASE(F_avg_first)    {cur[k] = (byte)((raw[k] + (cur[k- output_bytes] >> 1)) & 0xff); } break;
            CASE(F_paeth_first)  {cur[k] = (byte)((raw[k] + paeth(cur[k- output_bytes],0,0)) & 0xff); } break;
         }
         #undef CASE

         // the loop above sets the high byte of the pixels' alpha, but for
         // 16 bit png files we also need the low byte set. we'll do that here.
         if (depth == 16) {
            cur = png->out + stride*j; // start at the beginning of the row again
            for (i = 0; i < x; ++i,cur+=output_bytes) {
               cur[filter_bytes + 1] = 255;
            }
         }
      }
   }

   // we make a separate pass to expand bits to pixels; for performance,
   // this could run two scanlines behind the above code, so it won't
   // intefere with filtering but will still be in the cache.
   if (depth < 8) {
      for (j = 0; j < y; ++j) {
         byte*cur = png->out + stride*j;
         byte*in  = png->out + stride*j + x*out_n - img_width_bytes;
         // unpack 1/2/4-bit into a 8-bit buffer. allows us to keep the common 8-bit path optimal at minimal cost for 1/2/4-bit
         // png guarante byte alignment, if width is not multiple of 8/4/2 we'll decode dummy trailing data that will be skipped in the later loop
         byte scale = (color == 0) ? depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range

         // note that the final byte might overshoot and write more data than desired.
         // we can allocate enough data that this never writes out of memory, but it
         // could also overwrite the next scanline. can it overwrite non-empty data
         // on the next scanline? yes, consider 1-pixel-wide scanlines with 1-bit-per-pixel.
         // so we need to explicitly clamp the final ones

         if (depth == 4) {
            for (k = x*img_n; k >= 2; k-=2, ++in) {
               *cur++ = scale * ((*in >> 4)       );
               *cur++ = scale * ((*in     ) & 0x0f);
            }
            if (k > 0) *cur++ = scale * ((*in >> 4)       );
         }
         else if (depth == 2) {
            for (k=x*img_n; k >= 4; k-=4, ++in) {
               *cur++ = scale * ((*in >> 6)       );
               *cur++ = scale * ((*in >> 4) & 0x03);
               *cur++ = scale * ((*in >> 2) & 0x03);
               *cur++ = scale * ((*in     ) & 0x03);
            }
            if (k > 0) *cur++ = scale * ((*in >> 6)       );
            if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
            if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
         }
         else if (depth == 1) {
            for (k=x*img_n; k >= 8; k-=8, ++in) {
               *cur++ = scale * ((*in >> 7)       );
               *cur++ = scale * ((*in >> 6) & 0x01);
               *cur++ = scale * ((*in >> 5) & 0x01);
               *cur++ = scale * ((*in >> 4) & 0x01);
               *cur++ = scale * ((*in >> 3) & 0x01);
               *cur++ = scale * ((*in >> 2) & 0x01);
               *cur++ = scale * ((*in >> 1) & 0x01);
               *cur++ = scale * ((*in     ) & 0x01);
            }
            if (k > 0) *cur++ = scale * ((*in >> 7)       );
            if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
            if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
            if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
            if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
            if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
            if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
         }
         if (img_n != out_n) {
            int32 q;
            // insert alpha = 255
            cur = png->out + stride*j;
            if (img_n == 1) {
               for (q=x - 1; q >= 0; --q) {
                  cur[q*2 + 1] = 255;
                  cur[q*2 + 0] = cur[q];
               }
            }
            else {
               ASSERT(img_n == 3);
               for (q=x-1; q >= 0; --q) {
                  cur[q*4 + 3] = 255;
                  cur[q*4 + 2] = cur[q*3+2];
                  cur[q*4 + 1] = cur[q*3 + 1];
                  cur[q*4 + 0] = cur[q*3+0];
               }
            }
         }
      }
   }
   else if (depth == 16) {
      // force the image data from big-endian to platform-native.
      // this is done in a separate pass due to the decoding relying
      // on the data being untouched, but could probably be done
      // per-line during decode if care is taken.
      byte*cur = png->out;
      uint16 *cur16 = (uint16*)cur;

      for(i = 0; i < x*y*out_n; ++i,cur16++,cur+=2) {
         *cur16 = (cur[0] << 8) | cur[1];
      }
   }

   return 1;
}

int32 create_png_image(Arena* arena, byte** data, PNG* png, byte* image_data, uint32 image_data_len, int32 out_n, int32 depth, int32 color, int32 interlaced) {
   int32 bytes = (depth == 16 ? 2 : 1);
   int32 out_bytes = out_n*bytes;
   int32 p;
   if (!interlaced)
      return create_png_image_raw(arena, data, png, image_data, image_data_len, out_n, png->img_x, png->img_y, depth, color);

   // de-interlacing
   byte* final = (byte*)ArenaAlloc(arena, png->img_x*png->img_y*out_bytes);
   if (!final) return FAIL("Out of memory");
   for (p = 0; p < 7; ++p) {
      int32 xorig[] = {0, 4, 0, 2, 0, 1, 0};
      int32 yorig[] = {0, 0, 4, 0, 2, 0, 1};
      int32 xspc[]  = {8, 8, 4, 4, 2, 2, 1};
      int32 yspc[]  = {8, 8, 8, 4, 4, 2, 2};
      int32 i,j,x,y;
      // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
      x = (png->img_x - xorig[p] + xspc[p]-1) / xspc[p];
      y = (png->img_y - yorig[p] + yspc[p]-1) / yspc[p];
      if (x && y) {
         uint32 img_len = ((((png->img_n * x * depth) + 7) >> 3) + 1) * y;
         if (!create_png_image_raw(arena, data, png, image_data, image_data_len, out_n, x, y, depth, color)) {
            return 0;
         }
         for (j = 0; j < y; ++j) {
            for (i = 0; i < x; ++i) {
               int32 out_y = j*yspc[p]+yorig[p];
               int32 out_x = i*xspc[p]+xorig[p];
               memcpy(final + out_y*png->img_x*out_bytes + out_x*out_bytes,
                      png->out + (j*x+i)*out_bytes, out_bytes);
            }
         }
         image_data += img_len;
         image_data_len -= img_len;
      }
   }
   png->out = final;

   return 1;
}

int32 compute_transparency(byte** data, PNG* png, byte tc[3], int32 out_n) {
   uint32 i, pixel_count = png->img_x * png->img_y;
   byte *p = png->out;

   // compute color-based transparency, assuming we've
   // already got 255 as the alpha value in the output
   ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i = 0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } 
   else {
      for (i = 0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

int32 compute_transparency16(byte** data, PNG* png, uint16 tc[3], int32 out_n) {
   uint32 i, pixel_count = png->img_x * png->img_y;
   uint16* p = (uint16*)png->out;

   // compute color-based transparency, assuming we've
   // already got 65535 as the alpha value in the output
   ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i = 0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 65535);
         p += 2;
      }
   }
   else {
      for (i = 0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

int32 expand_png_palette(Arena* arena, PNG* png, byte *palette, int32 len, int32 pal_img_n) {
   uint32 i, pixel_count = png->img_x * png->img_y;
   byte *temp_out, *orig = png->out;

   byte* p = (byte *)ArenaAlloc(arena, pixel_count*pal_img_n);
   if (p == NULL) return FAIL("Out of memory");

   // between here and free(out) below, exitting would leak
   temp_out = p;

   if (pal_img_n == 3) {
      for (i = 0; i < pixel_count; ++i) {
         int32 n = orig[i]*4;
         p[0] = palette[n + 0];
         p[1] = palette[n + 1];
         p[2] = palette[n + 2];
         p += 3;
      }
   }
   else {
      for (i = 0; i < pixel_count; ++i) {
         int32 n = orig[i]*4;
         p[0] = palette[n + 0];
         p[1] = palette[n + 1];
         p[2] = palette[n + 2];
         p[3] = palette[n + 3];
         p += 4;
      }
   }
   png->out = temp_out;

   return 1;
}

void set_unpremultiply_on_load(int32 flag_true_if_should_unpremultiply) {
   unpremultiply_on_load = flag_true_if_should_unpremultiply;
}

void convert_iphone_png_to_rgb(int32 flag_true_if_should_convert) {
   de_iphone_flag = flag_true_if_should_convert;
}

void de_iphone(byte** data, PNG* png) {
   uint32 i, pixel_count = png->img_x * png->img_y;
   byte *p = png->out;

   if (png->img_out_n == 3) {  // convert bgr to rgb
      for (i = 0; i < pixel_count; ++i) {
         byte t = p[0];
         p[0] = p[2];
         p[2] = t;
         p += 3;
      }
   } 
   else {
      ASSERT(png->img_out_n == 4);
      if (unpremultiply_on_load) {
         // convert bgr to rgb and unpremultiply
         for (i = 0; i < pixel_count; ++i) {
            byte a = p[3];
            byte t = p[0];
            if (a) {
               byte half = a / 2;
               p[0] = (p[2] * 255 + half) / a;
               p[1] = (p[1] * 255 + half) / a;
               p[2] = ( t   * 255 + half) / a;
            }
            else {
               p[0] = p[2];
               p[2] = t;
            }
            p += 4;
         }
      }
      else {
         // convert bgr to rgb
         for (i = 0; i < pixel_count; ++i) {
            byte t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 4;
         }
      }
   }
}

#include "zlib.cpp"

#define PNG_TYPE(a,b,c,d)  (((uint32)(a) << 24) + ((uint32)(b) << 16) + ((uint32)(c) << 8) + (uint32)(d))

int32 parse_png_file(Arena* arena, byte** data, PNG* png, int32 scan) {
   byte palette[1024], pal_img_n = 0;
   byte has_trans = 0, tc[3]={0};
   uint16 tc16[3];
   uint32 ioff = 0, idata_limit = 0, i, pal_len = 0;
   int32 first=1,k,interlace = 0, color = 0, is_iphone = 0;

   png->expanded = NULL;
   png->idata = NULL;
   png->out = NULL;

   if (!check_png_header(data)) return 0;

   if (scan == SCAN_type) return 1;

   while (true) {
      PNGChunk c = get_chunk_header(data);
      switch (c.type) {
         case PNG_TYPE('C','g','B','I'):
            is_iphone = 1;
            Skip(data, c.length);
            break;
         case PNG_TYPE('I','H','D','R'): {
            int32 comp,filter;
            if (!first) return FAIL("multiple IHDR Corrupt PNG");
            first = 0;
            if (c.length != 13) return FAIL("bad IHDR len Corrupt PNG");
            png->img_x = ReadUint32BigEndian(data);
            png->img_y = ReadUint32BigEndian(data);
            if (png->img_y > MAX_DIMENSIONS) return FAIL("too large Very large image (corrupt?)");
            if (png->img_x > MAX_DIMENSIONS) return FAIL("too large Very large image (corrupt?)");
            png->depth = ReadByte(data);  if (png->depth != 1 && png->depth != 2 && png->depth != 4 && png->depth != 8 && png->depth != 16)  return FAIL("1/2/4/8/16-bit only PNG not supported: 1/2/4/8/16-bit only");
            color = ReadByte(data);  if (color > 6)         return FAIL("bad ctype Corrupt PNG");
            if (color == 3 && png->depth == 16)                  return FAIL("bad ctype Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return FAIL("bad ctype Corrupt PNG");
            comp  = ReadByte(data);  if (comp) return FAIL("bad comp method Corrupt PNG");
            filter= ReadByte(data);  if (filter) return FAIL("bad filter method Corrupt PNG");
            interlace = ReadByte(data); if (interlace>1) return FAIL("bad interlace method Corrupt PNG");
            if (!png->img_x || !png->img_y) return FAIL("0-pixel image Corrupt PNG");
            if (!pal_img_n) {
               png->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / png->img_x / png->img_n < png->img_y) return FAIL("too large Image too large to decode");
               if (scan == SCAN_header) return 1;
            }
            else {
               // if paletted, then pal_n is our final components, and
               // img_n is # components to decompress/filter.
               png->img_n = 1;
               if ((1 << 30) / png->img_x / 4 < png->img_y) return FAIL("too large Corrupt PNG");
               // if SCAN_header, have to scan to see if we have a tRNS
            }
            break;
         }

         case PNG_TYPE('P','L','T','E'):  {
            if (first) return FAIL("first not IHDR Corrupt PNG");
            if (c.length > 256*3) return FAIL("invalid PLTE Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return FAIL("invalid PLTE Corrupt PNG");
            for (i = 0; i < pal_len; ++i) {
               palette[i*4 + 0] = ReadByte(data);
               palette[i*4 + 1] = ReadByte(data);
               palette[i*4 + 2] = ReadByte(data);
               palette[i*4 + 3] = 255;
            }
            break;
         }

         case PNG_TYPE('t','R','N','S'): {
            if (first) return FAIL("first not IHDR Corrupt PNG");
            if (png->idata) return FAIL("tRNS after IDAT Corrupt PNG");
            if (pal_img_n) {
               if (scan == SCAN_header) { png->img_n = 4; return 1; }
               if (pal_len == 0) return FAIL("tRNS before PLTE Corrupt PNG");
               if (c.length > pal_len) return FAIL("bad tRNS len Corrupt PNG");
               pal_img_n = 4;
               for (i = 0; i < c.length; ++i)
                  palette[i*4+3] = ReadByte(data);
            }
            else {
               if (!(png->img_n & 1)) return FAIL("tRNS with alpha Corrupt PNG");
               if (c.length != (uint32) png->img_n*2) return FAIL("bad tRNS len Corrupt PNG");
               has_trans = 1;
               if (png->depth == 16) {
                  for (k = 0; k < png->img_n; ++k) tc16[k] = ReadUint16BigEndian(data); // copy the values as-is
               }
            else {
                  for (k = 0; k < png->img_n; ++k) tc[k] = (byte)(ReadUint16BigEndian(data) & 255) * depth_scale_table[png->depth]; // non 8-bit images will be larger
               }
            }
            break;
         }

         case PNG_TYPE('I','D','A','T'): {
            if (first) return FAIL("first not IHDR Corrupt PNG");
            if (pal_img_n && !pal_len) return FAIL("no PLTE Corrupt PNG");
            if (scan == SCAN_header) { png->img_n = pal_img_n; return 1; }
            if ((int32)(ioff + c.length) < (int32)ioff) return 0;
            if (ioff + c.length > idata_limit) {
               uint32 idata_limit_old = idata_limit;
               byte *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               p = (byte*)ArenaReAlloc(arena,png->idata, idata_limit_old, idata_limit); if (p == NULL) return FAIL("Out of memory");
               png->idata = p;
            }
            memcpy(png->idata+ioff, *data, c.length);
            *data += c.length;
            ioff += c.length;
            break;
         }

         case PNG_TYPE('I','E','N','D'): {
            uint32 raw_len, bpl;
            if (first) return FAIL("first not IHDR Corrupt PNG");
            if (scan != SCAN_load) return 1;
            if (png->idata == NULL) return FAIL("no IDAT Corrupt PNG");
            // initial guess for decoded data size to avoid unnecessary reallocs
            bpl = (png->img_x * png->depth + 7) / 8; // bytes per line, per component
            raw_len = bpl * png->img_y * png->img_n + png->img_y; // filter mode per row
            png->expanded = (byte *) zlib_decode_malloc_guesssize_headerflag(arena, png->idata, ioff, raw_len, (int32*) &raw_len, !is_iphone);
            if (png->expanded == NULL) return 0; // zlib should set error
            png->idata = NULL;
            if ((4 == png->img_n + 1 && 4 != 3 && !pal_img_n) || has_trans)
               png->img_out_n = png->img_n + 1;
            else
               png->img_out_n = png->img_n;
            if (!create_png_image(arena, data, png, png->expanded, raw_len, png->img_out_n, png->depth, color, interlace)) return 0;
            if (has_trans) {
               if (png->depth == 16) {
                  if (!compute_transparency16(data, png, tc16, png->img_out_n)) return 0;
               }
               else {
                  if (!compute_transparency(data, png, tc, png->img_out_n)) return 0;
               }
            }
            if (is_iphone && de_iphone_flag && png->img_out_n > 2)
               de_iphone(data, png);
            if (pal_img_n) {
               // pal_img_n == 3 or 4
               png->img_n = pal_img_n; // record the actual colors we had
               png->img_out_n = pal_img_n;
               png->img_out_n = 4;
               if (!expand_png_palette(arena, png, palette, pal_len, png->img_out_n))
                  return 0;
            }
            else if (has_trans) {
               // non-paletted image with tRNS -> source image has (constant) alpha
               ++png->img_n;
            }
            png->expanded = NULL;
            // end of PNG chunk, read and skip CRC
            ReadUint32BigEndian(data);
            return 1;         }

         default:
            // if critical, fail
            if (first) return FAIL("first not IHDR Corrupt PNG");
            if ((c.type & (1 << 29)) == 0) {
               return FAIL("PNG not supported: unknown PNG chunk type");
            }
            Skip(data, c.length);
            break;
      }
      // end of PNG chunk, read and Skip CRC
      ReadUint32BigEndian(data);
   }
}

byte compute_y(int32 r, int32 g, int32 b) {
   return (byte) (((r*77) + (g*150) +  (29*b)) >> 8);
}

byte* convert_format(Arena* arena, byte* data, int32 img_n, uint32 x, uint32 y) {
   if (4 == img_n) return data;

   byte* good = (byte*)ArenaAlloc(arena, 4*x*y);
   if (good == NULL) {
      LOG("Out of memory");
      return NULL;
   }

   for (int32 j = 0; j < (int32)y; ++j) {
      byte* src  = data + j * x * img_n   ;
      byte* dest = good + j * x * 4;

      #define COMBO(a,b)  ((a)*8+(b))
      #define CASE(a,b)   case COMBO(a,b): for(int32 i = x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with 4 components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (COMBO(img_n, 4)) {
         CASE(1,2) {dest[0] = src[0]; dest[1]=255;                               } break;
         CASE(1,3) {dest[0] = dest[1]=dest[2]=src[0];                            } break;
         CASE(1,4) {dest[0] = dest[1]=dest[2]=src[0]; dest[3]=255;               } break;
         CASE(2,1) {dest[0] = src[0];                                            } break;
         CASE(2,3) {dest[0] = dest[1]=dest[2]=src[0];                            } break;
         CASE(2,4) {dest[0] = dest[1]=dest[2]=src[0]; dest[3]=src[1];            } break;
         CASE(3,4) {dest[0] = src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=255;  } break;
         CASE(3,1) {dest[0] = compute_y(src[0],src[1],src[2]);                   } break;
         CASE(3,2) {dest[0] = compute_y(src[0],src[1],src[2]); dest[1] = 255;    } break;
         CASE(4,1) {dest[0] = compute_y(src[0],src[1],src[2]);                   } break;
         CASE(4,2) {dest[0] = compute_y(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         CASE(4,3) {dest[0] = src[0];dest[1]=src[1];dest[2]=src[2];              } break;
         default: ASSERT(0); LOG("Unsupported format conversion"); return NULL;
      }
      #undef CASE
   }

   return good;
}

uint16 compute_y_16(int32 r, int32 g, int32 b) {
   return (uint16) (((r*77) + (g*150) +  (29*b)) >> 8);
}

uint16* convert_format16(Arena* arena, uint16 *data, int32 img_n, uint32 x, uint32 y) {
   if (4 == img_n) return data;

   uint16* good = (uint16*)ArenaAlloc(arena, 4*x*y*2);
   if (good == NULL) {
      LOG("Out of memory");
      return NULL;
   }

   for (int32 j = 0; j < (int32)y; ++j) {
      uint16* src  = data + j*x*img_n;
      uint16* dest = good + j*x*4;

      #define COMBO(a,b)  ((a)*8+(b))
      #define CASE(a,b)   case COMBO(a,b): for(int32 i = x - 1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with 4 components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (COMBO(img_n, 4)) {
         CASE(1,2) { dest[0]=src[0]; dest[1] = 0xffff;                               } break;
         CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                               } break;
         CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3] = 0xffff;               } break;
         CASE(2,1) { dest[0]=src[0];                                               } break;
         CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                               } break;
         CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1];               } break;
         CASE(3,4) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3] = 0xffff;  } break;
         CASE(3,1) { dest[0]=compute_y_16(src[0],src[1],src[2]);                   } break;
         CASE(3,2) { dest[0]=compute_y_16(src[0],src[1],src[2]); dest[1] = 0xffff; } break;
         CASE(4,1) { dest[0]=compute_y_16(src[0],src[1],src[2]);                   } break;
         CASE(4,2) { dest[0]=compute_y_16(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         CASE(4,3) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];                 } break;
         default: ASSERT(0); LOG("Unsupported format conversion"); return NULL;
      }
      #undef CASE
   }

   return good;
}

void vertical_flip(Image* image) {
   ssize bytes_per_row = (ssize)image->width * 4;
   byte temp[2048];

   for (int32 row = 0; row < (image->height>>1); row++) {
      byte *row0 = image->data + row*bytes_per_row;
      byte *row1 = image->data + (image->height - row - 1)*bytes_per_row;
      // swap row0 with row1
      ssize bytes_left = bytes_per_row;
      while (bytes_left) {
         ssize bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
         memcpy(temp, row0, bytes_copy);
         memcpy(row0, row1, bytes_copy);
         memcpy(row1, temp, bytes_copy);
         row0 += bytes_copy;
         row1 += bytes_copy;
         bytes_left -= bytes_copy;
      }
   }
}

Image PNGLoadImage(Arena* arena, byte* data) {
   Image image = {};
   int32 bits_per_channel;
   PNG png;
   if (parse_png_file(arena, &data, &png, SCAN_load)) {
      if (png.depth <= 8)
         bits_per_channel = 8;
      else if (png.depth == 16)
         bits_per_channel = 16;
      else {
         LOG("bad bits_per_channel PNG not supported: unsupported color depth");
         return image;
      }
      image.data = png.out;
      png.out = NULL;
      if (4 != png.img_out_n) {
         if (bits_per_channel == 8)
            image.data = convert_format(arena, image.data, png.img_out_n, png.img_x, png.img_y);
         else
            image.data = (byte*)convert_format16(arena, (uint16*)image.data, png.img_out_n, png.img_x, png.img_y);
         png.img_out_n = 4;
         if (image.data == NULL) return image;
      }
      image.width = png.img_x;
      image.height = png.img_y;
      image.channels = 4;
   }
   vertical_flip(&image);
   return image;
}