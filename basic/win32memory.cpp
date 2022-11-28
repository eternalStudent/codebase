LPVOID Win32Allocate(SIZE_T size) {
	return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

LPVOID Win32Reserve(SIZE_T size) {
	return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

LPVOID Win32Commit(LPVOID data, SIZE_T size) {
	return VirtualAlloc(data, size, MEM_COMMIT, PAGE_READWRITE);
}

BOOL Win32Free(LPVOID data, SIZE_T size) {
	return VirtualFree(data, 0, MEM_RELEASE);
}

BOOL Win32Decommit(LPVOID data) {
	return VirtualFree(data, 0, MEM_DECOMMIT);
}