HANDLE Win32OpenFile(LPCSTR filePath) {
    HANDLE hFile = CreateFile(
        filePath, 
        GENERIC_READ, 
        FILE_SHARE_READ,
        NULL, // securtiy mode
        OPEN_EXISTING, 
        0,    // attributes
        NULL  // template file 
    ); 
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG("failed to open file");
        CloseHandle(hFile);
        return NULL;
    }
    return hFile;
}

HANDLE Win32CreateFile(LPCSTR filePath) {
    HANDLE hFile = CreateFile(
        filePath, 
        GENERIC_WRITE, 
        0,    // sharing mode
        NULL, // securtiy mode
        CREATE_ALWAYS, 
        0,    // attributes
        NULL  // template file 
    ); 
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG("failed to open file");
        CloseHandle(hFile);
        return NULL;
    }
    return hFile;
}

DWORD Win32ReadFile(HANDLE hFile, LPVOID buffer, DWORD maxBytesToRead) {
    DWORD bytesRead;
    if (FALSE == ReadFile(hFile, buffer, maxBytesToRead, &bytesRead, 0)) {
        LOG("failed to read file");
    }
    return bytesRead;
}

DWORD Win32WriteFile(HANDLE hFile, LPVOID buffer, DWORD maxBytesToWrite){
    DWORD bytesWritten;
    if (FALSE == WriteFile(hFile, buffer, maxBytesToWrite, &bytesWritten, NULL)) {
        LOG("failed to write to file");
    }
    return bytesWritten;
}

LONGLONG Win32GetFileSize(HANDLE hFile) {
    LARGE_INTEGER size;
    GetFileSizeEx(hFile, &size);
    return size.QuadPart;
}