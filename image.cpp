struct Image {
    union {
        Dimensions2i dimensions;
        struct { int32 width, height;};
    };
    int32 channels;
    byte* data;
};

#include "binary_reader.cpp"
#include "bitmap.cpp"

Image LoadImage(Arena* arena, const char* filepath) {
    byte* data = OsReadAll(arena, filepath).data;
    return BMPLoadImage(arena, data);
}

typedef Point4 Color;

#define RGBA_WHITE          0xffffffff
#define RGBA_LIGHTGREY      0xff7e7872
#define RGBA_GREY           0xff5e574f
#define RGBA_DARKGREY       0xff413830
#define RGBA_BLUE           0xffcc9966
#define RGBA_GREEN          0xff94c799
#define RGBA_ORANGE         0xff58aef9
#define RGBA_LILAC          0xffc695c6
#define RGBA_RED            0xff665fec

inline Color RgbaToColor(uint32 rgba) {
    union {
        uint32 rgba;
        struct {byte r, g, b, a;};
    } color;
    color.rgba = rgba;
    return {color.r/255.0f,
            color.g/255.0f,
            color.b/255.0f,
            color.a/255.0f};
}

byte* ExpandChannels(Arena* arena, byte* buffer, int32 length, uint32 rgb) {
    uint32* new_buffer = (uint32*)ArenaAlloc(arena, length*4);
    if (new_buffer == NULL) return NULL;
    for (int32 i = 0; i < length; i++) {
        new_buffer[i] = (buffer[i] << 24) | rgb;
    }
    return (byte*)new_buffer;
}