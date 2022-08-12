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

Font LoadFont(Arena* arena, const char* filePath, float32 height, uint32 rgb){
   byte* data = OsReadAll(filePath, arena).data;
   return TTLoadFont(arena, data, height, rgb);
}

// NOTE: adopted from stbtt_GetBakedQuad
void DrawText(Font* font, float32 x, float32 y, String string){
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

         DrawImage(font->texture, crop, pos);

         x += bakedchar->xadvance;
      }
   }
}

float32 GetTextWidth(Font* font, String string) {
   int32 first_char = 32;
   int32 last_char = 128;
   float32 x = 0;

   for (int32 i=0; i<string.length; i++) {
      byte b = string.data[i];
      if (b >= first_char && b < last_char) {
         BakedChar* bakedchar = font->chardata + (b - first_char);
         x += bakedchar->xadvance;
      }
   }

   return x;
}