#if defined(_WIN32)
#  include "win32io.cpp"
#  define File          		HANDLE
#  define FILE_ERROR 			INVALID_HANDLE_VALUE 
#  define OsOpenFile    		Win32OpenFile
#  define OsCreateFile 			Win32CreateFile
#  define OsReadFile    		Win32ReadFile
#  define OsWriteFile			Win32WriteFile
#  define OsGetFileSize 		Win32GetFileSize
#  define OsCloseFile   		CloseHandle
#  define STDIN 				GetStdHandle(STD_INPUT_HANDLE)
#  define STDOUT 				GetStdHandle(STD_OUTPUT_HANDLE)

#endif

#if defined(__gnu_linux__)
#  include <fcntl.h>
#  include <sys/stat.h>
#  include "linuxio.cpp"
#  define File          		int
#  define FILE_ERROR 			-1
#  define OsOpenFile    		LinuxOpenFile
#  define OsCreateFile 			LinuxCreateFile
#  define OsReadFile    		LinuxReadFile
#  define OsWriteFile			LinuxWriteFile
#  define OsGetFileSize 		LinuxGetFileSize
#  define OsCloseFile   		close
#  define STDIN 				0
#  define STDOUT 				1
#endif

bool OsReadAll(File file, byte* buffer, int64 length) {
	if (file == FILE_ERROR)
		return false;

	int64 size = OsGetFileSize(file);
	if (!size) 
		return false;

	buffer[size] = 0;	
	int64 left = MIN(size, length);
	byte* pos = buffer;
	int64 read;
	do {
		read = OsReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));
	return true;
}

String OsReadAll(File file, Arena* arena) {
	if (file == FILE_ERROR)
		return {};

	int64 size = OsGetFileSize(file);
	if (!size) 
		return {};

	byte* buffer = (byte*)ArenaAlloc(arena, size);
	if (!buffer)
		return {};

	buffer[size] = 0;	
	int64 left = size;
	byte* pos = buffer;
	int64 read;
	do {
		read = OsReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while ((0 < left) && (0 < read));
	return {buffer, size};
}

String OsReadAll(const char* path, Arena* arena) {
	File file = OsOpenFile(path);
	String all = OsReadAll(file, arena);
	OsCloseFile(file);
	return all;
}

bool OsWriteAll(File file, byte* buffer, int64 length) {
	if (file == FILE_ERROR)
		return false;

	int64 size = length;
	int64 left = size;
	byte* pos = buffer;
	int64 written;
	do {
		written = OsWriteFile(file, pos, (int32)(left & MAX_INT32));
		pos  += written;
		left -= written;
	} while ((0 < left) && (0 < written));
	return true;
}