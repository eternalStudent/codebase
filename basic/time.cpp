#if defined(_OS_WINDOWS)
#  include "win32time.cpp"
#  define OSTimeInit		Win32TimeInit
#  define OSTimeStart 		Win32TimeStart
#  define OSTimePause 		Win32TimePause
#  define OSTimeStop		Win32TimeStop
#  define OSTimeRestart 	Win32TimeRestart
#elif defined(_OS_UNIX)
#  include <time.h>
#  include "linuxtime.cpp"
#  define OSTimeInit		LinuxTimeInit
#  define OSTimeStart 		LinuxTimeStart
#  define OSTimePause 		LinuxTimePause
#  define OSTimeStop		LinuxTimeStop
#  define OSTimeRestart 	LinuxTimeRestart
#endif