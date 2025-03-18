#if defined(OS_WINDOWS)
#  include "win32memory.cpp"
#  define OSAllocate			Win32Allocate
#  define OSReserve				Win32Reserve
#  define OSCommit				Win32Commit
#  define OSDecommit			Win32Decommit
#  define OSFree				Win32Free
#endif

#if defined(OS_UNIX)
#  include <sys/mman.h>
#  include "linuxmemory.cpp"
#  define OSAllocate			LinuxAllocate
#  define OSReserve				LinuxReserve
#  define OSCommit				LinuxCommit
#  define OSDecommit			LinuxDecommit
#  define OSFree				LinuxFree
#endif

#define KB(n)	(((ssize)(n)) << 10)
#define MB(n)	(((ssize)(n)) << 20)
#define GB(n)	(((ssize)(n)) << 30)
#define RESERVE_SIZE		GB(1)

#include "arena.cpp"
#include "bigarray.cpp"
#include "fixedsize.cpp"
