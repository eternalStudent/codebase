#if (!defined(WM_MOUSEHWHEEL))
#define WM_MOUSEHWHEEL 0x020e
#endif

#define EVENT_QUEUE_CAP			8
#define EVENT_QUEUE_MASK		(EVENT_QUEUE_CAP - 1)

struct EventQueue {
	OSEvent table[EVENT_QUEUE_CAP];
	volatile uint32 writeIndex;
	volatile uint32 readIndex;
};

struct {
	HWND handle;

	Dimensions2i dim;
	BOOL destroyed;

	int32 clickCount;
	RECT clickRect;
	LONG timeLastClicked;

	BOOL iconSet;
	int32 icon;

	HCURSOR cursors[CUR_COUNT];

	EventQueue queue;
} window;

HWND Win32GetWindowHandle() {
	return window.handle;
}

Dimensions2i Win32GetWindowDimensions() {
	return window.dim;
}

BOOL Win32IsKeyDown(int vkCode) {
	return (GetKeyState(vkCode) & 0x8000) != 0;
}

void Win32ExitFullScreen() {
	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window.handle, NULL, 0, 0, 920, 540, SWP_FRAMECHANGED);
}

void Win32SetWindowToNoneResizable() {
	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle & ~WS_THICKFRAME);
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

void Win32EnqueueEvent(OSEvent event) {
	window.queue.table[window.queue.writeIndex & EVENT_QUEUE_MASK] = event;
	MemoryBarrier();
	window.queue.writeIndex++;
}

BOOL Win32PollEvent(OSEvent* event) {
	if (window.queue.readIndex == window.queue.writeIndex) return FALSE;
	*event = window.queue.table[window.queue.readIndex & EVENT_QUEUE_MASK];
	MemoryBarrier();
	window.queue.readIndex++;
	return TRUE;
}

LRESULT CALLBACK MainWindowCallback(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY: {
			window.destroyed = true;
			return 0;
		}
		case WM_SIZE: {
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			Dimensions2i dim = {(int32)width, (int32)height};

			OSEvent event;
			event.type = Event_WindowResize;
			event.time = GetMessageTime();
			event.window.dim = dim;
			event.window.prevDim = window.dim;
			Win32EnqueueEvent(event);

			window.dim = dim;
		} break;
		case WM_ACTIVATE: {
			window.clickCount = 0;
			// TODO: probably more stuff should be reset here
		} break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: {
			WORD vkCode = LOWORD(wParam);
			WORD keyFlags = HIWORD(lParam);
			WORD scanCode = LOBYTE(keyFlags);
			BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix
			if (isExtendedKey)
				scanCode = MAKEWORD(scanCode, 0xE0);

			OSEvent event;
			event.type = Event_KeyboardPress;
			event.time = GetMessageTime();
			event.keyboard.vkCode = vkCode;
			event.keyboard.scanCode = scanCode;
			event.keyboard.ctrlIsDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
			event.keyboard.shiftIsDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
			Win32EnqueueEvent(event);
			return TRUE;
		} break;
		case WM_KEYUP: {
			WORD vkCode = LOWORD(wParam);
			WORD keyFlags = HIWORD(lParam);
			WORD scanCode = LOBYTE(keyFlags);
			BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix
			if (isExtendedKey)
				scanCode = MAKEWORD(scanCode, 0xE0);

			OSEvent event;
			event.type = Event_KeyboardKeyUp;
			event.time = GetMessageTime();
			event.keyboard.vkCode = vkCode;
			event.keyboard.scanCode = scanCode;
			event.keyboard.ctrlIsDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
			event.keyboard.shiftIsDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
			Win32EnqueueEvent(event);
		} break;
		case WM_CHAR: {
			// NOTE: yeah... only ASCII plz...
			if (wParam == 13) wParam = 10;
			if (32 <= wParam && wParam <= 127 || 9 <= wParam && wParam <= 10) {
				OSEvent event;
				event.type = Event_KeyboardChar;
				event.time = GetMessageTime();
				event.keyboard.character = (byte)wParam;
				event.keyboard.ctrlIsDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				event.keyboard.shiftIsDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
				Win32EnqueueEvent(event);
			}
		} break;

		case WM_MOUSEMOVE: {
			if (window.iconSet) SetCursor(window.cursors[window.icon]);
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			WORD flags = LOWORD(wParam);

			OSEvent event;
			event.type = Event_MouseMove;
			event.time = GetMessageTime();
			event.mouse.cursorPos = {x, y};
			event.mouse.ctrlIsDown = (flags & MK_CONTROL) == MK_CONTROL;
			Win32EnqueueEvent(event);
		} break;
		case WM_LBUTTONDOWN : {
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			LONG time = GetMessageTime();
			WORD flags = LOWORD(wParam);

			OSEvent event;
			event.type = Event_MouseLeftButtonDown;
			event.time = time;
			event.mouse.cursorPos = {x, y};
			event.mouse.ctrlIsDown = (flags & MK_CONTROL) == MK_CONTROL;
			Win32EnqueueEvent(event);
			
			if (!PtInRect(&window.clickRect, {x, y}) || (UINT)(time - window.timeLastClicked) > GetDoubleClickTime()) {
				window.clickCount = 0;
			}
			window.clickCount++;
			window.timeLastClicked = time;
			SetRect(&window.clickRect, x, y, x, y);
			InflateRect(&window.clickRect, GetSystemMetrics(SM_CXDOUBLECLK) / 2, GetSystemMetrics(SM_CYDOUBLECLK) / 2);

			if (window.clickCount == 2) {
				event.type = Event_MouseDoubleClick;
				Win32EnqueueEvent(event);
			}

			if (window.clickCount == 3) {
				event.type = Event_MouseTripleClick;
				Win32EnqueueEvent(event);
			}
			
		} break;
		case WM_LBUTTONUP : {
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			LONG time = GetMessageTime();

			OSEvent event;
			event.type = Event_MouseLeftButtonUp;
			event.time = time;
			event.mouse.cursorPos = {x, y};
			Win32EnqueueEvent(event);
		} break;
		case WM_RBUTTONDOWN : {
			window.clickCount = 0;

			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			LONG time = GetMessageTime();

			OSEvent event;
			event.type = Event_MouseRightButtonDown;
			event.time = time;
			event.mouse.cursorPos = {x, y};
			Win32EnqueueEvent(event);
		} break;
		case WM_RBUTTONUP : {
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			LONG time = GetMessageTime();

			OSEvent event;
			event.type = Event_MouseRightButtonUp;
			event.time = time;
			event.mouse.cursorPos = {x, y};
			Win32EnqueueEvent(event);
		} break;
		case WM_MOUSEWHEEL: {
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			POINT p = {x, y};
			ScreenToClient(handle, &p);
			WORD flags = LOWORD(wParam);

			OSEvent event;
			event.type = Event_MouseVerticalWheel;
			event.time = GetMessageTime();
			event.mouse.cursorPos = {p.x, p.y};
			event.mouse.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			event.mouse.ctrlIsDown = (flags & MK_CONTROL) == MK_CONTROL;
			Win32EnqueueEvent(event);
		} break;
		case WM_MOUSEHWHEEL: {
			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			POINT p = {x, y};
			ScreenToClient(handle, &p);
			WORD flags = LOWORD(wParam);

			OSEvent event;
			event.type = Event_MouseHorizontalWheel;
			event.time = GetMessageTime();
			event.mouse.cursorPos = {p.x, p.y};
			event.mouse.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			event.mouse.ctrlIsDown = (flags & MK_CONTROL) == MK_CONTROL;
			Win32EnqueueEvent(event);
		} break;
	}

	return DefWindowProcW(handle, message, wParam, lParam);
}

