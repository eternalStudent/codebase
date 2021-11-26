struct Image {
    union {
        Dimensions2i dimensions;
        struct { int32 width, height;};
    };
    int32 channels;
    byte* data;
};

#include "bitmap.cpp"

Image LoadImage(Arena* arena, const char* filepath) {
    byte* data = ReadAll(arena, filepath).data;
    return BMPLoadImage(arena, data);
}

typedef Point4 Color;

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