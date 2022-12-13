struct Arena {
	byte* buffer;
	ssize ptr;			// base-address realtive to the buffer
	ssize capacity;
};

void ArenaInit(Arena* arena, void* buffer, ssize capacity) {
	arena->ptr = 0;
	arena->buffer = (byte*)buffer;
	arena->capacity = capacity;
}

void ArenaAlign(Arena* arena, int32 divisibility) {
	ASSERT(divisibility == 2 || divisibility == 4 || divisibility == 8 || divisibility == 16);
	uint32 modulo = arena->ptr & (divisibility - 1);
	if (modulo != 0) arena->ptr += (divisibility - modulo);
}

byte* ArenaAlloc(Arena* arena, ssize size) {
	if (size == 0) return NULL;

	if (arena->capacity < arena->ptr+size) return NULL;

	byte* data = arena->buffer + arena->ptr;
	arena->ptr += size;

	return data;
}

void ArenaFreeAll(Arena* arena) {
	arena->ptr = 0;
}

void ArenaReset(Arena* arena) {
	memset(arena->buffer, 0, arena->ptr);
	arena->ptr = 0;
}

void* ArenaReAlloc(Arena* arena, void* oldData, ssize oldSize, ssize newSize) {
	void* result = ArenaAlloc(arena, newSize);
	memcpy(result, oldData, oldSize);
	return result;
}