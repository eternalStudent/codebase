LPVOID Win32HeapAllocate(SIZE_T size) {
	return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

BOOL Win32HeapFree(LPVOID data) {
	return HeapFree(GetProcessHeap(), 0, data);
}