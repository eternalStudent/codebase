struct Arena {
	byte* buffer;
	ssize ptr;			// base-address realtive to the buffer
	ssize capacity;
	ssize last;
};

Arena CreateArena() {
	Arena arena;
	arena.ptr = 0;
	arena.last = 0;
	arena.buffer = (byte*)OSReserve(RESERVE_SIZE);
	OSCommit(arena.buffer, CHUNK_SIZE);
	arena.capacity = CHUNK_SIZE;
	return arena;
}

byte* ArenaAlloc(Arena* arena, ssize size, int32 alignment = 0) {
	ASSERT(0 <= size);

	if (size == 0) 
		return arena->buffer + arena->ptr;

	int32 moveToAlign = 0;
	if (alignment > 1) {
		uint32 modulo = arena->ptr & (alignment - 1);
		moveToAlign = alignment - modulo;
	}

	while (arena->capacity < arena->ptr + size + moveToAlign) {
		OSCommit(arena->buffer + arena->capacity, CHUNK_SIZE);
		arena->capacity += CHUNK_SIZE;
	}

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
	if (oldData == NULL) 
		return ArenaAlloc(arena, newSize);

	if (newSize <= oldSize)
		return oldData;

	if (oldData == arena->buffer + arena->last) {
		while (arena->capacity < arena->last + newSize) {
			OSCommit(arena->buffer + arena->capacity, CHUNK_SIZE);
		    arena->capacity += CHUNK_SIZE;
		}
		return oldData;
	}

	void* result = ArenaAlloc(arena, newSize);
	memcpy(result, oldData, oldSize);
	return result;
}

void DestroyArena(Arena* arena) {
	OSFree(arena->buffer, RESERVE_SIZE);
}
