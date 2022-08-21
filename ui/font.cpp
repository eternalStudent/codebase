struct BakedChar {
   union {
   	Box2i box;
   	struct {Point2i p0, p1;};
   	struct {int32 x0, y0, x1, y1;};
   };
   float32 xoff, yoff, xadvance;
};

struct Font {
	BakedChar chardata[96];
	TextureId texture;
   float32 height;
};

#include "truetype.cpp"

Font LoadFont(Arena* arena, const char* filePath, float32 height){
   byte* data = OsReadAll(filePath, arena).data;
   return TTLoadFont(arena, data, height);
}

// NOTE: adopted from stbtt_GetBakedQuad
void RenderText(Font* font, float32 x, float32 y, String string, uint32 color){
   const int32 pw = 512;
   const int32 ph = 512;
   const int32 first_char = 32;
   const int32 last_char = 128;
   const float32 original_x = x;

   for (int32 i=0; i<string.length; i++) {
      byte b = string.data[i];
      if (b == 10) {
         y -= font->height;
         x = original_x;
      }
      if (b >= first_char && b < last_char) {
         float32 ipw = 1.0f / pw, iph = 1.0f / ph; // NOTE: division takes longer than multipication, this runs division at compile time.
         BakedChar* bakedchar = font->chardata + (b - first_char);

         float32 round_y = floor(y + bakedchar->yoff + 0.5f);
         float32 y1 = round_y + bakedchar->y1 - bakedchar->y0;
         float32 printy = 2*y - y1;

         float32 round_x = floor(x + bakedchar->xoff + 0.5f);

         float32 width = (float32)(bakedchar->x1 - bakedchar->x0);
         float32 height = (float32)(bakedchar->y1 - bakedchar->y0);
         Box2 pos = {round_x, printy, round_x + width, printy + height};

         Box2 crop = {
            bakedchar->x0 * ipw,
            bakedchar->y1 * iph,
            bakedchar->x1 * ipw,
            bakedchar->y0 * iph};

         GfxDrawText(font->texture, crop, pos, 0, color);

         x += bakedchar->xadvance;
      }
   }
}

float32 GetCharWidth(Font* font, byte b) {
   BakedChar* bakedchar = font->chardata + (b - 32);
   return bakedchar->xadvance;
}

float32 GetTextWidth(Font* font, String string, int32* lineCount) {
   *lineCount = 1;
   if (string.length == 0) return 0.0f;
   int32 first_char = 32;
   int32 last_char = 128;
   float32 x = 0;

   for (int32 i=0; i<string.length; i++) {
      byte b = string.data[i];
      if (b == 10) {
         x = 0;
         *lineCount = *lineCount + 1;
      }
      if (b >= first_char && b < last_char) {
         BakedChar* bakedchar = font->chardata + (b - first_char);
         x += bakedchar->xadvance;
      }
   }

   return x;
}

float32 GetTextLineWidth(Font* font, String string) {
   if (string.length == 0) return 0.0f;
   int32 first_char = 32;
   int32 last_char = 128;
   float32 x = 0;

   for (int32 i=0; i<string.length; i++) {
      byte b = string.data[i];
      if (b == 10) {
         x = 0;
      }
      if (b >= first_char && b < last_char) {
         BakedChar* bakedchar = font->chardata + (b - first_char);
         x += bakedchar->xadvance;
      }
   }

   return x;
}

int32 GetCharIndex(Font* font, String string, float32 x_end, float32 y_end) {
   int32 first_char = 32;
   int32 last_char = 128;
   float32 x = 0;
   float32 y = 0;

   for (int32 i=0; i<string.length; i++) {
      byte b = string.data[i];
      if (b == 10) {
         y += font->height;
         x = 0;
      }
      if (b >= first_char && b < last_char) {
         BakedChar* bakedchar = font->chardata + (b - first_char);
         x += bakedchar->xadvance;
         if (x_end <= x - 2 && y_end <= y) return i;
      }
   }

   return -1;
}