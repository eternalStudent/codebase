#if defined(_WIN32)
#  include "win32io.cpp"
#  define File          		HANDLE
#  define FILE_ERROR 			INVALID_HANDLE_VALUE 
#  define OSOpenFile    		Win32OpenFile
#  define OSCreateFile			Win32CreateFile
#  define OSReadFile    		Win32ReadFile
#  define OSWriteFile			Win32WriteFile
#  define OSGetFileSize 		Win32GetFileSize
#  define OSCloseFile   		CloseHandle
#  define STDIN 				GetStdHandle(STD_INPUT_HANDLE)
#  define STDOUT 				GetStdHandle(STD_OUTPUT_HANDLE)
#endif

#if defined(__gnu_linux__)
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <errno.h>
#  include <limits.h>
#  include "linuxio.cpp"
#  define File          		int
#  define FILE_ERROR 			-1
#  define OSOpenFile    		LinuxOpenFile
#  define OSCreateFile			LinuxCreateFile
#  define OSReadFile    		LinuxReadFile
#  define OSWriteFile			LinuxWriteFile
#  define OSGetFileSize 		LinuxGetFileSize
#  define OSCloseFile   		close
#  define STDIN 				0
#  define STDOUT 				1
#endif

bool OSReadAll(File file, byte* buffer, int64 length) {
	if (file == FILE_ERROR)
		return false;

	int64 size = OSGetFileSize(file);
	if (!size) 
		return false;

	buffer[size] = 0;	
	int64 left = MIN(size, length);
	byte* pos = buffer;
	int64 read;
	do {
		read = OSReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));
	return true;
}

String OSReadAll(File file, Arena* arena) {
	if (file == FILE_ERROR)
		return {};

	int64 size = OSGetFileSize(file);
	if (!size) 
		return {};

	byte* buffer = (byte*)ArenaAlloc(arena, size+1);
	if (!buffer)
		return {};

	buffer[size] = 0;	
	int64 left = size;
	byte* pos = buffer;
	int64 read;
	do {
		read = OSReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));
	return {buffer, (ssize)size};
}

String OSReadAll(const char* path, Arena* arena) {
	File file = OSOpenFile(path);
	String all = OSReadAll(file, arena);
	OSCloseFile(file);
	return all;
}

bool OSWriteAll(File file, byte* buffer, int64 length) {
	if (file == FILE_ERROR)
		return false;

	int64 size = length;
	int64 left = size;
	byte* pos = buffer;
	int64 written;
	do {
		written = OSWriteFile(file, pos, (int32)(left & MAX_INT32));
		pos  += written;
		left -= written;
	} while ((0 < left) && (0 < written));
	return true;
}