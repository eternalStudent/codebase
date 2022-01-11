#include "win32io.cpp"
#define File          		HANDLE
#define OsOpenFile    		Win32OpenFile
#define OsCreateFile 		Win32CreateFile
#define OsReadFile    		Win32ReadFile
#define OsWriteFile			Win32WriteFile
#define OsGetFileSize 		Win32GetFileSize
#define OsCloseFile   		CloseHandle


String OsReadAll(Arena* arena, const char* path) {
	byte* buffer = NULL;
	int64 size = 0;
	int32 read;
	int64 left;
	byte* pos;

	File file = OsOpenFile(path);
	if (!file) goto CloseAndReturn;

	size = OsGetFileSize(file);
	if (!size) goto CloseAndReturn;

	buffer = ArenaAlloc(arena, size+1);
	if (!buffer) goto CloseAndReturn;
	buffer[size] = 0;
	
	left = size;
	pos = buffer;
	do {
		read = OsReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while (0<left && 0<read);

CloseAndReturn:
	OsCloseFile(file);
	return String{buffer, size};
}

String OsReadToBuffer(byte* buffer, const char* path) {
	int64 size = 0;
	int32 read;
	int64 left;
	byte* pos;

	File file = OsOpenFile(path);
	if (!file) goto CloseAndReturn;

	size = OsGetFileSize(file);
	if (!size) goto CloseAndReturn;
	
	left = size;
	pos = buffer;
	do {
		read = OsReadFile(file, pos, (int32)(left & MAX_INT32));
		pos  += read;
		left -= read;
	} while (0<left && 0<read);

CloseAndReturn:
	OsCloseFile(file);
	return String{buffer, size};
}

void OsWriteAll(String str, const char* path) {
	int64 size;
	int32 written;
	int64 left;
	byte* pos;

	File file = OsCreateFile(path);
	if (!file) goto CloseAndReturn;

	size = str.length;
	left = size;
	pos = str.data;
	do {
		written = OsWriteFile(file, pos, (int32)(left & MAX_INT32));
		pos  += written;
		left -= written;
	} while (0<left && 0<written);

CloseAndReturn:
	OsCloseFile(file);
}