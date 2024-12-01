HANDLE Win32OpenFile(LPCWSTR filePath) {
	HANDLE hFile = CreateFileW(
		filePath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, // securtiy mode
		OPEN_EXISTING, 
		0,    // attributes
		NULL  // template file 
	); 
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		(void)error;
		OutputDebugStringA("failed to open file\n");
	}
	return hFile;
}

HANDLE Win32CreateFile(LPCWSTR filePath) {
	HANDLE hFile = CreateFileW(
		filePath, 
		GENERIC_WRITE, 
		0,    // sharing mode
		NULL, // securtiy mode
		CREATE_ALWAYS, 
		0,    // attributes
		NULL  // template file 
	); 
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		(void)error;
		OutputDebugStringA("failed to open file\n");
	}
	return hFile;
}

DWORD Win32ReadFile(HANDLE hFile, LPVOID buffer, DWORD maxBytesToRead) {
	DWORD bytesRead;
	if (FALSE == ReadFile(hFile, buffer, maxBytesToRead, &bytesRead, 0)) {
		DWORD error = GetLastError();
		(void)error;
		OutputDebugStringA("failed to read file\n");
	}
	return bytesRead;
}

DWORD Win32WriteFile(HANDLE hFile, LPVOID buffer, DWORD maxBytesToWrite) {
	DWORD bytesWritten;
	if (FALSE == WriteFile(hFile, buffer, maxBytesToWrite, &bytesWritten, NULL)) {
		DWORD error = GetLastError();
		(void)error;
		OutputDebugStringA("failed to write to file\n");
	}
	return bytesWritten;
}

LONGLONG Win32GetFileSize(HANDLE hFile) {
	LARGE_INTEGER size;
	GetFileSizeEx(hFile, &size);
	return size.QuadPart;
}

HANDLE Win32GetDefaultFontFile() {
	WCHAR filePath[MAX_PATH] = {};
	LPWSTR ptr = filePath + GetEnvironmentVariableW(L"windir", (LPWSTR)filePath, MAX_PATH);
	COPY(L"\\Fonts\\consola.ttf", ptr);
	return Win32OpenFile(filePath);
}

enum CreateProcessFlags {
	CPF_None  = 0,
	CPF_Debug = 1
};

// NOTE: https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
void Win32CreateChildProcess(LPWSTR cmd, CreateProcessFlags creationFlags, 
							 HANDLE* outReadHandle = NULL,
							 HANDLE* processHandle = NULL,
							 HANDLE* outWriteHandle = NULL,
							 HANDLE* threadHandle = NULL) {

	HANDLE readHandle = NULL;
	HANDLE writeHandle = NULL;

	// NOTE: Set the bInheritHandle flag so pipe handles are inherited. 
	SECURITY_ATTRIBUTES attributes;
	attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	attributes.bInheritHandle = TRUE;
	attributes.lpSecurityDescriptor = NULL;

	ASSERT(CreatePipe(&readHandle, &writeHandle, &attributes, 0));

	// NOTE: Ensure the read handle to the pipe for STDOUT is not inherited.
	ASSERT(SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0));
	 
	STARTUPINFOW si = {};
	si.cb = sizeof(STARTUPINFOW);
	si.hStdError = writeHandle;
	si.hStdOutput = writeHandle;
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi = {};

	DWORD dwCreationFlags = 0;
	if ( (creationFlags&CPF_Debug) != 0 ) dwCreationFlags |= DEBUG_ONLY_THIS_PROCESS;
	 
	ASSERT(CreateProcessW(
		NULL,		// application name
		cmd, 
		NULL,		// process security attributes 
		NULL,		// primary thread security attributes 
		TRUE,		// handles are inherited 
		dwCreationFlags,
		NULL,		// use parent's environment 
		NULL,		// use parent's current directory 
		&si, 		// STARTUPINFO pointer 
		&pi  		// receives PROCESS_INFORMATION 
	));

	if (outReadHandle ) *outReadHandle  = readHandle ; else CloseHandle(readHandle );
	if (outWriteHandle) *outWriteHandle = writeHandle; else CloseHandle(writeHandle);
	if (processHandle ) *processHandle  = pi.hProcess; else CloseHandle(pi.hProcess);
	if (threadHandle  ) *threadHandle   = pi.hThread ; else CloseHandle(pi.hThread );
}