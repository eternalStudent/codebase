#define CUR_ARROW 					0
#define CUR_MOVE					1	 					
#define CUR_RESIZE					2
#define CUR_HAND					3
#define CUR_MOVESIDE				4
#define CUR_MOVEUPDN				5
#define CUR_TEXT					6

#if defined(_WIN32)
#  include <Windowsx.h>
#  include "win32window.cpp"
#  define KEY_BACKSPACE 			VK_BACK			// 0x08
#  define KEY_TAB					VK_TAB			// 0x09
#  define KEY_SHIFT 				VK_SHIFT		// 0x10
#  define KEY_CTRL					VK_CONTROL		// 0x11
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
#  define KEY_DELETE				VK_DELETE		// 0x2E
#  define KEY_A 					0x41
#  define KEY_C 					0x43
#  define KEY_V 					0x56
#  define KEY_X 					0x58

#  define OSCreateWindow			Win32CreateWindow
#  define OSProcessWindowEvents 	Win32ProcessWindowEvents

#  define OSCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OSExitFullScreen			Win32ExitFullScreen
#  define OSEnterFullScreen 		Win32EnterFullScreen

#  define OSGetWindowDimensions 	Win32GetWindowDimensions
#  define OSWindowDestroyed 		Win32WindowDestroyed
#  define OSSetCursorIcon			Win32SetCursorIcon
#  define OSGetCursorPosition 		Win32GetCursorPosition

#  define OSOpenFileDialog			Win32OpenFileDialog
#  define OSSaveFileDialog			Win32SaveFileDialog

#  define OSIsKeyDown 				Win32IsKeyDown
#  define OSIsKeyPressed			Win32IsKeyPressed
#  define OSIsMouseLeftButtonDown 	Win32IsMouseLeftButtonDown
#  define OSIsMouseLeftClicked		Win32IsMouseLeftClicked
#  define OSIsMouseRightClicked		Win32IsMouseRightClicked
#  define OSIsMouseDoubleClicked	Win32IsMouseDoubleClicked
#  define OSIsMouseTripleClicked	Win32IsMouseTripleClicked
#  define OSGetMouseWheelDelta		Win32GetMouseWheelDelta
#  define OSGetMouseHWheelDelta 	Win32GetMouseHWheelDelta
#  define OSGetTypedText			Win32GetTypedText

#  define OSCopyToClipboard 		Win32CopyToClipboard
#  define OSPasteFromClipboard		Win32PasteFromClipboard
#endif

#if defined(__gnu_linux__)
#  define Font	X11Font
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  undef Font
#  include "linuxwindow.cpp"
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

#  define OSCreateWindow			LinuxCreateWindow
#  define OSProcessWindowEvents 	LinuxProcessWindowEvents

#  define OSCreateWindowFullScreen	LinuxCreateWindowFullScreen
#  define OSExitFullScreen			LinuxExitFullScreen
#  define OSEnterFullScreen 		LinuxEnterFullScreen

#  define OSGetWindowDimensions 	LinuxGetWindowDimensions
#  define OSWindowDestroyed 		LinuxWindowDestroyed
#  define OSSetCursorIcon			LinuxSetCursorIcon
#  define OSGetCursorPosition 		LinuxGetCursorPosition

#  define OSOpenFileDialog			LinuxOpenFileDialog
#  define OSSaveFileDialog			LinuxSaveFileDialog

#  define OSIsKeyDown 				LinuxIsKeyDown
#  define OSIsKeyPressed			LinuxIsKeyPressed
#  define OSIsMouseLeftButtonDown 	LinuxIsMouseLeftButtonDown
#  define OSIsMouseLeftClicked		LinuxIsMouseLeftClicked
#  define OSIsMouseRightClicked		LinuxIsMouseRightClicked
#  define OSIsMouseDoubleClicked	LinuxIsMouseDoubleClicked
#  define OSIsMouseTripleClicked	LinuxIsMouseTripleClicked
#  define OSGetMouseWheelDelta		LinuxGetMouseWheelDelta
#  define OSGetMouseHWheelDelta 	LinuxGetMouseHWheelDelta
#  define OSGetTypedText			LinuxGetTypedText

#  define OSCopyToClipboard 		LinuxCopyToClipboard
#  define OSPasteFromClipboard		LinuxPasteFromClipboard
#endif