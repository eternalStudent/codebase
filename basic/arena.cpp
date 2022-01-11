struct Arena {
	byte* buffer;
	int64 ptr;			// base-address realtive to the buffer
	int64 capacity;
};

void ArenaInit(Arena* arena, void* buffer, int64 capacity) {
	arena->ptr = 0;
	arena->buffer = (byte*)buffer;
	arena->capacity = capacity;
}

void ArenaAlign(Arena* arena, int32 divisibility) {
	ASSERT(divisibility == 2 || divisibility == 4 || divisibility == 8 || divisibility == 16);
	uint32 modulo = arena->ptr & (divisibility - 1);
	if (modulo != 0) arena->ptr += (divisibility - modulo);
}

byte* ArenaAlloc(Arena* arena, int64 size) {
	if (size == 0) return NULL;

	if (arena->capacity < arena->ptr+size) return NULL;

	byte* data = arena->buffer + arena->ptr;
	arena->ptr += size;

	return data;
}

void ArenaFreeAll(Arena* arena) {
	arena->ptr = 0;
}