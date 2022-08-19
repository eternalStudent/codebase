#if defined(_WIN32)
#  include "win32clipboard.cpp"
#  define OSCopyToClipboard 	Win32CopyToClipboard
#endif