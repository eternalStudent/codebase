BOOL Win32CopyToClipboard(String str) {
	if (!OpenClipboard(NULL)) {
		LOG("failed to open clipboard");
		return FALSE;

	if (!EmptyClipboard()) {
		LOG("failed to empty clipboard");
		return FALSE;
	}

	HGLOBAL hMem = GlobalAlloc(GMEM_DDESHARE, str.length + 1);
	if (!hMem) {
		LOG("failed to allocate memory");
		return FALSE;
	}

	LPVOID pchData = GlobalLock(hMem);
	memcpy(pchData, str.data, str.length);
	if (GlobalUnlock(hMem)) {
		LOG("failed to unlock memory");
		return FALSE;
	}

	if (SetClipboardData(CF_TEXT, hMem)) {
		LOG("failed to set clipboard data");
		return FALSE;
	}
	 
	CloseClipboard();
	return TRUE;
}
	