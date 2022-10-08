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
	Cursor cursors[7];
	XIC xic;

	char keyPressed[256];
	char shiftIsDown;
	char shiftWasDown;
	char ctrlIsDown;
	char ctrlWasDown;
	char typed[4];
	int32 strlength;
	
	char mouseLeftButtonIsDown;
	char mouseRightButtonIsDown;
	char mouseLeftButtonWasDown;
	char mouseRightButtonWasDown;
	int32 clickCount;
	int32 prevClickCount;
	Point2i lastClickPoint;
	Time timeLastClicked;
	int32 mouseWheelDelta;
	int32 mouseHWheelDelta;

	Point2i cursorPos;
	Dimensions2i dim;
	Bool destroyed;

	// clipboard
	Atom CLIPBOARD;
	Atom TARGETS;
	Atom UTF8_STRING;
	String selection;
	Atom target;
	void (*pasteCallback)(String);
	
} window;

void _internalCreateWindow(Window root, const char* title, int width, int height) {
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

void LinuxProcessWindowEvents() {
	memset(window.keyPressed, 0, 256);
	window.ctrlWasDown = window.ctrlIsDown;
	window.shiftWasDown = window.shiftIsDown;
	window.mouseLeftButtonWasDown = window.mouseLeftButtonIsDown;
	window.mouseRightButtonWasDown = window.mouseRightButtonIsDown;
	window.prevClickCount = window.clickCount;
	window.strlength = 0;
	window.mouseWheelDelta = 0;
	window.mouseHWheelDelta = 0;
	window.strlength = 0;

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
				if (button.button == Button1) {
					window.mouseLeftButtonIsDown = 1;

					Time time = button.time;

					int32 dx = ABS(window.cursorPos.x - window.lastClickPoint.x);
					int32 dy = ABS(window.cursorPos.y - window.lastClickPoint.y);
					if (dx > 4 || dy > 4 || time - window.timeLastClicked > 500) {
						window.clickCount = 0;
					}
					window.clickCount++;

					window.timeLastClicked = time;
					window.lastClickPoint = window.cursorPos;
				}
				//else if (button.button == Button2) {
				//	window.mouse[1] = 1;
				//}
				else if (event.xbutton.button == Button3) {
					window.mouseRightButtonIsDown = 1;
					window.clickCount = 0;
				}
				else if (event.xbutton.button == Button4) {
					window.mouseWheelDelta = 72;
				}
				else if (event.xbutton.button == Button5) {
					window.mouseWheelDelta = -72;
				}
				else if (event.xbutton.button == Button6) {
					window.mouseHWheelDelta = -72;
				}
				else if (event.xbutton.button == Button7) {
					window.mouseHWheelDelta = 72;
				}
			} break;
			case ButtonRelease: {
				XButtonEvent button = event.xbutton;
				window.cursorPos = {button.x, button.y};
				if (button.button == Button1) {
					window.mouseLeftButtonIsDown = 0;
				}
				else if (button.button == Button3) {
					window.mouseRightButtonIsDown = 0;
				} 
			} break;
			case MotionNotify: {
				window.cursorPos = {event.xmotion.x, event.xmotion.y};
			} break;
			case ConfigureNotify: {
				window.dim = {event.xconfigure.width, event.xconfigure.height};
			} break;
			case KeyPress: {
				KeySym symbol = NoSymbol;
				Status status;
				char typed[4];
				int32 strlength = Xutf8LookupString(window.xic, &event.xkey, typed, 4, &symbol, &status);

				if (strlength == 1 && (32 <= typed[0] && typed[0] < 127 || 9 <= typed[0] && typed[0] <= 10)) {
					window.typed[window.strlength] = typed[0];
					window.strlength++;
				}

				if (symbol == XK_Control_L || symbol == XK_Control_R) {window.ctrlIsDown = 1;} 
				else if (symbol == XK_Shift_L || symbol == XK_Shift_R) {window.shiftIsDown = 1;}
				else if (symbol == XK_BackSpace ) {window.keyPressed[KEY_BACKSPACE] = 1;}
				else if (symbol == XK_Tab       ) {window.keyPressed[KEY_TAB]       = 1;}
				else if (symbol == XK_Return    ) {window.keyPressed[KEY_ENTER]     = 1;}
				else if (symbol == XK_Escape    ) {window.keyPressed[KEY_ESC]       = 1;}
				else if (symbol == XK_space     ) {window.keyPressed[KEY_SPACE]     = 1;}
				else if (symbol == XK_Page_Up   ) {window.keyPressed[KEY_PGUP]      = 1;}
				else if (symbol == XK_Page_Down ) {window.keyPressed[KEY_PGDN]      = 1;}
				else if (symbol == XK_End       ) {window.keyPressed[KEY_END]       = 1;}
				else if (symbol == XK_Home      ) {window.keyPressed[KEY_HOME]      = 1;}
				else if (symbol == XK_Left      ) {window.keyPressed[KEY_LEFT]      = 1;}
				else if (symbol == XK_Up        ) {window.keyPressed[KEY_UP]        = 1;}
				else if (symbol == XK_Right     ) {window.keyPressed[KEY_RIGHT]     = 1;}
				else if (symbol == XK_Down      ) {window.keyPressed[KEY_DOWN]      = 1;}
				else if (symbol == XK_Delete    ) {window.keyPressed[KEY_DELETE]    = 1;}
				else if (symbol == XK_A || symbol == XK_a) {window.keyPressed[KEY_A]= 1;}
				else if (symbol == XK_C || symbol == XK_c) {window.keyPressed[KEY_C]= 1;}
				else if (symbol == XK_V || symbol == XK_v) {window.keyPressed[KEY_V]= 1;}
				else if (symbol == XK_X || symbol == XK_X) {window.keyPressed[KEY_X]= 1;}
				else window.keyPressed[symbol & 0xff] = 1;
			} break;
			case KeyRelease: {
				KeySym symbol = XkbKeycodeToKeysym(window.display, event.xkey.keycode, 0, 0);
				if (symbol == XK_Control_L || symbol == XK_Control_R) {
					window.ctrlIsDown = 0;
				} 
				else if (symbol == XK_Shift_L || symbol == XK_Shift_R) {
					window.shiftIsDown = 0;
				}
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


void LinuxSetWindowIcon(Image image) {
	int longCount = 2 + image.width * image.height;
	byte* icon = (byte*)LinuxHeapAllocate(longCount * sizeof(unsigned long));
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

Bool LinuxIsKeyDown(int key) {
	if (key == 0x10) return window.shiftIsDown == 1;
	if (key == 0x11) return window.ctrlIsDown == 1;
	return window.keyPressed[key] == 1;
}

Bool LinuxIsKeyPressed(int key) {
	if (key == 0x10) return window.shiftIsDown == 1 && window.shiftWasDown == 0;
	if (key == 0x11) return window.ctrlIsDown == 1 && window.ctrlWasDown == 0;
	return window.keyPressed[key] == 1;
}

Bool LinuxIsMouseLeftButtonDown() {
	return window.mouseLeftButtonIsDown == 1 && window.clickCount == 1;
}

Bool LinuxIsMouseLeftButtonUp() {
	return window.mouseLeftButtonIsDown == 0;
}

Bool LinuxIsMouseLeftReleased() {
	return window.mouseLeftButtonIsDown == 0 && window.mouseLeftButtonWasDown == 1;
}

Bool LinuxIsMouseLeftClicked() {
	return window.clickCount == 1 && window.mouseLeftButtonIsDown == 1 && window.mouseLeftButtonWasDown == 0;
}

Bool LinuxIsMouseRightClicked() {
	return window.mouseRightButtonIsDown == 1 && window.mouseRightButtonWasDown == 0;
}

Bool LinuxIsMouseDoubleClicked() {
	return window.clickCount == 2 && window.prevClickCount == 1;
}

Bool LinuxIsMouseTripleClicked() {
	return window.clickCount == 3 && window.prevClickCount == 2;
}

void LinuxResetMouse(){
	window.mouseLeftButtonIsDown = 0;
}

int32 LinuxGetMouseWheelDelta() {
	return window.mouseWheelDelta;
}

int32 LinuxGetMouseHWheelDelta() {
	return window.mouseHWheelDelta;
}

String LinuxGetTypedText() {
	return {(byte*)window.typed, (ssize)window.strlength};
}

void LinuxResetTypedText() {
	window.strlength = 0;
}

Bool LinuxWindowDestroyed() {
	return window.destroyed;
}

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
	if (pid = -1) {
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
	LinuxHeapFree(window.selection.data, window.selection.length);
	window.selection.data = (byte*)LinuxHeapAllocate(str.length);
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