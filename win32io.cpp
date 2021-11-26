HANDLE Win32OpenFile(const char* filePath) {
    HANDLE hFile = CreateFileA(filePath, 
        GENERIC_READ, 
        FILE_SHARE_READ,
        NULL, // securtiy mode
        OPEN_EXISTING, 
        0,   // attributes
        NULL // template file 
        );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Log("failed to open file");
        CloseHandle(hFile);
        return NULL;
    }
    return hFile;
}

DWORD Win32ReadFile(HANDLE hFile, void* buffer, DWORD maxBytesToRead) {
    DWORD bytesRead;
    if (FALSE == ReadFile(hFile, buffer, maxBytesToRead, &bytesRead, 0)) {
        Log("failed to read file");
    }
    return bytesRead;
}

LONGLONG Win32GetFileSize(HANDLE hFile) {
    LARGE_INTEGER size;
    GetFileSizeEx(hFile, &size);
    return size.QuadPart;
}