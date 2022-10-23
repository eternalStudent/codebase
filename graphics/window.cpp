#define CUR_ARROW 				0
#define CUR_MOVE				1	 					
#define CUR_RESIZE				2
#define CUR_HAND				3
#define CUR_MOVESIDE			4
#define CUR_MOVEUPDN			5
#define CUR_TEXT				6

#define KEY_BACKSPACE 			0x08
#define KEY_TAB 				0x09
#define KEY_ENTER				0x0D
#define KEY_SHIFT 				0x10
#define KEY_CTRL				0x11
#define KEY_ESC 				0x1B
#define KEY_SPACE 				0x20
#define KEY_PGUP				0x21
#define KEY_PGDN				0x22
#define KEY_END 				0x23
#define KEY_HOME				0x24
#define KEY_LEFT				0x25
#define KEY_UP					0x26
#define KEY_RIGHT 				0x27
#define KEY_DOWN				0x28
#define KEY_DELETE				0x2E
#define KEY_A 					0x41
#define KEY_C 					0x43
#define KEY_N 					0x4E
#define KEY_O 					0x4F
#define KEY_S 					0x53
#define KEY_V 					0x56
#define KEY_X 					0x58

#if defined(_WIN32)
#  include <Windowsx.h>
#  include "win32window.cpp"


#  define OSCreateWindow			Win32CreateWindow
#  define OSProcessWindowEvents 	Win32ProcessWindowEvents

#  define OSCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OSExitFullScreen			Win32ExitFullScreen
#  define OSEnterFullScreen 		Win32EnterFullScreen
#  define OSSetWindowIcon			Win32SetWindowIcon

#  define OSGetWindowDimensions 	Win32GetWindowDimensions
#  define OSWindowDestroyed 		Win32WindowDestroyed
#  define OSSetCursorIcon			Win32SetCursorIcon
#  define OSGetCursorPosition 		Win32GetCursorPosition

#  define OSOpenFileDialog			Win32OpenFileDialog
#  define OSSaveFileDialog			Win32SaveFileDialog

#  define OSIsKeyDown 				Win32IsKeyDown
#  define OSIsKeyPressed			Win32IsKeyPressed
#  define OSGetTypedText			Win32GetTypedText
#  define OSResetTypedText			Win32ResetTypedText
#  define OSIsMouseLeftButtonDown 	Win32IsMouseLeftButtonDown
#  define OSIsMouseLeftButtonUp 	Win32IsMouseLeftButtonUp
#  define OSIsMouseLeftClicked		Win32IsMouseLeftClicked
#  define OSIsMouseLeftReleased 	Win32IsMouseLeftReleased
#  define OSIsMouseRightClicked 	Win32IsMouseRightClicked
#  define OSIsMouseDoubleClicked	Win32IsMouseDoubleClicked
#  define OSIsMouseTripleClicked	Win32IsMouseTripleClicked
#  define OSGetMouseWheelDelta		Win32GetMouseWheelDelta
#  define OSGetMouseHWheelDelta 	Win32GetMouseHWheelDelta
#  define OSResetMouse				Win32ResetMouse

#  define OSCopyToClipboard 		Win32CopyToClipboard
#  define OSRequestClipboardData	Win32RequestClipboardData
#endif

#if defined(__gnu_linux__)
#  define Font	X11Font
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  include <X11/XKBlib.h>
#  undef Font
#  define Button6            6
#  define Button7            7
#  include "linuxwindow.cpp"

#  define OSCreateWindow			LinuxCreateWindow
#  define OSProcessWindowEvents 	LinuxProcessWindowEvents

#  define OSCreateWindowFullScreen	LinuxCreateWindowFullScreen
#  define OSExitFullScreen			LinuxExitFullScreen
#  define OSEnterFullScreen 		LinuxEnterFullScreen
#  define OSSetWindowIcon			LinuxSetWindowIcon

#  define OSGetWindowDimensions 	LinuxGetWindowDimensions
#  define OSWindowDestroyed 		LinuxWindowDestroyed
#  define OSSetCursorIcon			LinuxSetCursorIcon
#  define OSGetCursorPosition 		LinuxGetCursorPosition

#  define OSOpenFileDialog			LinuxOpenFileDialog
#  define OSSaveFileDialog			LinuxSaveFileDialog

#  define OSIsKeyDown 				LinuxIsKeyDown
#  define OSIsKeyPressed			LinuxIsKeyPressed
#  define OSGetTypedText			LinuxGetTypedText
#  define OSResetTypedText			LinuxResetTypedText
#  define OSIsMouseLeftButtonDown 	LinuxIsMouseLeftButtonDown
#  define OSIsMouseLeftButtonUp 	LinuxIsMouseLeftButtonUp
#  define OSIsMouseLeftClicked		LinuxIsMouseLeftClicked
#  define OSIsMouseLeftReleased 	LinuxIsMouseLeftReleased
#  define OSIsMouseRightClicked 	LinuxIsMouseRightClicked
#  define OSIsMouseDoubleClicked	LinuxIsMouseDoubleClicked
#  define OSIsMouseTripleClicked	LinuxIsMouseTripleClicked
#  define OSGetMouseWheelDelta		LinuxGetMouseWheelDelta
#  define OSGetMouseHWheelDelta 	LinuxGetMouseHWheelDelta
#  define OSResetMouse				LinuxResetMouse

#  define OSCopyToClipboard 		LinuxCopyToClipboard
#  define OSRequestClipboardData	LinuxRequestClipboardData
#endif