struct {
	HWND handle;
	Dimensions2i dim;
	BOOL destroyed;
	BYTE keys[256];
	BYTE mouse[3];
} _window;

HWND Win32GetWindowHandle() {
	return _window.handle;
}

Dimensions2i Win32GetWindowDimensions() {
	return _window.dim;
}

BOOL Win32IsKeyDown(DWORD key) {
	return _window.keys[key] == 1;
}

BOOL Win32IsMouseDown(DWORD mouse) {
	return _window.mouse[mouse] == 1;
}

void Win32ExitFullScreen() {
	HWND window = _window.handle;
	DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
	SetWindowLong(window, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window, NULL, 0, 0, 920, 540, SWP_FRAMECHANGED);
	memset(_window.keys, 0, 256);
	memset(_window.mouse, 0, 3);
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
	HWND window = _window.handle;
	RECT monitorRect;
	if (!GetPrimaryMonitorRect(&monitorRect))
		return FAIL("failed to get monitor placement");

	DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
	SetWindowLong(window, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
	SetWindowPos(window, NULL, 0, 0, monitorRect.right, monitorRect.bottom, SWP_FRAMECHANGED);
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

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY: {
			_window.destroyed = true;
			return 0;
		}
		case WM_SIZE: {
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			_window.dim = {(int32)width, (int32)height};
		} break;
		case WM_KEYDOWN: {
			UINT vkcode = (UINT)wParam;
			_window.keys[vkcode] = 1;
		} break;
		case WM_KEYUP: {
			UINT vkcode = (UINT)wParam;
			_window.keys[vkcode] = 0;
		} break;
		case WM_LBUTTONDOWN : {
			_window.mouse[0] = 1;
		} break;
		case WM_LBUTTONUP : {
			_window.mouse[0] = 0;
		} break;
		case WM_MBUTTONDOWN : {
			_window.mouse[1] = 1;
		} break;
		case WM_MBUTTONUP : {
			_window.mouse[1] = 0;
		} break;
		case WM_RBUTTONDOWN : {
			_window.mouse[2] = 1;
		} break;
		case WM_RBUTTONUP : {
			_window.mouse[2] = 0;
		} break;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

WNDCLASSA CreateWindowClass() {
	WNDCLASSA windowClass = {};
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.lpszClassName = "WindowClass";
	windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);

	RegisterClassA(&windowClass);

	return windowClass;
}

BOOL Win32WindowDestroyed() {
	return _window.destroyed;
}

void Win32CreateWindow(LPCSTR title, LONG width, LONG height) {
	WNDCLASSA windowClass = CreateWindowClass();
	_window = {};

	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT rect = {0, 0, width, height};
	BOOL success = AdjustWindowRect(&rect, style, FALSE);
	if (!success)
		LOG("failed to adjust window size");

	_window.dim = {width, height};
	_window.handle = CreateWindowExA(0,
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
	_window = {};
	
	RECT monitorRect;
	if (!GetPrimaryMonitorRect(&monitorRect)) {
		LOG("failed to get monitor placement");
		return;
	}

	LONG width = monitorRect.right - monitorRect.left;
	LONG height = monitorRect.bottom - monitorRect.top;

	_window.dim = {width, height};
	_window.handle = CreateWindowExA(0,
		windowClass.lpszClassName, 
		title,
		WS_VISIBLE,
		monitorRect.left,
		monitorRect.top,
		width,
		height,
		0, 0, windowClass.hInstance, 0);
	
	// NOTE: WTF ?!?
	DWORD dwStyle = GetWindowLong(_window.handle, GWL_STYLE);
	SetWindowLong(_window.handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
}

void Win32HandleWindowEvents() {
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

void Win32SetCursorIcon(LPCSTR icon) {
	SetCursor(LoadCursorA(NULL, icon));
}

BOOL Win32OpenFileDialog(LPSTR path, DWORD max) {
	OPENFILENAME dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = max;
	dialog.hwndOwner = _window.handle;
	dialog.Flags = OFN_FILEMUSTEXIST;
	dialog.Flags |= OFN_NOCHANGEDIR;
	return GetOpenFileName(&dialog);
}

BOOL Win32SaveFileDialog(LPSTR path, DWORD max) {
	OPENFILENAME dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = max;
	dialog.hwndOwner = _window.handle;
	dialog.Flags = OFN_OVERWRITEPROMPT;
	dialog.Flags |= OFN_NOCHANGEDIR;
	return GetSaveFileName(&dialog);
}

Point2i Win32GetCursorPosition() {
	HWND window = _window.handle;
	POINT cursorPos;
	GetCursorPos(&cursorPos); // relative to screen

	ScreenToClient(window, &cursorPos);
	return Point2i{cursorPos.x, cursorPos.y};
}