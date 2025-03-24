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
	Event_MouseContextMenu,
	Event_MouseNotMove,

	Event_KeyboardPress,
	Event_KeyboardChar,
	Event_KeyboardKeyUp, // TODO: rename to Release?

	Event_Custom,

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
	int32 notMoveCount;
};

struct KeyboardEvent {
	uint32 vkCode;
	uint32 scanCode;
	// TODO: UTF8
	byte character;
	bool ctrlIsDown;
	bool shiftIsDown;
};

struct CustomEvent {
	uint32 code;
	String str;
};

struct OSEvent {
	uint32 type; 
	int32 time;
	union {
		WindowEvent window;
		MouseEvent mouse;
		KeyboardEvent keyboard;
		CustomEvent custom;
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
#define KEY_ALT					0x12
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
#define KEY_B 					0x42
#define KEY_C 					0x43
#define KEY_D 					0x44
#define KEY_E 					0x45
#define KEY_F 					0x46
#define KEY_G 					0x47
#define KEY_H 					0x48
#define KEY_I 					0x49
#define KEY_J 					0x4A
#define KEY_K 					0x4B
#define KEY_L					0x4C
#define KEY_M					0x4D
#define KEY_N 					0x4E
#define KEY_O 					0x4F
#define KEY_P 					0x50
#define KEY_Q 					0x51
#define KEY_R 					0x52
#define KEY_S 					0x53
#define KEY_T 					0x54
#define KEY_U 					0x55
#define KEY_V 					0x56
#define KEY_W 					0x57
#define KEY_X 					0x58
#define KEY_Y 					0x59
#define KEY_Z 					0x5A
#define KEY_F1 					0x70
#define KEY_F2 					0x71
#define KEY_F3 					0x72
#define KEY_F4 					0x73
#define KEY_F5 					0x74
#define KEY_F6 					0x75
#define KEY_F7 					0x76
#define KEY_F8 					0x77
#define KEY_F9 					0x78
#define KEY_F10 				0x79
#define KEY_F11 				0x7A
#define KEY_F12 				0x7B

#if defined(OS_WINDOWS)
#  include <Windowsx.h>
#  include "win32window.cpp"


#  define OSCreateWindow			Win32CreateWindow
#  define OSProcessWindowEvents 	Win32ProcessWindowEvents

#  define OSCreateWindowFullScreen	Win32CreateWindowFullScreen
#  define OSCreateWindowMaximized	Win32CreateWindowMaximized
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

#  define OSEnqueueEvent			Win32EnqueueEvent
#  define OSPollEvent				Win32PollEvent
#endif

#if defined(OS_LINUX)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  include <X11/cursorfont.h>
#  include <X11/XKBlib.h>
#  define Button6            6
#  define Button7            7
#  include "linuxwindow.cpp"
#  define GFX_BACKEND_OPENGL

#  define OSCreateWindow			LinuxCreateWindow
#  define OSProcessWindowEvents 	LinuxProcessWindowEvents

#  define OSCreateWindowFullScreen	LinuxCreateWindowFullScreen
#  define OSCreateWindowMaximized	LinuxCreateWindowMaximized
#  define OSExitFullScreen			LinuxExitFullScreen
#  define OSEnterFullScreen 		LinuxEnterFullScreen
#  define OSSetWindowIcon			LinuxSetWindowIcon
#  define OSSetWindowToNoneResizable LinuxSetWindowToNoneResizable
#  define OSSetWindowTitle			LinuxSetWindowTitle

#  define OSGetWindowDimensions 	LinuxGetWindowDimensions
#  define OSWindowDestroyed 		LinuxWindowDestroyed
#  define OSSetCursorIcon			LinuxSetCursorIcon
#  define OSGetCursorPosition 		LinuxGetCursorPosition

#  define OSOpenFileDialog			LinuxOpenFileDialog
#  define OSSaveFileDialog			LinuxSaveFileDialog

#  define OSGetTypedText			LinuxGetTypedText
#  define OSResetTypedText			LinuxResetTypedText
#  define OSIsMouseLeftButtonDown 	LinuxIsMouseLeftButtonDown
#  define OSIsKeyDown				LinuxIsKeyDown

#  define OSCopyToClipboard 		LinuxCopyToClipboard
#  define OSRequestClipboardData	LinuxRequestClipboardData

#  define OSEnqueueEvent			LinuxEnqueueEvent
#  define OSPollEvent				LinuxPollEvent
#endif