#include "geometry.cpp"
#include "image.cpp"

enum : uint32 {
	Event_none,

	Event_WindowResize,

	Event_MouseMove,
	Event_MouseLeftButtonDown,
	Event_MouseLeftButtonUp,
	Event_MouseRightButtonDown,
	Event_MouseRightButtonUp,
	Event_MouseDoubleClick,
	Event_MouseTripleClick,
	Event_MouseVerticalWheel,
	Event_MouseHorizontalWheel,

	Event_KeyboardPress,
	Event_KeyboardChar,

	Event_count
};

struct WindowEvent {
	Dimensions2i dim;
	Dimensions2i prevDim;
};

struct MouseEvent {
	Point2i cursorPos;
	int32 wheelDelta;
	bool ctrlIsDown;
};

struct KeyboardEvent {
	uint32 vkCode;
	uint32 scanCode;
	byte character;
	bool ctrlIsDown;
	bool shiftIsDown;
};

struct OSEvent {
	uint32 type; 
	int32 time;
	union {
		WindowEvent window;
		MouseEvent mouse;
		KeyboardEvent keyboard;
	};
};

enum : uint32 {
	CUR_NONE,
	CUR_ARROW,
	CUR_MOVE,
	CUR_RESIZE,	
	CUR_HAND,
	CUR_MOVESIDE,
	CUR_MOVEUPDN,
	CUR_TEXT,

	CUR_COUNT
};

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
#define KEY_GREATER				0x3E
#define KEY_A 					0x41
#define KEY_C 					0x43
#define KEY_D 					0x44
#define KEY_E 					0x45
#define KEY_F 					0x46
#define KEY_G 					0x47
#define KEY_I 					0x49
#define KEY_L					0x4C
#define KEY_N 					0x4E
#define KEY_O 					0x4F
#define KEY_R 					0x52
#define KEY_S 					0x53
#define KEY_T 					0x54
#define KEY_U 					0x55
#define KEY_V 					0x56
#define KEY_X 					0x58
#define KEY_Y 					0x59
#define KEY_Z 					0x5A

#if defined(_OS_WINDOWS)
#  include <Windowsx.h>
#  include "win32window.cpp"


#  define OSCreateWindow			Win32CreateWindow
#  define OSProcessWindowEvents 	Win32ProcessWindowEvents

#  define OSCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OSExitFullScreen			Win32ExitFullScreen
#  define OSEnterFullScreen 		Win32EnterFullScreen
#  define OSSetWindowIcon			Win32SetWindowIcon
#  define OSSetWindowToNoneResizable Win32SetWindowToNoneResizable
#  define OSSetWindowTitle			Win32SetWindowTitle

#  define OSGetWindowDimensions 	Win32GetWindowDimensions
#  define OSWindowDestroyed 		Win32WindowDestroyed
#  define OSSetCursorIcon			Win32SetCursorIcon
#  define OSGetCursorPosition 		Win32GetCursorPosition

#  define OSOpenFileDialog			Win32OpenFileDialog
#  define OSSaveFileDialog			Win32SaveFileDialog

#  define OSGetTypedText			Win32GetTypedText
#  define OSResetTypedText			Win32ResetTypedText
#  define OSIsMouseLeftButtonDown 	Win32IsMouseLeftButtonDown
#  define OSIsKeyDown				Win32IsKeyDown

#  define OSCopyToClipboard 		Win32CopyToClipboard
#  define OSRequestClipboardData	Win32RequestClipboardData

#  define OSPollEvent				Win32PollEvent
#endif

#if defined(_OS_LINUX)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  include <X11/XKBlib.h>
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