#include "win32io.cpp"
#define File          HANDLE
#define OsOpenFile    Win32OpenFile
#define OsReadFile    Win32ReadFile
#define OsGetFileSize Win32GetFileSize
#define OsCloseFile   CloseHandle


String ReadAll(Arena* arena, const char* path) {
	byte* buffer = NULL;
	int64 size = 0;

	// NOTE: goto shouldn't skip variable declarations
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
	} while (!read);

CloseAndReturn:
	OsCloseFile(file);
	return String{buffer, size};
}