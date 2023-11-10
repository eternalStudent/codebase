struct {
	struct android_app* handle
} app;

void AndroidProcessAppEvents() {
	int ident;
	int events;
	struct android_poll_source* source;
	while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
		if (source != NULL) {
			source->process(app.handle, source);
		}
	}
}