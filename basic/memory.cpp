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

#define RESERVE_SIZE	1024*1024*1024*16ull
#define CHUNK_SIZE 		1024*1024*16

#include "arena.cpp"
#include "bigarray.cpp"
#include "fixedsize.cpp"
