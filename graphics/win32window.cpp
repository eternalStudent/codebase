struct {
	HWND handle;
	Dimensions2i dim;
	BOOL destroyed;
	BYTE keys[256];
	BYTE mouse[4];
	BYTE keys_prev[256];
	BYTE mouse_prev[3];
	DWORD mouseWheelDelta;
	DWORD mouseHWheelDelta;
	CHAR typed[4];
	int32 strlength;
	HCURSOR cursors[7];
} window;

HWND Win32GetWindowHandle() {
	return window.handle;
}

Dimensions2i Win32GetWindowDimensions() {
	return window.dim;
}

BOOL Win32IsKeyDown(DWORD key) {
	return window.keys[key] == 1;
}

BOOL Win32IsKeyPressed(DWORD key) {
	return window.keys[key] == 1 && window.keys_prev[key] == 0;
}

BOOL Win32IsMouseDown(DWORD mouse) {
	return window.mouse[mouse] == 1;
}

BOOL Win32IsMousePressed(DWORD mouse) {
	return window.mouse[mouse] == 1 && window.mouse_prev[mouse] == 0;
}

BOOL Win32IsMouseDoubleClicked() {
	return window.mouse[3] == 1;
}

DWORD Win32GetMouseWheelDelta() {
	return window.mouseWheelDelta;
}

DWORD Win32GetMouseHWheelDelta() {
	return window.mouseHWheelDelta;
}

String Win32GetTypedText() {
	return {(byte*)window.typed, (ssize)window.strlength};
}

void Win32ExitFullScreen() {
	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window.handle, NULL, 0, 0, 920, 540, SWP_FRAMECHANGED);
	memset(window.keys, 0, 256);
	memset(window.mouse, 0, 3);
}

BOOL GetPrimaryMonitorRect(RECT* monitorRect){
	const POINT ptZero = {0, 0};
	HMONITOR monitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFO mi = { sizeof(mi) };

	if (GetMonitorInfo(monitor, &mi) == FALSE) {
		FAIL("failed to get monitor-info");
	}

	*monitorRect = mi.rcMonitor;
	return TRUE;
}

BOOL Win32EnterFullScreen() {
	RECT monitorRect;
	if (!GetPrimaryMonitorRect(&monitorRect))
		return FAIL("failed to get monitor placement");

	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
	SetWindowPos(window.handle, NULL, 0, 0, monitorRect.right, monitorRect.bottom, SWP_FRAMECHANGED);
	return TRUE;
}

/*
Dimensions2i Win32GetWindowDimensions(HWND window) {
	RECT clientRect;
	GetClientRect(window, &clientRect);
	LONG width = clientRect.right;
	LONG height = clientRect.bottom;
	return Dimensions2i{width, height};
}*/

LRESULT CALLBACK MainWindowCallback(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY: {
			window.destroyed = true;
			return 0;
		}
		case WM_SIZE: {
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			window.dim = {(int32)width, (int32)height};
		} break;
		case WM_KEYDOWN: {
			UINT vkcode = (UINT)wParam;
			window.keys[vkcode] = 1;
		} break;
		case WM_KEYUP: {
			UINT vkcode = (UINT)wParam;
			window.keys[vkcode] = 0;
		} break;
		case WM_LBUTTONDOWN : {
			window.mouse[0] = 1;
		} break;
		case WM_LBUTTONUP : {
			window.mouse[0] = 0;
		} break;
		case WM_MBUTTONDOWN : {
			window.mouse[1] = 1;
		} break;
		case WM_MBUTTONUP : {
			window.mouse[1] = 0;
		} break;
		case WM_RBUTTONDOWN : {
			window.mouse[2] = 1;
		} break;
		case WM_RBUTTONUP : {
			window.mouse[2] = 0;
		} break;
		case WM_MOUSEWHEEL: {
			window.mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		} break;
		case WM_MOUSEHWHEEL: {
			window.mouseHWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		} break;
		case WM_LBUTTONDBLCLK: {
			window.mouse[3] = 1;
		} break;
		case WM_CHAR: {
			if (wParam == 13) wParam = 10;
			if (32 <= wParam && wParam <= 127 || 9 <= wParam && wParam <= 10) {
				window.typed[window.strlength] = (CHAR)wParam;
				window.strlength++;
			}
		}
	}

	return DefWindowProc(handle, message, wParam, lParam);
}

WNDCLASSA CreateWindowClass() {
	WNDCLASSA windowClass = {};
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.lpszClassName = "WindowClass";

	RegisterClassA(&windowClass);

	return windowClass;
}

