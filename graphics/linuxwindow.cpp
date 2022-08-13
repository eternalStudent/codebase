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
	Bool destroyed;
	char mouse[3];
	char mouse_prev[3];
	char keys[256];
	char keys_prev[256];
	Point2i cursorPos;
	Dimensions2i dim;
	Window root;
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

	// subscribe to window close notification
	window.WM_PROTOCOLS = XInternAtom(window.display, "WM_PROTOCOLS", False);
	window.WM_DELETE_WINDOW = XInternAtom(window.display , "WM_DELETE_WINDOW", False);
	XSetWMProtocols(window.display, window.window, &window.WM_DELETE_WINDOW, 1);

	// show the window
	XMapWindow(window.display, window.window);
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
	for (int32 i = 0; i < 256; i++) window.keys_prev[i] = window.keys[i];
	window.mouse_prev[0] = window.mouse[0];
	window.mouse_prev[1] = window.mouse[1];
	window.mouse_prev[2] = window.mouse[2];

	if (XPending(window.display)) {
		XEvent event;
		XNextEvent(window.display, &event);
		switch (event.type) {
			case ClientMessage: {
				if (event.xclient.message_type == window.WM_PROTOCOLS) {
					Atom protocol = event.xclient.data.l[0];
					if (protocol == window.WM_DELETE_WINDOW) {
						window.destroyed = True;
					}
			 	}
			} break;
			case DestroyNotify: {
                if ((window.display == event.xdestroywindow.display) &&
                    (window.window == event.xdestroywindow.window)) {
                    window.destroyed = True;
                }
            } break;
            case ButtonPress: {
            	window.cursorPos = {event.xbutton.x, event.xbutton.y};
            	if (event.xbutton.button == Button1) {
            		window.mouse[0] = 1;
            	}
            	else if (event.xbutton.button == Button2) {
            		window.mouse[1] = 1;
            	}
            	else if (event.xbutton.button == Button3) {
            		window.mouse[2] = 1;
            	} 
            } break;
            case ButtonRelease: {
            	window.cursorPos = {event.xbutton.x, event.xbutton.y};
            	if (event.xbutton.button == Button1) {
            		window.mouse[0] = 0;
            	}
            	else if (event.xbutton.button == Button2) {
            		window.mouse[1] = 0;
            	}
            	else if (event.xbutton.button == Button3) {
            		window.mouse[2] = 0;
            	} 
            } break;
            case MotionNotify: {
            	window.cursorPos = {event.xmotion.x, event.xmotion.y};
            } break;
            case ConfigureNotify: {
            	window.dim = {event.xconfigure.width, event.xconfigure.height};
            } break;
            case KeyPress: {
            	window.keys[event.xkey.keycode && 0xff] = 1;
            } break;
            case KeyRelease: {
            	window.keys[event.xkey.keycode && 0xff] = 0;
            } break;
		}
	}
}

Bool LinuxIsKeyDown(int key) {
	return window.keys[key] == 1;
}

Bool LinuxIsKeyPressed(int key) {
	return window.keys[key] == 1 && window.keys_prev[key] == 0;
}

Bool LinuxIsMouseDown(int mouse) {
	return window.mouse[mouse] == 1;
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

// TODO: preload cursors
void LinuxSetCursorIcon(unsigned int shape) {
	XDefineCursor(window.display, window.window, XCreateFontCursor(window.display, shape));
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