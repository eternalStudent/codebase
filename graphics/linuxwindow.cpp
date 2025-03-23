#define EVENT_QUEUE_CAP			8
#define EVENT_QUEUE_MASK		(EVENT_QUEUE_CAP - 1)

struct EventQueue {
	OSEvent table[EVENT_QUEUE_CAP];
	volatile uint32 writeIndex;
	volatile uint32 readIndex;
};

struct WindowHandle {
	Window window;
	Display* display;
};

struct {
	union {
		WindowHandle handle;
		struct {Window window; Display* display;};
	};

	Atom WM_PROTOCOLS;
	Atom WM_DELETE_WINDOW;
	Atom _NET_WM_ICON;

	Window root;
	Cursor cursors[CUR_COUNT];
	XIC xic;

	int32 clickCount;
	Point2i lastClickPoint;
	Time timeLastClicked;

	Point2i cursorPos;
	Dimensions2i dim;
	Bool destroyed;

	EventQueue queue;

	// clipboard
	Atom CLIPBOARD;
	Atom TARGETS;
	Atom UTF8_STRING;
	String selection;
	Atom target;
	void (*pasteCallback)(String);
} window;

WindowHandle LinuxGetWindowHandle() {
	return window.handle;
}

Dimensions2i LinuxGetWindowDimensions() {
	/*
	XWindowAttributes attr;
	Status status = XGetWindowAttributes(window.display, window.window, &attr);
	ASSERT(status && "Failed to get window attributes");

	return {attr.width, attr.height};
	*/
	return window.dim;
}