BOOL Win32WindowDestroyed() {
	return window.destroyed;
}

void LoadCursors() {
	window.cursors[CUR_ARROW] = LoadCursorA(NULL, IDC_ARROW);
	window.cursors[CUR_MOVE] = LoadCursorA(NULL, IDC_SIZEALL);	
	window.cursors[CUR_RESIZE] = LoadCursorA(NULL, IDC_SIZENWSE);
	window.cursors[CUR_HAND] = LoadCursorA(NULL, IDC_HAND);
	window.cursors[CUR_MOVESIDE] = LoadCursorA(NULL, IDC_SIZEWE);
	window.cursors[CUR_MOVEUPDN] = LoadCursorA(NULL, IDC_SIZENS);
	window.cursors[CUR_TEXT] = LoadCursorA(NULL, IDC_IBEAM);
}

void Win32CreateWindow(LPCSTR title, LONG width, LONG height) {
	WNDCLASSA windowClass = CreateWindowClass();
	window = {};
	LoadCursors();

	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT rect = {0, 0, width, height};
	BOOL success = AdjustWindowRect(&rect, style, FALSE);
	if (!success)
		LOG("failed to adjust window size");

	window.dim = {width, height};
	window.handle = CreateWindowExA(0,
		windowClass.lpszClassName, 
		title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, windowClass.hInstance, 0);
}

void Win32CreateWindowFullScreen(LPCSTR title) {
	WNDCLASSA windowClass = CreateWindowClass();
	window = {};
	LoadCursors();
	
	RECT monitorRect;
	if (!GetPrimaryMonitorRect(&monitorRect)) {
		LOG("failed to get monitor placement");
		return;
	}

	LONG width = monitorRect.right - monitorRect.left;
	LONG height = monitorRect.bottom - monitorRect.top;

	window.dim = {width, height};
	window.handle = CreateWindowExA(0,
		windowClass.lpszClassName, 
		title,
		WS_VISIBLE,
		monitorRect.left,
		monitorRect.top,
		width,
		height,
		0, 0, windowClass.hInstance, 0);
	
	// NOTE: WTF ?!?
	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
}

void Win32ProcessWindowEvents() {
	for (int32 i = 0; i < 256; i++) window.keys_prev[i] = window.keys[i];
	window.mouse_prev[0] = window.mouse[0];
	window.mouse_prev[1] = window.mouse[1];
	window.mouse_prev[2] = window.mouse[2];
	window.mouse[3] = 0;
	window.strlength = 0;
	window.mouseWheelDelta = 0;
	window.mouseHWheelDelta = 0;
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

void Win32SetCursorIcon(int32 icon) {
	SetCursor(window.cursors[icon]);
}

HANDLE Win32OpenFileDialog() {
	CHAR path[MAX_PATH];
	OPENFILENAME dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.hwndOwner = window.handle;
	dialog.Flags = OFN_FILEMUSTEXIST;
	dialog.Flags |= OFN_NOCHANGEDIR;
	BOOL success = GetOpenFileName(&dialog);
	if (!success) {
		LOG("failed to get save file name");
		return INVALID_HANDLE_VALUE ;
	}
	return Win32OpenFile(path);
}

HANDLE Win32SaveFileDialog() {
	CHAR path[MAX_PATH];
	OPENFILENAME dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.hwndOwner = window.handle;
	dialog.Flags = OFN_OVERWRITEPROMPT;
	dialog.Flags |= OFN_NOCHANGEDIR;
	BOOL success = GetSaveFileName(&dialog);
	if (!success) {
		LOG("failed to get save file name");
		return INVALID_HANDLE_VALUE ;
	}
	return Win32CreateFile(path);
}

Point2i Win32GetCursorPosition() {
	POINT cursorPos;
	GetCursorPos(&cursorPos); // relative to screen

	ScreenToClient(window.handle, &cursorPos);
	return Point2i{cursorPos.x, cursorPos.y};
}

BOOL Win32CopyToClipboard(String str) {
	if (!OpenClipboard(window.handle)) {
		LOG("failed to open clipboard");
		return FALSE;
	}

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
	if (!GlobalUnlock(hMem)) {
		LOG("failed to unlock memory");
		return FALSE;
	}

	if (!SetClipboardData(CF_TEXT, hMem)) {
		LOG("failed to set clipboard data");
		return FALSE;
	}
	 
	CloseClipboard();
	return TRUE;
}