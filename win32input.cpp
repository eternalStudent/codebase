Point2i Win32GetCursorPosition(HWND window){
	POINT cursorPos;
	GetCursorPos(&cursorPos); // relative to screen

	ScreenToClient(window, &cursorPos);
	return Point2i{cursorPos.x, cursorPos.y};
}