String LoadAsset(int i) {
	HRSRC res = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(i), RT_RCDATA);
	HGLOBAL handle = LoadResource(NULL, res);
	LPVOID data = LockResource(handle);
	DWORD size = SizeofResource(NULL, res);
	return {(byte*)data, (ssize)size};
}

String LoadAsset(LPCTSTR name) {
	HRSRC res = FindResource(NULL, name, RT_RCDATA);
	HGLOBAL handle = LoadResource(NULL, res);
	LPVOID data = LockResource(handle);
	DWORD size = SizeofResource(NULL, res);
	return {(byte*)data, (ssize)size};
}

// NOTE: use the other one for cross-platfromness
void Win32SetWindowIcon(int i) {
	HRSRC res = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(i), RT_ICON);
	HGLOBAL handle = LoadResource(NULL, res);
	LPVOID data = LockResource(handle);
	DWORD size = SizeofResource(NULL, res);
	HICON icon = CreateIconFromResource((PBYTE)data, size, TRUE, 0x00030000);
	SendMessage(window.handle, WM_SETICON, ICON_BIG, (LPARAM)icon);
	SendMessage(window.handle, WM_SETICON, ICON_SMALL, (LPARAM)icon);
}

void Win32SetWindowIcon(Image image) {
	HDC dc = GetDC(NULL);
	BITMAPV5HEADER bi = {};
	bi.bV5Size        = sizeof(bi);
	bi.bV5Width       = image.width;
	bi.bV5Height      = image.height;
	bi.bV5Planes      = 1;
	bi.bV5BitCount    = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask     = 0x00ff0000;
	bi.bV5GreenMask   = 0x0000ff00;
	bi.bV5BlueMask    = 0x000000ff;
	bi.bV5AlphaMask   = 0xff000000;
	byte* target = NULL;
	HBITMAP color = CreateDIBSection(dc, 
		(BITMAPINFO*)&bi, 
		DIB_RGB_COLORS, 
		(VOID**)&target, 
		NULL, 0);
	ReleaseDC(NULL, dc);
	if (!color) {
		LOG("Failed to create bitmap");
		return;
	}

	HBITMAP mask = CreateBitmap(image.width, image.height, 1, 1, NULL);
	if (!mask) {
		LOG("Failed to create mask bitmap");
		DeleteObject(color);
		return;
	}

	byte* source = image.data;
	for (int32 i = 0;  i < image.width*image.height; i++) {
		target[0] = source[2];
		target[1] = source[1];
		target[2] = source[0];
		target[3] = source[3];
		target += 4;
		source += 4;
	}

	ICONINFO ii = {};
	ii.fIcon    = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmMask  = mask;
	ii.hbmColor = color;
	HICON icon = CreateIconIndirect(&ii);

	DeleteObject(color);
	DeleteObject(mask);

	if (!icon) {
		LOG("Failed to create icon");
		return;
	}

	SendMessage(window.handle, WM_SETICON, ICON_BIG, (LPARAM)icon);
	SendMessage(window.handle, WM_SETICON, ICON_SMALL, (LPARAM)icon);
}

