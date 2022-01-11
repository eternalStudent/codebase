#include "win32input.cpp"
#define OsGetCursorPosition 	Win32GetCursorPosition

#define Event					UINT
#define MOUSE_LDN				WM_LBUTTONDOWN	// 0x201
#define MOUSE_LUP				WM_LBUTTONUP	// 0x202
#define MOUSE_RDN				WM_RBUTTONDOWN	// 0x204
#define MOUSE_RUP				WM_RBUTTONUP	// 0x205
#define KEY_ESC 				VK_ESCAPE		// 0x018
#define KEY_SPACE				VK_SPACE		// 0x020
#define KEY_LEFT				VK_LEFT			// 0x025
#define KEY_UP					VK_UP			// 0x026
#define KEY_RIGHT				VK_RIGHT		// 0x027
#define KEY_DOWN				VK_DOWN			// 0x028