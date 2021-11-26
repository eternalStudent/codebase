#include <memory.h>

#include "win32memory.cpp"
#define OsHeapAllocate			Win32HeapAllocate
#define OsHeapFree				Win32HeapFree

Arena CreateArena(int64 capacity) {
	void* buffer = OsHeapAllocate(capacity);
	Arena arena;
 	ArenaInit(&arena, buffer, capacity);
 	return arena;
}