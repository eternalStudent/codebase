struct Arena {
	byte* buffer;
	byte* pos;
	byte* end;

	byte* last;
	
	ssize commitSize;
};

Arena CreateArena(ssize commitSize = MB(1), ssize reserveSize = RESERVE_SIZE) {
	Arena arena;
	arena.buffer = (byte*)OSReserve(RESERVE_SIZE);
	arena.pos = arena.buffer;
	arena.last = arena.buffer;
	OSCommit(arena.buffer, commitSize);
	arena.end = arena.buffer + commitSize;
	arena.commitSize = commitSize;
	return arena;
}

void ArenaEnsureCapacity(Arena* arena, ssize size) {
	while (arena->end < arena->pos + size) {
		OSCommit(arena->end, arena->commitSize);
		arena->end += arena->commitSize;
	}
}

byte* ArenaAlloc(Arena* arena, ssize size, int32 alignment = 0) {
	ASSERT(0 <= size);

	if (size == 0) 
		return arena->pos;

	int32 moveToAlign = 0;
	if (alignment > 1) {
		uint32 modulo = ((uint64)(arena->pos)) & (alignment - 1);
		moveToAlign = alignment - modulo;
	}

	while (arena->end < arena->pos + size + moveToAlign) {
		OSCommit(arena->end, arena->commitSize);
		arena->end += arena->commitSize;
	}

	arena->last = arena->pos;
	arena->pos += moveToAlign;
	byte* data = arena->pos;
	arena->pos += size;

	return data;
}

void ArenaFreeAll(Arena* arena) {
	arena->pos = arena->buffer;
	arena->last = arena->buffer;
}

void ArenaReset(Arena* arena) {
	memset(arena->buffer, 0, arena->end - arena->pos);
	arena->pos = arena->buffer;
	arena->last = arena->buffer;
}

void* ArenaReAlloc(Arena* arena, void* oldData, ssize oldSize, ssize newSize) {
	if (oldData == NULL) 
		return ArenaAlloc(arena, newSize);

	if (newSize <= oldSize)
		return oldData;

	if (oldData == arena->last) {
		while (arena->end < arena->last + newSize) {
			OSCommit(arena->end, arena->commitSize);
			arena->end += arena->commitSize;
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
