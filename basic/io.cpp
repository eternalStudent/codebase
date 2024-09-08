#if defined(_OS_WINDOWS)
#  include "Shlwapi.h"
#  include "win32io.cpp"
#  define File          		HANDLE
#  define FILE_ERROR 			INVALID_HANDLE_VALUE 
#  define OSOpenFile    		Win32OpenFile
#  define OSCreateFile			Win32CreateFile
#  define OSReadFile    		Win32ReadFile
#  define OSWriteFile			Win32WriteFile
#  define OSGetFileSize 		Win32GetFileSize
#  define OSCloseFile   		CloseHandle
#  define OSFileExists			PathFileExistsW
#  define OSGetDefaultFontFile	Win32GetDefaultFontFile
#  define OSCreateChildProcess  Win32CreateChildProcess
#  define STDIN 				GetStdHandle(STD_INPUT_HANDLE)
#  define STDOUT 				GetStdHandle(STD_OUTPUT_HANDLE)

#elif defined(_OS_UNIX)
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
#  define OSGetDefaultFontFile	LinuxGetDefaultFontFile
#  define OSCloseFile   		close
#  define STDIN 				0
#  define STDOUT 				1
#endif

bool OSReadAll(File file, byte* buffer, ssize length) {
	if (file == FILE_ERROR)
		return false;

	ssize size = OSGetFileSize(file);
	if (!size) 
		return false;

	buffer[size] = 0;	
	ssize left = MIN(size, length);
	byte* pos = buffer;
	ssize read;
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

	ssize size = OSGetFileSize(file);
	if (!size) 
		return {};

	byte* buffer = (byte*)ArenaAlloc(arena, size+1);
	if (!buffer)
		return {};

	buffer[size] = 0;	
	ssize left = size;
	byte* pos = buffer;
	ssize read;
	do {
		read = OSReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));
	return {buffer, (ssize)size};
}

bool OSReadAll(File file, BigBuffer* buffer) {
	if (file == FILE_ERROR)
		return false;

	ssize size = OSGetFileSize(file);
	if (!size) 
		return false;

	BigBufferEnsureCapacity(buffer, size);

	ssize left = size;
	ssize read;
	do {
		read = OSReadFile(file, buffer->pos, (int32)(left & MAX_INT32));
		buffer->pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));

	return true;
}

bool OSWriteAll(File file, byte* buffer, ssize length) {
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

void Print(String str) {
	OSWriteFile(STDOUT, str.data, (int32)str.length);
}