void LinuxExitFullScreen() {
	Atom wm_state = XInternAtom(window.display, "_NET_WM_STATE", True);
	Atom wm_fullscreen = XInternAtom(window.display, "_NET_WM_STATE_FULLSCREEN", True);

	XEvent event = {};
	event.type = ClientMessage;
	event.xclient.window = window.window;
	event.xclient.message_type = wm_state;
	//event.xclient.send_event = True; ???
	event.xclient.format = 32;
	event.xclient.data.l[0] = 0;
	event.xclient.data.l[1] = wm_fullscreen;
	event.xclient.data.l[2] = 0;
	XSendEvent(window.display, window.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void LinuxEnterFullScreen() {
	Atom wm_state = XInternAtom(window.display, "_NET_WM_STATE", True);
	Atom wm_fullscreen = XInternAtom(window.display, "_NET_WM_STATE_FULLSCREEN", True);

	XEvent event = {};
	event.type = ClientMessage;
	event.xclient.window = window.window;
	event.xclient.message_type = wm_state;
	//event.xclient.send_event = True; ???
	event.xclient.format = 32;
	event.xclient.data.l[0] = 1;
	event.xclient.data.l[1] = wm_fullscreen;
	event.xclient.data.l[2] = 0;
	XSendEvent(window.display, window.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void LinuxEnqueueEvent(OSEvent event) {
	window.queue.table[window.queue.writeIndex & EVENT_QUEUE_MASK] = event;
	__asm__ __volatile__ ("":::"memory");
	window.queue.writeIndex++;
}

bool LinuxPollEvent(OSEvent* event) {
	if (window.queue.readIndex == window.queue.writeIndex) return False;
	*event = window.queue.table[window.queue.readIndex & EVENT_QUEUE_MASK];
	__asm__ __volatile__ ("":::"memory");
	window.queue.readIndex++;
	return True;
}

void LinuxSetWindowIcon(Image image) {
	// TODO: why am I allocating and not freeing?

	int longCount = 2 + image.width * image.height;
	byte* icon = (byte*)LinuxAllocate(longCount * sizeof(unsigned long));
	unsigned long* target = (unsigned long*)icon;
	*target++ = image.width;
	*target++ = image.height;
	for (int x = image.width;  x >= 0;  x--) for (int y = 0;  y < image.height;  y++) {
		*target++ = (((unsigned long) image.data[(x*image.height + y) * 4 + 0]) << 16) |
		(((unsigned long) image.data[(x*image.height + y) * 4 + 1]) <<  8) |
		(((unsigned long) image.data[(x*image.height + y) * 4 + 2]) <<  0) |
		(((unsigned long) image.data[(x*image.height + y) * 4 + 3]) << 24);
	}

	XChangeProperty(window.display, window.window,
					window._NET_WM_ICON,
					XA_CARDINAL, 32,
					PropModeReplace,
					(byte*) icon,
					longCount);

	XFlush(window.display);
}

void LinuxSetWindowTitle(const char* title) {
	XStoreName(window.display, window.window, title);
}

Bool LinuxWindowDestroyed() {
	return window.destroyed;
}

void _internalCreateWindow(Window root, const char* title, int width, int height, bool maximized = false) {
	window.root = root;
	XSetWindowAttributes attributes;
	attributes.event_mask = StructureNotifyMask;
	window.window = XCreateWindow(
		window.display, DefaultRootWindow(window.display),
		0, 0, width, height, 
		0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
		&attributes);
	if (!window.window) LOG("Failed to create window");
	XStoreName(window.display, window.window, title);

	XSelectInput(window.display, window.window, StructureNotifyMask | ExposureMask | PointerMotionMask 
		| ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);

	window.WM_PROTOCOLS = XInternAtom(window.display, "WM_PROTOCOLS", False);
	window.WM_DELETE_WINDOW = XInternAtom(window.display , "WM_DELETE_WINDOW", False);
	window.CLIPBOARD = XInternAtom(window.display , "CLIPBOARD", False);
	window.TARGETS = XInternAtom(window.display , "TARGETS", False);
	window.UTF8_STRING = XInternAtom(window.display, "UTF8_STRING", False);

	window._NET_WM_ICON = XInternAtom(window.display, "_NET_WM_ICON", False);

	XSetWMProtocols(window.display, window.window, &window.WM_DELETE_WINDOW, 1);

	if (maximized) {
		Atom _NET_WM_STATE  =  XInternAtom(window.display, "_NET_WM_STATE", False);

		Atom _NET_WM_STATE_MAXIMIZED[2];
		_NET_WM_STATE_MAXIMIZED[0] =  XInternAtom(window.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		_NET_WM_STATE_MAXIMIZED[1] =  XInternAtom(window.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

		XChangeProperty(window.display, window.window, 
			_NET_WM_STATE, XA_ATOM, 
			32, PropModeAppend, 
			(unsigned char*)_NET_WM_STATE_MAXIMIZED, 2);
	}

	// show the window
	XMapWindow(window.display, window.window);

	// load cursors
	window.cursors[CUR_ARROW] = XCreateFontCursor(window.display, XC_left_ptr);
	window.cursors[CUR_MOVE] = XCreateFontCursor(window.display, XC_fleur);	
	window.cursors[CUR_RESIZE] = XCreateFontCursor(window.display, XC_bottom_left_corner);
	window.cursors[CUR_HAND] = XCreateFontCursor(window.display, XC_hand1);
	window.cursors[CUR_MOVESIDE] = XCreateFontCursor(window.display, XC_sb_h_double_arrow);
	window.cursors[CUR_MOVEUPDN] = XCreateFontCursor(window.display, XC_sb_v_double_arrow);
	window.cursors[CUR_TEXT] = XCreateFontCursor(window.display, XC_xterm);

	// What is this even?
	XSetLocaleModifiers("");
	XIM xim = XOpenIM(window.display, 0, 0, 0);
	if(!xim){
		XSetLocaleModifiers("@im=none");
		xim = XOpenIM(window.display, 0, 0, 0);
	}
	window.xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window.window, XNFocusWindow, window.window, NULL);
}

void LinuxCreateWindow(const char* title, int width, int height) {
	window.display = XOpenDisplay(NULL);
	if (!window.display) {
		LOG("Cannot open X display");
		return;
	}
	_internalCreateWindow(DefaultRootWindow(window.display), title, width, height);
}

void LinuxCreateWindowFullScreen(const char* title) {
	window.display = XOpenDisplay(NULL);
	if (!window.display) {
		LOG("Cannot open X display");
		return;
	}
	Window root = DefaultRootWindow(window.display);
	
	XWindowAttributes attributes;
	XGetWindowAttributes(window.display, root, &attributes);	

	_internalCreateWindow(root, title, attributes.width, attributes.height);

	LinuxEnterFullScreen();
}

void LinuxMaximizeWindow() {
	Atom _NET_WM_STATE  =  XInternAtom(window.display, "_NET_WM_STATE", False);
	Atom _NET_WM_STATE_MAXIMIZED_HORZ =  XInternAtom(window.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	Atom _NET_WM_STATE_MAXIMIZED_VERT =  XInternAtom(window.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = window.window;
	xev.xclient.message_type = _NET_WM_STATE;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = _NET_WM_STATE_MAXIMIZED_HORZ;
	xev.xclient.data.l[2] = _NET_WM_STATE_MAXIMIZED_VERT;

	XSendEvent(window.display, DefaultRootWindow(window.display), False, SubstructureNotifyMask, &xev);
}

uint32 GetVkCodeFromSymbol(KeySym symbol) {
	uint32 vkCode = 0;
	switch (symbol) {
	case XK_BackSpace: {vkCode = KEY_BACKSPACE;} break;
	case XK_Tab      : {vkCode = KEY_TAB      ;} break;
	case XK_Return   : {vkCode = KEY_ENTER    ;} break;
	case XK_Shift_L  :
	case XK_Shift_R  : {vkCode = KEY_SHIFT    ;} break;
	case XK_Control_L:
	case XK_Control_R: {vkCode = KEY_CTRL	  ;} break;
	case XK_Alt_L    :
	case XK_Alt_R    : {vkCode = KEY_ALT      ;} break;
	case XK_Escape   : {vkCode = KEY_ESC 	  ;} break;
	case XK_space    : {vkCode = KEY_SPACE    ;} break;
	case XK_Page_Up  : {vkCode = KEY_PGUP	  ;} break;
	case XK_Page_Down: {vkCode = KEY_PGDN     ;} break;
	case XK_End      : {vkCode = KEY_END      ;} break;
	case XK_Home     : {vkCode = KEY_HOME     ;} break;
	case XK_Left     : {vkCode = KEY_LEFT     ;} break;
	case XK_Up       : {vkCode = KEY_UP	      ;} break;
	case XK_Right    : {vkCode = KEY_RIGHT    ;} break;
	case XK_Down     : {vkCode = KEY_DOWN     ;} break;
	case XK_Delete   : {vkCode = KEY_DELETE   ;} break;
	case XK_greater  : {vkCode = KEY_GREATER  ;} break;
	case XK_A        :
	case XK_a        : {vkCode = KEY_A  	  ;} break;
	case XK_B        :
	case XK_b        : {vkCode = KEY_B  	  ;} break;
	case XK_C        :
	case XK_c        : {vkCode = KEY_C  	  ;} break;
	case XK_D        :
	case XK_d        : {vkCode = KEY_D  	  ;} break;
	case XK_E        :
	case XK_e        : {vkCode = KEY_E  	  ;} break;
	case XK_F        :
	case XK_f        : {vkCode = KEY_F  	  ;} break;
	case XK_G        :
	case XK_g        : {vkCode = KEY_G  	  ;} break;
	case XK_H        :
	case XK_h        : {vkCode = KEY_H  	  ;} break;
	case XK_I        :
	case XK_i        : {vkCode = KEY_I  	  ;} break;
	case XK_J        :
	case XK_j        : {vkCode = KEY_J  	  ;} break;
	case XK_K        :
	case XK_k        : {vkCode = KEY_K  	  ;} break;
	case XK_L        :
	case XK_l        : {vkCode = KEY_L		  ;} break;
	case XK_M        :
	case XK_m        : {vkCode = KEY_M	      ;} break;
	case XK_N        :
	case XK_n        : {vkCode = KEY_N  	  ;} break;
	case XK_O        :
	case XK_o        : {vkCode = KEY_O  	  ;} break;
	case XK_P        :
	case XK_p        : {vkCode = KEY_P  	  ;} break;
	case XK_Q        :
	case XK_q        : {vkCode = KEY_Q  	  ;} break;
	case XK_R        :
	case XK_r        : {vkCode = KEY_R  	  ;} break;
	case XK_S        :
	case XK_s        : {vkCode = KEY_S  	  ;} break;
	case XK_T        :
	case XK_t        : {vkCode = KEY_T  	  ;} break;
	case XK_U        :
	case XK_u        : {vkCode = KEY_U  	  ;} break;
	case XK_V        :
	case XK_v        : {vkCode = KEY_V  	  ;} break;
	case XK_W        :
	case XK_w        : {vkCode = KEY_W  	  ;} break;
	case XK_X        :
	case XK_x        : {vkCode = KEY_X  	  ;} break;
	case XK_Y        :
	case XK_y        : {vkCode = KEY_Y  	  ;} break;
	case XK_Z        :
	case XK_z        : {vkCode = KEY_Z  	  ;} break;
	case XK_F1       : {vkCode = KEY_F1 	  ;} break;
	case XK_F2       : {vkCode = KEY_F2 	  ;} break;
	case XK_F3       : {vkCode = KEY_F3 	  ;} break;
	case XK_F4       : {vkCode = KEY_F4 	  ;} break;
	case XK_F5       : {vkCode = KEY_F5 	  ;} break;
	case XK_F6       : {vkCode = KEY_F6 	  ;} break;
	case XK_F7       : {vkCode = KEY_F7 	  ;} break;
	case XK_F8       : {vkCode = KEY_F8 	  ;} break;
	case XK_F9       : {vkCode = KEY_F9 	  ;} break;
	case XK_F10      : {vkCode = KEY_F10      ;} break;
	case XK_F11      : {vkCode = KEY_F11      ;} break;
	case XK_F12      : {vkCode = KEY_F12      ;} break;
	}
	return vkCode;
}

void LinuxProcessWindowEvents() {
	while (XPending(window.display)) {
		XEvent event;
		XNextEvent(window.display, &event);
		switch (event.type) {
			case ClientMessage: {
				XClientMessageEvent client = event.xclient;
				if (client.message_type == window.WM_PROTOCOLS) {
					Atom protocol = client.data.l[0];
					if (protocol == window.WM_DELETE_WINDOW) {
						window.destroyed = True;
					}
				}
			} break;
			case DestroyNotify: {
				XDestroyWindowEvent destroywindow = event.xdestroywindow;
				if ((window.display == destroywindow.display) && (window.window == destroywindow.window)) {
					window.destroyed = True;
				}
			} break;
			case ButtonPress: {
				XButtonEvent button = event.xbutton;
				window.cursorPos = {button.x, button.y};
				OSEvent osevent;
				osevent.time = button.time;
				osevent.mouse.cursorPos = {button.x, button.y};
				osevent.mouse.ctrlIsDown = (button.state & ControlMask) == ControlMask;
				osevent.type = Event_none;

				if (button.button == Button1) {
					Time time = button.time;

					int32 dx = ABS(window.cursorPos.x - window.lastClickPoint.x);
					int32 dy = ABS(window.cursorPos.y - window.lastClickPoint.y);
					if (dx > 4 || dy > 4 || time - window.timeLastClicked > 500) {
						window.clickCount = 0;
					}
					window.clickCount++;

					window.timeLastClicked = time;
					window.lastClickPoint = window.cursorPos;

					osevent.type = Event_MouseLeftButtonDown;
				}
				else if (event.xbutton.button == Button3) {
					window.clickCount = 0;

					osevent.type = Event_MouseRightButtonDown;
				}
				else if (event.xbutton.button == Button4) {
					osevent.mouse.wheelDelta = 72;
					osevent.type = Event_MouseVerticalWheel;
				}
				else if (event.xbutton.button == Button5) {
					osevent.mouse.wheelDelta = -72;
					osevent.type = Event_MouseVerticalWheel;
				}
				else if (event.xbutton.button == Button6) {
					osevent.mouse.wheelDelta = -72;
					osevent.type = Event_MouseHorizontalWheel;
				}
				else if (event.xbutton.button == Button7) {
					osevent.mouse.wheelDelta = 72;
					osevent.type = Event_MouseHorizontalWheel;
				}

				if (event.type != Event_none)
					LinuxEnqueueEvent(osevent);
			} break;
			case ButtonRelease: {
				XButtonEvent button = event.xbutton;
				window.cursorPos = {button.x, button.y};

				OSEvent osevent;
				osevent.type = Event_none;
				osevent.time = button.time;
				osevent.mouse.cursorPos = {button.x, button.y};
				osevent.mouse.ctrlIsDown = (button.state & ControlMask) == ControlMask;

				if (button.button == Button1) {
					osevent.type = Event_MouseLeftButtonUp;
				}
				else if (button.button == Button3) {
					osevent.type = Event_MouseRightButtonUp;
				}
				if (event.type != Event_none)
					LinuxEnqueueEvent(osevent);
			} break;
			case MotionNotify: {
				window.cursorPos = {event.xmotion.x, event.xmotion.y};
				OSEvent osevent;
				osevent.type = Event_MouseMove;
				osevent.time = event.xmotion.time;
				osevent.mouse.cursorPos = {event.xmotion.x, event.xmotion.y};
				osevent.mouse.ctrlIsDown = (event.xmotion.state & ControlMask) == ControlMask;
				LinuxEnqueueEvent(osevent);
			} break;
			case ConfigureNotify: {
				window.dim = {event.xconfigure.width, event.xconfigure.height};

				OSEvent osevent;
				osevent.type = Event_WindowResize;
				osevent.window.dim = {event.xconfigure.width, event.xconfigure.height};
				osevent.window.prevDim = window.dim;
				LinuxEnqueueEvent(osevent);
			} break;
			case KeyPress: {
				KeySym symbol = NoSymbol;
				Status status;
				char buffer[100];
				int length = Xutf8LookupString(window.xic, &event.xkey, buffer, sizeof(buffer)-1, &symbol, &status);

				if (status == XBufferOverflow) {
					// TODO: increase the buffer
				}

				OSEvent osevent;
				osevent.type = Event_KeyboardPress;
				osevent.time = event.xkey.time;
				osevent.keyboard.scanCode = event.xkey.keycode;
				osevent.keyboard.vkCode = GetVkCodeFromSymbol(symbol);
				osevent.keyboard.ctrlIsDown = (event.xkey.state & ControlMask) != 0;
				osevent.keyboard.shiftIsDown = (event.xkey.state & ShiftMask) != 0;
				LinuxEnqueueEvent(osevent);

				if (status == XLookupChars || status == XLookupBoth) {					
					osevent.type = Event_KeyboardChar;

					for (int i = 0; i < length; i++) {
						// TODO: UTF8
						osevent.keyboard.character = buffer[i];
						LinuxEnqueueEvent(osevent);
					}
				}
			} break;
			case KeyRelease: {
				OSEvent osevent;
				osevent.type = Event_KeyboardKeyUp;
				osevent.time = event.xkey.time;
				osevent.keyboard.scanCode = event.xkey.keycode;
				//osevent.keyboard.vkCode = GetVkCodeFromSymbol(symbol);
				osevent.keyboard.ctrlIsDown = (event.xkey.state & ControlMask) != 0;
				osevent.keyboard.shiftIsDown = (event.xkey.state & ShiftMask) != 0;
				LinuxEnqueueEvent(osevent);
			} break;
			case SelectionNotify: {
				XSelectionEvent selection = event.xselection;
				if (selection.property != None) {
					Atom actualType;
					int actualFormat;
					unsigned long bytesAfter;
					unsigned char* data;
					unsigned long count;
					XGetWindowProperty(window.display, window.window, window.CLIPBOARD, 0, LONG_MAX, False, AnyPropertyType,
						&actualType, &actualFormat, &count, &bytesAfter, &data);
					
					if (selection.target == window.TARGETS) {
						Atom* list = (Atom*)data;
						window.target = None;
						for (unsigned long i = 0; i < count; i++) {
							if (list[i] == XA_STRING) {
								window.target = XA_STRING;
							}
							else if (list[i] == window.UTF8_STRING) {
								window.target = window.UTF8_STRING;
								break;
							}
						}
						if (window.target != None)
							XConvertSelection(window.display, window.CLIPBOARD, window.target, window.CLIPBOARD, window.window, CurrentTime);
					}

					else if (selection.target == window.target) {
						String pasted = {data, (ssize)count};
						window.pasteCallback(pasted);
					}

					if (data) XFree(data);
				}
			} break;
			case SelectionRequest: {
				if ((XGetSelectionOwner(window.display, window.CLIPBOARD) == window.window) && event.xselectionrequest.selection == window.CLIPBOARD) {
					XSelectionRequestEvent request = event.xselectionrequest;
					if (request.target == window.TARGETS && request.property != None) {
						// We are asked for a targets list and we reply with a list of length 1, only UTF8_STRING.
						XChangeProperty(request.display, request.requestor, request.property, 
						XA_ATOM, 32, PropModeReplace, (unsigned char*)&window.UTF8_STRING, 1);
					}
					else if (request.target == window.UTF8_STRING && request.property != None) {
						// The target was chosen, and now we deliver the selection
						XChangeProperty(request.display, request.requestor, request.property, 
							request.target, 8, PropModeReplace, window.selection.data, window.selection.length);
					}

					XSelectionEvent sendEvent;
					sendEvent.type = SelectionNotify;
					sendEvent.serial = request.serial;
					sendEvent.send_event = request.send_event;
					sendEvent.display = request.display;
					sendEvent.requestor = request.requestor;
					sendEvent.selection = request.selection;
					sendEvent.target = request.target;
					sendEvent.property = request.property;
					sendEvent.time = request.time;
					XSendEvent(window.display, request.requestor, 0, 0, (XEvent*)&sendEvent);
				}
			} break;
		}
	}
}

void LinuxSetCursorIcon(int32 icon) {
	XDefineCursor(window.display, window.window, window.cursors[icon]);
}

Point2i LinuxGetCursorPosition() {
	return window.cursorPos;
}

int OpenPipe(const char* cmd) {
	int pipefd[2];
	pipe(pipefd);
	int write_end = pipefd[1];
	int read_end = pipefd[0];
	int pid = fork();
	if (pid == -1) {
		LOG("failed to create new process");
		close(write_end);
		return -1;
	}
	else if (pid == 0) {
		close(read_end);
		dup2(write_end, 1);
		close(write_end);
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		exit(127);
	}

	close(write_end);
	return read_end;
}

int LinuxOpenFileDialog() {
	return OpenPipe("zenity --file-selection");
}

int LinuxSaveFileDialog() {
	return OpenPipe("zenity --file-selection --save");
}

void LinuxCopyToClipboard(String str) {
	LinuxFree(window.selection.data, window.selection.length);
	window.selection.data = (byte*)LinuxAllocate(str.length);
	window.selection.length = str.length;
	StringCopy(str, window.selection.data);
	XSetSelectionOwner(window.display, window.CLIPBOARD, window.window, CurrentTime);
}

void LinuxRequestClipboardData(void (*callback)(String)) {
	window.pasteCallback = callback;
	XConvertSelection(window.display, window.CLIPBOARD, window.TARGETS, window.CLIPBOARD, window.window, CurrentTime);
}

char** LinuxGetFontsPath(int* n) {
	return XGetFontPath(window.display, n);
} 