#include "win32window.cpp"
#define Window                  	HWND
#define CursorIcon 					LPCSTR
#define CUR_MOVE				    IDC_SIZEALL	 					
#define CUR_ARROW 					IDC_ARROW
#define CUR_RESIZE 					IDC_SIZENWSE
#define CUR_HAND					IDC_HAND
#define CUR_MOVESIDE				IDC_SIZEWE

#define OsGetWindowDimensions   	Win32GetWindowDimensions
#define OsCreateWindow				Win32CreateWindow
#define OsCreateWindowFullScreen	Win32CreateWindowFullScreen
#define OsHandleWindowEvents		Win32HandleWindowEvents
#define OsSetCursorIcon				Win32SetCursorIcon
#define OsExitFullScreen			Win32ExitFullScreen
#define OsEnterFullScreen			Win32EnterFullScreen
#define MAX_PATH_SIZE				MAX_PATH
#define OsOpenFileDialog			Win32OpenFileDialog
#define OsSaveFileDialog			Win32SaveFileDialog