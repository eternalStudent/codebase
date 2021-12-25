void Win32ExitFullScreen(HWND window) {
	DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
	SetWindowLong(window, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window, NULL, 0, 0, 920, 540, SWP_FRAMECHANGED);
}

void UpdateDimensions(Dimensions2i dimensions);

Dimensions2i Win32GetWindowDimensions(HWND window) {
	RECT clientRect;
	GetClientRect(window, &clientRect);
	LONG width = clientRect.right;
	LONG height = clientRect.bottom;
	return Dimensions2i{width, height};
}

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CLOSE: {
			DestroyWindow(window);
			return 0;
		}
    	case WM_DESTROY: {
    		PostQuitMessage(0);
    		return 0;
        }
        case WM_SIZE: {
        	Dimensions2i dimensions = Win32GetWindowDimensions(window);
        	UpdateDimensions(dimensions);
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

HWND Win32CreateWindow(LPCSTR title, LONG width, LONG height) {
	WNDCLASSA windowClass = CreateWindowClass();

	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT rect = {0, 0, width, height};
	BOOL success = AdjustWindowRect(&rect, style, FALSE);
	if (!success)
		Log("failed to adjust window size");

	return CreateWindowExA(0,
		windowClass.lpszClassName, 
		title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, windowClass.hInstance, 0);
}

BOOL GetPrimaryMonitorRect(RECT* monitorRect){
	const POINT ptZero = {0, 0};
	HMONITOR monitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFO mi = { sizeof(mi) };

	if (GetMonitorInfo(monitor, &mi) == FALSE) {
		Log("failed to get monitor-info");
		return FALSE;
	}

	*monitorRect = mi.rcMonitor;
	return TRUE;
}

HWND Win32CreateWindowFullScreen(LPCSTR title, int32* width, int32* height) {
	WNDCLASSA windowClass = CreateWindowClass();
	RECT monitorRect;
	if (!GetPrimaryMonitorRect(&monitorRect))
		return NULL;

	*width = monitorRect.right - monitorRect.left;
	*height = monitorRect.bottom - monitorRect.top;

	HWND window = CreateWindowExA(0,
		windowClass.lpszClassName, 
		title,
		WS_VISIBLE,
		monitorRect.left,
		monitorRect.top,
		*width,
		*height,
		0, 0, windowClass.hInstance, 0);
	
	// NOTE: WTF ?!?
	DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
	SetWindowLong(window, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);

	return window;
}

UINT Win32HandleWindowEvents(HWND window) {
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_KEYUP)
		{
			uint32 VKCode = (uint32)message.wParam;
			int32 wasDown = ((message.lParam & (1 << 30)) != 0);
			int32 isDown = ((message.lParam & (1 << 31)) == 0);
			if (wasDown != isDown) return VKCode;
		}
		else {
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}
	return message.message;
}

void Win32SetCursorIcon(LPCSTR icon) {
	SetCursor(LoadCursorA(NULL, icon));
}