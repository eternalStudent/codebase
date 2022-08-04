#include "win32window.cpp"
#define CursorIcon					LPCSTR
#define CUR_MOVE					IDC_SIZEALL	 					
#define CUR_ARROW					IDC_ARROW
#define CUR_RESIZE					IDC_SIZENWSE
#define CUR_HAND					IDC_HAND
#define CUR_MOVESIDE				IDC_SIZEWE

#define MOUSE_L 					0
#define MOUSE_M 					1
#define MOUSE_R 					2
#define KEY_ESC 					VK_ESCAPE		// 0x1B
#define KEY_SPACE					VK_SPACE		// 0x20
#define KEY_PGUP					VK_PRIOR		// 0x21
#define KEY_PGDN					VK_NEXT			// 0x22
#define KEY_END 					VK_END			// 0x23
#define KEY_HOME					VK_HOME			// 0x24
#define KEY_LEFT					VK_LEFT			// 0x25
#define KEY_UP						VK_UP			// 0x26
#define KEY_RIGHT					VK_RIGHT		// 0x27
#define KEY_DOWN					VK_DOWN			// 0x28

#define OsGetWindowDimensions		Win32GetWindowDimensions
#define OsGetWindowHandle			Win32GetWindowHandle
#define OsCreateWindow				Win32CreateWindow
#define OsCreateWindowFullScreen	Win32CreateWindowFullScreen
#define OsHandleWindowEvents		Win32HandleWindowEvents
#define OsSetCursorIcon				Win32SetCursorIcon
#define OsExitFullScreen			Win32ExitFullScreen
#define OsEnterFullScreen			Win32EnterFullScreen
#define MAX_PATH_SIZE				MAX_PATH
#define OsOpenFileDialog			Win32OpenFileDialog
#define OsSaveFileDialog			Win32SaveFileDialog
#define OsGetCursorPosition 		Win32GetCursorPosition
#define IsKeyDown					Win32IsKeyDown
#define IsKeyPressed				Win32IsKeyPressed
#define IsMouseDown 				Win32IsMouseDown
#define OsWindowDestroyed			Win32WindowDestroyed