#if defined(_OS_WINDOWS)
#  include "win32memory.cpp"
#  define OSAllocate			Win32Allocate
#  define OSReserve				Win32Reserve
#  define OSCommit				Win32Commit
#  define OSDecommit			Win32Decommit
#  define OSFree				Win32Free
#endif

#if defined(_OS_UNIX)
#  include <sys/mman.h>
#  include "linuxmemory.cpp"
#  define OSAllocate			LinuxAllocate
#  define OSReserve				LinuxReserve
#  define OSCommit				LinuxCommit
#  define OSDecommit			LinuxDecommit
#  define OSFree				LinuxFree
#endif

#include "arena.cpp"
#include "fixedsize.cpp"

Arena CreateArena(int64 capacity) {
	void* buffer = OSAllocate(capacity);
	Arena arena;
 	ArenaInit(&arena, buffer, capacity);
 	return arena;
}

void DestroyArena(Arena* arena) {
	OSFree(arena->buffer, arena->capacity);
}

FixedSize CreateFixedSize(int32 capacity, int32 size) {
	void* buffer = OSAllocate(capacity*size);
	memset(buffer, 0, capacity*size);
	FixedSize allocator;
	FixedSizeInit(&allocator, (byte*)buffer, capacity, size);
	return allocator;
}

FixedSize CreateFixedSize(Arena* arena, int32 capacity, int32 size) {
	void* buffer = ArenaAlloc(arena, capacity*size, 8);
	memset(buffer, 0, capacity*size);
	FixedSize allocator;
	FixedSizeInit(&allocator, (byte*)buffer, capacity, size);
	return allocator;
}