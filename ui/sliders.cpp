/*
 * NOTE:
 * This is a plug-n-play style library for adding sliders without a ui framework.
 * 
 */

struct UISlider {
	Box2i hitBox;
	Box2i boundaries;

	Point2i grabPos;
	bool isGrabbing;
	bool isActive;
	bool isFocused; // intercept keyboard
};

Box2i GetNewBoxButKeepInBounds(int32 x, int32 y, int32 width, int32 height, Box2i bounds) {
	if (x < bounds.x0) x = bounds.x0;
	if (x + width >= bounds.x1) x = bounds.x1 - width;
	if (y < bounds.y0) y = bounds.y0;
	if (y + height >= bounds.y1) y = bounds.y1 - height;

	return {x, y, x + width, y + height};
}

bool UISliderProcessEvent(OSEvent event, UISlider* slider) {

	int32 width = slider->hitBox.x1 - slider->hitBox.x0;
	int32 height = slider->hitBox.y1 - slider->hitBox.y0;
	Point2i cursor = event.mouse.cursorPos;

	switch (event.type) {
	case Event_MouseMove: {

		if (slider->isGrabbing) {
			if (cursor.x != slider->grabPos.x || cursor.y != slider->grabPos.y) {

				int32 x = slider->grabPos.x + cursor.x;
				int32 y = slider->grabPos.y + cursor.y;
				slider->hitBox = GetNewBoxButKeepInBounds(x, y, width, height, slider->boundaries);
			}

			return true;
		}
		else if (InBounds(slider->hitBox, cursor)) {

			slider->isActive = true;

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
			slider->grabPos = slider->hitBox.p0 - cursor;

			return true;
		}
		else if (InBounds(slider->boundaries, cursor)) {

			int32 x = cursor.x - width/2;
			int32 y = cursor.y - height/2;
			slider->hitBox = GetNewBoxButKeepInBounds(x, y, width, height, slider->boundaries);

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
			if (slider->boundaries.x0 <= slider->hitBox.x0 - 1) {
				slider->hitBox.x0--;
				slider->hitBox.x1--;
			}
		} break;
		case KEY_RIGHT: {
			if (slider->hitBox.x1 + 1 < slider->boundaries.x1) {
				slider->hitBox.x0++;
				slider->hitBox.x1++;
			}
		} break;
		case KEY_UP: {
			if (slider->boundaries.y0 <= slider->hitBox.y0 - 1) {
				slider->hitBox.y0--;
				slider->hitBox.y1--;
			}
		} break;
		case KEY_DOWN: {
			if (slider->hitBox.y1 + 1 < slider->boundaries.y1) {
				slider->hitBox.y0++;
				slider->hitBox.y1++;
			}
		} break;
		}

		return true;
	} break;
	}

	return false;
}