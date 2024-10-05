struct UISlider {
	Box2 handle;
	Box2 boundaries;

	Point2 grabPos;
	bool isGrabbing;
	bool isActive;
	bool isFocused; // intercept keyboard
};

struct UISliderMixer {
	int32 count;
	Box2* handles;
	Box2* boundaries;

	Point2 grabPos;
	bool isGrabbing;
	int32 active;
	int32 focused;
};

Box2 GetNewBoxButKeepInBounds(float32 x, float32 y, float32 width, float32 height, Box2 bounds) {
	if (x < bounds.x0) x = bounds.x0;
	if (x + width >= bounds.x1) x = bounds.x1 - width;
	if (y < bounds.y0) y = bounds.y0;
	if (y + height >= bounds.y1) y = bounds.y1 - height;

	return {x, y, x + width, y + height};
}

bool UISliderProcessEvent(UISlider* slider, OSEvent event) {

	float32 width = slider->handle.x1 - slider->handle.x0;
	float32 height = slider->handle.y1 - slider->handle.y0;
	Point2 cursor = event.mouse.cursorPos + 0.5f;

	switch (event.type) {
	case Event_MouseMove: {
		if (slider->isGrabbing) {
			if (cursor.x != slider->grabPos.x || cursor.y != slider->grabPos.y) {

				float32 x = slider->grabPos.x + cursor.x;
				float32 y = slider->grabPos.y + cursor.y;
				slider->handle = GetNewBoxButKeepInBounds(x, y, width, height, slider->boundaries);
			}

			OSSetCursorIcon(CUR_ARROW);
			return true;
		}
		else if (InBounds(slider->handle, cursor)) {
			slider->isActive = true;

			OSSetCursorIcon(CUR_ARROW);
			return true;
		}
		else {
			slider->isActive = false;

			return false;
		}
	} break;

	case Event_MouseLeftButtonDown: {
		if (slider->isActive){
			slider->isGrabbing = true;
			slider->grabPos = slider->handle.p0 - cursor;

			return true;
		}
		else if (InBounds(slider->boundaries, cursor)) {

			float32 x = cursor.x - 0.5f*width;
			float32 y = cursor.y - 0.5f*height;
			slider->handle = GetNewBoxButKeepInBounds(x, y, width, height, slider->boundaries);

			slider->isActive = true;

			return true;
		}
		else {
			return false;
		}
	} break;

	case Event_MouseLeftButtonUp: {
		slider->isGrabbing = false;

		return false;
	} break;

	case Event_KeyboardPress: {
		if (!slider->isFocused) return false;

		switch (event.keyboard.vkCode) {
		case KEY_LEFT: {
			if (slider->boundaries.x0 <= slider->handle.x0 - 1) {
				slider->handle.x0--;
				slider->handle.x1--;
			}
		} break;
		case KEY_RIGHT: {
			if (slider->handle.x1 + 1 < slider->boundaries.x1) {
				slider->handle.x0++;
				slider->handle.x1++;
			}
		} break;
		case KEY_UP: {
			if (slider->boundaries.y0 <= slider->handle.y0 - 1) {
				slider->handle.y0--;
				slider->handle.y1--;
			}
		} break;
		case KEY_DOWN: {
			if (slider->handle.y1 + 1 < slider->boundaries.y1) {
				slider->handle.y0++;
				slider->handle.y1++;
			}
		} break;
		}

		return true;
	} break;
	}

	return false;
}

bool UISliderMixerProcessEvent(UISliderMixer* mixer, OSEvent event) {

	switch (event.type) {
	case Event_MouseMove: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;
		if (mixer->isGrabbing) {
			if (cursor.x != mixer->grabPos.x || cursor.y != mixer->grabPos.y) {

				float32 x = mixer->grabPos.x + cursor.x;
				float32 y = mixer->grabPos.y + cursor.y;
				Box2 handle = mixer->handles[mixer->active - 1];
				Box2 bounds = mixer->boundaries[mixer->active - 1];
				float32 width = handle.x1 - handle.x0;
				float32 height = handle.y1 - handle.y0;
				mixer->handles[mixer->active - 1] = GetNewBoxButKeepInBounds(x, y, width, height, bounds);
			}

			OSSetCursorIcon(CUR_ARROW);
			return true;
		}

		mixer->active = 0;
		for (int32 i = 0; i < mixer->count; i++) {
			Box2 handle = mixer->handles[i];

			if (InBounds(handle, cursor)) {
				mixer->active = i + 1;

				OSSetCursorIcon(CUR_ARROW);
				return true;	
			}
		}
		
		return false;
	} break;

	case Event_MouseLeftButtonDown: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;
		if (mixer->active){
			mixer->isGrabbing = true;
			mixer->grabPos = mixer->handles[mixer->active - 1].p0 - cursor;

			return true;
		}

		for (int32 i = 0; i < mixer->count; i++) {
			Box2 handle = mixer->handles[i];
			Box2 bounds = mixer->boundaries[i];

			if (InBounds(bounds, cursor)) {
				float32 width = handle.x1 - handle.x0;
				float32 height = handle.y1 - handle.y0;
				float32 x = cursor.x - 0.5f*width;
				float32 y = cursor.y - 0.5f*height;
				mixer->handles[i] = GetNewBoxButKeepInBounds(x, y, width, height, bounds);
				mixer->active = i + 1;

				return true;	
			}
		}

		return false;
	} break;

	case Event_MouseLeftButtonUp: {
		mixer->isGrabbing = false;

		return false;
	} break;

	case Event_KeyboardPress: {
		if (!mixer->focused) return false;

		Box2* handle = mixer->handles + mixer->focused - 1;
		Box2* bounds = mixer->boundaries + mixer->focused - 1;

		switch (event.keyboard.vkCode) {
		case KEY_LEFT: {
			if (bounds->x0 <= handle->x0 - 1) {
				handle->x0--;
				handle->x1--;
			}
		} break;
		case KEY_RIGHT: {
			if (handle->x1 + 1 < bounds->x1) {
				handle->x0++;
				handle->x1++;
			}
		} break;
		case KEY_UP: {
			if (bounds->y0 <= handle->y0 - 1) {
				handle->y0--;
				handle->y1--;
			}
		} break;
		case KEY_DOWN: {
			if (handle->y1 + 1 < bounds->y1) {
				handle->y0++;
				handle->y1++;
			}
		} break;
		}

		return true;
	} break;
	}

	return false;
}