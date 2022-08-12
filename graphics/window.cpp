#define MOUSE_L 					0
#define MOUSE_M 					1
#define MOUSE_R 					2

#if defined(_Win32)
#  include "win32window.cpp"
#  define CursorIcon				LPCSTR
#  define CUR_MOVE					IDC_SIZEALL	 					
#  define CUR_ARROW					IDC_ARROW
#  define CUR_RESIZE				IDC_SIZENWSE
#  define CUR_HAND					IDC_HAND
#  define CUR_MOVESIDE				IDC_SIZEWE

#  define KEY_ESC 					VK_ESCAPE		// 0x1B
#  define KEY_SPACE					VK_SPACE		// 0x20
#  define KEY_PGUP					VK_PRIOR		// 0x21
#  define KEY_PGDN					VK_NEXT			// 0x22
#  define KEY_END 					VK_END			// 0x23
#  define KEY_HOME					VK_HOME			// 0x24
#  define KEY_LEFT					VK_LEFT			// 0x25
#  define KEY_UP					VK_UP			// 0x26
#  define KEY_RIGHT					VK_RIGHT		// 0x27
#  define KEY_DOWN					VK_DOWN			// 0x28

#  define OsGetWindowDimensions		Win32GetWindowDimensions
#  define OsGetWindowHandle			Win32GetWindowHandle
#  define OsCreateWindow			Win32CreateWindow
#  define OsCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OsHandleWindowEvents		Win32HandleWindowEvents
#  define OsSetCursorIcon			Win32SetCursorIcon
#  define OsExitFullScreen			Win32ExitFullScreen
#  define OsEnterFullScreen			Win32EnterFullScreen
#  define MAX_PATH_SIZE				MAX_PATH
#  define OsOpenFileDialog			Win32OpenFileDialog
#  define OsSaveFileDialog			Win32SaveFileDialog
#  define OsGetCursorPosition 		Win32GetCursorPosition
#  define IsKeyDown					Win32IsKeyDown
#  define IsKeyPressed				Win32IsKeyPressed
#  define IsMouseDown 				Win32IsMouseDown
#  define OsWindowDestroyed			Win32WindowDestroyed
#endif

#if defined(__gnu_linux__)
#  define Font	X11Font
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  undef Font
#  include "linuxwindow.cpp"
#  define OsGetWindowDimensions		LinuxGetWindowDimensions
#  define OsCreateWindow			LinuxCreateWindow
#  define OsCreateWindowFullScreen	LinuxCreateWindowFullScreen
#  define OsHandleWindowEvents		LinuxHandleWindowEvents
#  define OsWindowDestroyed			LinuxWindowDestroyed
#  define OsGetWindowHandle			LinuxGetWindowHandle
#  define OsSetCursorIcon			LinuxSetCursorIcon

#  define CursorIcon				unsigned int
#  define CUR_MOVE					XC_fleur				
#  define CUR_ARROW					XC_left_ptr
#  define CUR_RESIZE				XC_bottom_left_corner
#  define CUR_HAND					XC_hand1
#  define CUR_MOVESIDE				XC_sb_h_double_arrow

#  define KEY_ESC 					(XK_Escape       && 0xff)
#  define KEY_SPACE					(XK_KP_Space     && 0xff)
#  define KEY_PGUP					(XK_KP_Page_Up   && 0xff)
#  define KEY_PGDN					(XK_KP_Page_Down && 0xff)
#  define KEY_END 					(XK_KP_End       && 0xff)
#  define KEY_HOME					(XK_KP_Home      && 0xff)
#  define KEY_LEFT					(XK_KP_Left      && 0xff)
#  define KEY_UP					(XK_KP_Up	     && 0xff)
#  define KEY_RIGHT					(XK_KP_Right     && 0xff)
#  define KEY_DOWN					(XK_KP_Down      && 0xff)

#  define IsMouseDown 				LinuxIsMouseDown
#  define IsKeyDown					LinuxIsKeyDown
#  define IsKeyPressed				LinuxIsKeyPressed
#endif