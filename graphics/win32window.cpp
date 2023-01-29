#if (!defined(WM_MOUSEHWHEEL))
#define WM_MOUSEHWHEEL 0x020e
#endif

struct {
	HWND handle;

	Dimensions2i dim;
	BOOL destroyed;

	BYTE keyIsDown[256];
	BYTE keyWasDown[256];
	CHAR typed[4];
	int32 strlength;

	BYTE mouseLeftButtonIsDown;
	BYTE mouseRightButtonIsDown;
	BYTE mouseLeftButtonWasDown;
	BYTE mouseRightButtonWasDown;
	int32 clickCount;
	int32 prevClickCount;
	RECT clickRect;
	DWORD timeLastClicked;
	DWORD mouseWheelDelta;
	DWORD mouseHWheelDelta;

	HCURSOR cursors[7];
} window;

HWND Win32GetWindowHandle() {
	return window.handle;
}

Dimensions2i Win32GetWindowDimensions() {
	return window.dim;
}

BOOL Win32IsKeyDown(DWORD key) {
	return window.keyIsDown[key] == 1;
}

BOOL Win32IsKeyPressed(DWORD key) {
	return window.keyIsDown[key] == 1 && window.keyWasDown[key] == 0;
}

BOOL Win32IsMouseLeftButtonDown() {
	return window.mouseLeftButtonIsDown == 1 && window.clickCount == 1;
}

BOOL Win32IsMouseLeftButtonUp() {
	return window.mouseLeftButtonIsDown == 0;
}

BOOL Win32IsMouseLeftReleased() {
	return window.mouseLeftButtonIsDown == 0 && window.mouseLeftButtonWasDown == 1;
}

BOOL Win32IsMouseLeftClicked() {
	return window.clickCount == 1 && window.mouseLeftButtonIsDown == 1 && window.mouseLeftButtonWasDown == 0;
}

BOOL Win32IsMouseRightClicked() {
	return window.mouseRightButtonIsDown == 1 && window.mouseRightButtonWasDown == 0;
}

BOOL Win32IsMouseDoubleClicked() {
	return window.clickCount == 2 && window.prevClickCount == 1;
}

BOOL Win32IsMouseTripleClicked() {
	return window.clickCount == 3 && window.prevClickCount == 2;
}

DWORD Win32GetMouseWheelDelta() {
	return window.mouseWheelDelta;
}

DWORD Win32GetMouseHWheelDelta() {
	return window.mouseHWheelDelta;
}

void Win32ResetMouse() {
	window.mouseLeftButtonIsDown = 0;
}

String Win32GetTypedText() {
	return {(byte*)window.typed, (ssize)window.strlength};
}

void Win32ResetTypedText() {
	window.strlength = 0;
}

void Win32ExitFullScreen() {
	DWORD dwStyle = GetWindowLong(window.handle, GWL_STYLE);
	SetWindowLong(window.handle, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
	SetWindowPos(window.handle, NULL, 0, 0, 920, 540, SWP_FRAMECHANGED);
	memset(window.keyIsDown, 0, 256);
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
			window.keyIsDown[vkcode] = 1;
		} break;
		case WM_KEYUP: {
			UINT vkcode = (UINT)wParam;
			window.keyIsDown[vkcode] = 0;
		} break;
		case WM_LBUTTONDOWN : {
			window.mouseLeftButtonIsDown = 1;

			LONG x = GET_X_LPARAM(lParam);
			LONG y = GET_Y_LPARAM(lParam);
			DWORD time = GetMessageTime();

			if (!PtInRect(&window.clickRect, {x, y}) || time - window.timeLastClicked > GetDoubleClickTime()) {
				window.clickCount = 0;
			}
			window.clickCount++;

			window.timeLastClicked = time;
			SetRect(&window.clickRect, x, y, x, y);
			InflateRect(&window.clickRect, GetSystemMetrics(SM_CXDOUBLECLK) / 2, GetSystemMetrics(SM_CYDOUBLECLK) / 2);
		} break;
		case WM_LBUTTONUP : {
			window.mouseLeftButtonIsDown = 0;
		} break;
		case WM_RBUTTONDOWN : {
			window.mouseRightButtonIsDown = 1;
			window.clickCount = 0;
		} break;
		case WM_RBUTTONUP : {
			window.mouseRightButtonIsDown = 0;
		} break;
		case WM_MOUSEWHEEL: {
			window.mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		} break;
		case WM_MOUSEHWHEEL: {
			window.mouseHWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		} break;
		case WM_ACTIVATE: {
			window.clickCount = 0;
			// TODO: probably more stuff should be reset here
		}
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

WNDCLASSA CreateWindowClass() {
	WNDCLASSA windowClass = {};
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
	for (int32 i = 0; i < 256; i++) window.keyWasDown[i] = window.keyIsDown[i];
	window.mouseLeftButtonWasDown = window.mouseLeftButtonIsDown;
	window.mouseRightButtonWasDown = window.mouseRightButtonIsDown;
	window.prevClickCount = window.clickCount;
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
	WCHAR path[MAX_PATH];
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.hwndOwner = window.handle;
	dialog.Flags = OFN_FILEMUSTEXIST;
	dialog.Flags |= OFN_NOCHANGEDIR;
	BOOL success = GetOpenFileNameW(&dialog);
	if (!success) {
		LOG("failed to get save file name");
		return INVALID_HANDLE_VALUE ;
	}
	return Win32OpenFile(path);
}

HANDLE Win32SaveFileDialog() {
	WCHAR path[MAX_PATH];
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAME);
	path[0] = 0;
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
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

BOOL Win32RequestClipboardData(void (*callback)(String)) {
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
	callback({data, length});
	return TRUE;
}