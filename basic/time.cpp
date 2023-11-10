#if defined(_OS_WINDOWS)
#  include "win32time.cpp"
#  define OsTimeInit		Win32TimeInit
#  define OsTimeStart 		Win32TimeStart
#  define OsTimePause 		Win32TimePause
#  define OsTimeStop		Win32TimeStop
#  define OsTimeRestart 	Win32TimeRestart
#elif defined(_OS_UNIX)
#  include <time.h>
#  include "linuxtime.cpp"
#  define OsTimeInit		LinuxTimeInit
#  define OsTimeStart 		LinuxTimeStart
#  define OsTimePause 		LinuxTimePause
#  define OsTimeStop		LinuxTimeStop
#  define OsTimeRestart 	LinuxTimeRestart
#endif