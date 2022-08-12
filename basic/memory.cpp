#if defined(_WIN32)
#  include "win32memory.cpp"
#  define OsHeapAllocate			Win32HeapAllocate
#  define OsHeapFree				Win32HeapFree
#endif

#if defined(__gnu_linux__)
#  include <sys/mman.h>
#  include "linuxmemory.cpp"
#  define OsHeapAllocate			LinuxHeapAllocate
#  define OsHeapFree				LinuxHeapFree
#endif

#include "arena.cpp"
#include "fixedsize.cpp"

Arena CreateArena(int64 capacity) {
	void* buffer = OsHeapAllocate(capacity);
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