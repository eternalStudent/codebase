#if defined(_WIN32)
#  include "win32memory.cpp"
#  define OSAllocate			Win32Allocate
#  define OSReserve				Win32Reserve
#  define OSCommit				Win32Commit
#  define OSDecommit			Win32Decommit
#  define OSFree				Win32Free
#endif

#if defined(__gnu_linux__)
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

FixedSize CreateFixedSize(Arena* arena, int32 capacity, int32 size) {
	ArenaAlign(arena, 8);
	void* buffer = ArenaAlloc(arena, capacity*size);
	memset(buffer, 0, capacity*size);
	FixedSize allocator;
	FixedSizeInit(&allocator, (byte*)buffer, capacity, size);
	return allocator;
}