#include "win32memory.cpp"
#define OsHeapAllocate			Win32HeapAllocate
#define OsHeapFree				Win32HeapFree

#include "arena.cpp"

Arena CreateArena(int64 capacity) {
	void* buffer = OsHeapAllocate(capacity);
	Arena arena;
 	ArenaInit(&arena, buffer, capacity);
 	return arena;
}