BOOL Win32SetWindowTitle(LPCWSTR title) {
	return SetWindowTextW(window.handle, title);
}

WNDCLASSEXW CreateWindowClass() {
	WNDCLASSEXW windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.lpszClassName = L"WindowClass";
	ATOM atom = RegisterClassExW(&windowClass);
	ASSERT(atom);
	(void)atom;

	return windowClass;
}

BOOL Win32WindowDestroyed() {
	return window.destroyed;
}

void LoadCursors() {
	window.cursors[CUR_NONE] = NULL;
	window.cursors[CUR_ARROW] = LoadCursor(NULL, IDC_ARROW);
	window.cursors[CUR_MOVE] = LoadCursor(NULL, IDC_SIZEALL);	
	window.cursors[CUR_RESIZE] = LoadCursor(NULL, IDC_SIZENWSE);
	window.cursors[CUR_HAND] = LoadCursor(NULL, IDC_HAND);
	window.cursors[CUR_MOVESIDE] = LoadCursor(NULL, IDC_SIZEWE);
	window.cursors[CUR_MOVEUPDN] = LoadCursor(NULL, IDC_SIZENS);
	window.cursors[CUR_TEXT] = LoadCursor(NULL, IDC_IBEAM);
}

void Win32CreateWindow(LPCWSTR title, LONG width, LONG height) {
	WNDCLASSEXW windowClass = CreateWindowClass();
	window = {};
	LoadCursors();

	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT rect = {0, 0, width, height};
	BOOL success = AdjustWindowRect(&rect, style, FALSE);
	if (!success)
		LOG("failed to adjust window size");

	window.dim = {width, height};
	window.handle = CreateWindowExW(
		WS_EX_APPWINDOW,
		windowClass.lpszClassName, 
		title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, windowClass.hInstance, 0);

	ASSERT(window.handle);
}

void Win32CreateWindowFullScreen(LPCWSTR title) {
	WNDCLASSEXW windowClass = CreateWindowClass();
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
	window.handle = CreateWindowExW(0,
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
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

void Win32SetCursorIcon(int32 icon) {
	SetCursor(window.cursors[icon]);
}

void Win32SetWindowCursorIcon(int32 icon) {
	window.iconSet = TRUE;
	window.icon = icon;
}

HANDLE Win32OpenFileDialog(WCHAR* path, DWORD maxPathLength) {
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = maxPathLength;
	dialog.hwndOwner = window.handle;
	dialog.Flags = OFN_FILEMUSTEXIST;
	dialog.Flags |= OFN_NOCHANGEDIR;
	BOOL success = GetOpenFileNameW(&dialog);
	if (!success) {
		LOG("failed to get open file name");
		return INVALID_HANDLE_VALUE ;
	}
	return Win32OpenFile(path);
}

HANDLE Win32OpenFileDialog() {
	WCHAR path[MAX_PATH];
	return Win32OpenFileDialog(path, MAX_PATH);
}

HANDLE Win32SaveFileDialog(WCHAR* path, DWORD maxPathLength) {
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = maxPathLength;
	dialog.hwndOwner = window.handle;
	dialog.Flags = OFN_OVERWRITEPROMPT;
	dialog.Flags |= OFN_NOCHANGEDIR;
	BOOL success = GetSaveFileNameW(&dialog);
	if (!success) {
		LOG("failed to get save file name");
		return INVALID_HANDLE_VALUE ;
	}
	return Win32CreateFile(path);
}

HANDLE Win32SaveFileDialog() {
	WCHAR path[MAX_PATH];
	return Win32SaveFileDialog(path, MAX_PATH);
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

BOOL Win32RequestClipboardData(void* userdata, void (*callback)(void*, String)) {
	if (!OpenClipboard(window.handle)) {
		LOG("failed to open clipboard");
		return FALSE;
	}

	HANDLE hMem = GetClipboardData(CF_TEXT);
	if (!hMem) {
		LOG("failed to get clipboard data");
		return FALSE;
	}

	byte* data = (byte*)GlobalLock(hMem);
	ssize length = 0;
	while (data[length]) length++;

	if (!GlobalUnlock(hMem)) {
		LOG("failed to unlock memory");
	}

	CloseClipboard();
	callback(userdata, {data, length});
	return TRUE;
}