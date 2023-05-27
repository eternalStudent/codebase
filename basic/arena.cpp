/*
 * NOTE:
 * non-growing arena
 *
 * TODO:
 * experiment with additional arena types
 */

struct Arena {
	byte* buffer;
	ssize ptr;			// base-address realtive to the buffer
	ssize capacity;
	ssize last;
};

void ArenaInit(Arena* arena, void* buffer, ssize capacity) {
	arena->ptr = 0;
	arena->last = 0;
	arena->buffer = (byte*)buffer;
	arena->capacity = capacity;
}

byte* ArenaAlloc(Arena* arena, ssize size, int32 alignment = 0) {
	ASSERT(0 <= size);
	if (size == 0) return NULL;

	int32 moveToAlign = 0;
	if (alignment > 1) {
		uint32 modulo = arena->ptr & (alignment - 1);
		moveToAlign = alignment - modulo;
	}

	if (arena->capacity < arena->ptr + size + moveToAlign) return NULL;

	arena->last = arena->ptr;
	arena->ptr += moveToAlign;
	byte* data = arena->buffer + arena->ptr;
	arena->ptr += size;

	return data;
}

void ArenaFreeAll(Arena* arena) {
	arena->ptr = 0;
	arena->last = 0;
}

void ArenaReset(Arena* arena) {
	memset(arena->buffer, 0, arena->ptr);
	arena->ptr = 0;
	arena->last = 0;
}

void* ArenaReAlloc(Arena* arena, void* oldData, ssize oldSize, ssize newSize) {
	if (oldData == arena->buffer + arena->last || newSize <= oldSize)
		return oldData;

	void* result = ArenaAlloc(arena, newSize);
	memcpy(result, oldData, oldSize);
	return result;
}