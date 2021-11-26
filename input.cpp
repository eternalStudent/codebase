#include "win32input.cpp"
#define OsGetCursorPosition     Win32GetCursorPosition

#define Event					UINT
#define MOUSE_LDN				WM_LBUTTONDOWN
#define MOUSE_LUP				WM_LBUTTONUP
#define MOUSE_RDN				WM_RBUTTONDOWN
#define MOUSE_RUP				WM_RBUTTONUP
#define KEY_ESC					VK_ESCAPE