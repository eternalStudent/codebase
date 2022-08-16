#define MOUSE_L 					0
#define MOUSE_M 					1
#define MOUSE_R 					2

#if defined(_WIN32)
#  include "win32window.cpp"
#  define CursorIcon				LPCSTR
#  define CUR_MOVE					IDC_SIZEALL	 					
#  define CUR_ARROW 				IDC_ARROW
#  define CUR_RESIZE				IDC_SIZENWSE
#  define CUR_HAND					IDC_HAND
#  define CUR_MOVESIDE				IDC_SIZEWE

#  define KEY_ESC 					VK_ESCAPE		// 0x1B
#  define KEY_SPACE 				VK_SPACE		// 0x20
#  define KEY_PGUP					VK_PRIOR		// 0x21
#  define KEY_PGDN					VK_NEXT			// 0x22
#  define KEY_END 					VK_END			// 0x23
#  define KEY_HOME					VK_HOME			// 0x24
#  define KEY_LEFT					VK_LEFT			// 0x25
#  define KEY_UP					VK_UP			// 0x26
#  define KEY_RIGHT 				VK_RIGHT		// 0x27
#  define KEY_DOWN					VK_DOWN			// 0x28

#  define OSGetWindowDimensions 	Win32GetWindowDimensions
#  define OSGetWindowHandle 		Win32GetWindowHandle
#  define OSCreateWindow			Win32CreateWindow
#  define OSCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OSProcessWindowEvents 	Win32ProcessWindowEvents
#  define OSSetCursorIcon			Win32SetCursorIcon
#  define OSExitFullScreen			Win32ExitFullScreen
#  define OSEnterFullScreen 		Win32EnterFullScreen
#  define OSOpenFileDialog			Win32OpenFileDialog
#  define OSSaveFileDialog			Win32SaveFileDialog
#  define OSGetCursorPosition 		Win32GetCursorPosition
#  define OSIsKeyDown 				Win32IsKeyDown
#  define OSIsKeyPressed			Win32IsKeyPressed
#  define OSIsMouseDown 			Win32IsMouseDown
#  define OSIsMousePressed			Win32IsMousePressed
#  define OSGetMouseWheelDelta		Win32GetMouseWheelDelta
#  define OSWindowDestroyed 		Win32WindowDestroyed
#endif

#if defined(__gnu_linux__)
#  define Font	X11Font
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  undef Font
#  include "linuxwindow.cpp"
#  define OSGetWindowDimensions		LinuxGetWindowDimensions
#  define OSCreateWindow			LinuxCreateWindow
#  define OSCreateWindowFullScreen	LinuxCreateWindowFullScreen
#  define OSProcessWindowEvents		LinuxProcessWindowEvents
#  define OSWindowDestroyed			LinuxWindowDestroyed
#  define OSGetWindowHandle			LinuxGetWindowHandle
#  define OSSetCursorIcon			LinuxSetCursorIcon
#  define OSGetCursorPosition 		LinuxGetCursorPosition

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

#  define OSIsMouseDown 			LinuxIsMouseDown
#  define OSIsKeyDown				LinuxIsKeyDown
#  define OSIsKeyPressed			LinuxIsKeyPressed

#  define OSOpenFileDialog			LinuxOpenFileDialog
#  define OSSaveFileDialog			LinuxSaveFileDialog
#endif