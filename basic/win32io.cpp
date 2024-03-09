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
		LOG("failed to open file");
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
		LOG("failed to open file");
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

DWORD Win32WriteFile(HANDLE hFile, LPVOID buffer, DWORD maxBytesToWrite) {
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

HANDLE Win32GetDefaultFontFile() {
	WCHAR filePath[MAX_PATH] = {};
	LPWSTR ptr = filePath + GetEnvironmentVariableW(L"windir", (LPWSTR)filePath, MAX_PATH);
	COPY(L"\\Fonts\\consola.ttf", ptr);
	return Win32OpenFile(filePath);
}

BOOL Win32FileExists(LPCWSTR pszPath) {
	return PathFileExistsW(pszPath);
}

HANDLE Win32CreateChildProcess(LPWSTR cmd) {
	HANDLE readHandle = NULL;
	HANDLE writeHandle = NULL;
	SECURITY_ATTRIBUTES attributes;

	// Set the bInheritHandle flag so pipe handles are inherited. 
	attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	attributes.bInheritHandle = TRUE;
	attributes.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	ASSERT(CreatePipe(&readHandle, &writeHandle, &attributes, 0));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	ASSERT(SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0));
	 
	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	STARTUPINFOW siStartInfo = {};
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = writeHandle;
	siStartInfo.hStdOutput = writeHandle;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION piProcInfo = {};
	 
	// Create the child process. 	
	ASSERT(CreateProcessW(
		NULL,          // application name
		cmd,     	   // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo    // receives PROCESS_INFORMATION 
	));

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);	
	CloseHandle(writeHandle);

	return readHandle;
}