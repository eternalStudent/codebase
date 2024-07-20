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

// NOTE: The bounds are in terms of pixel grid boundaries, while the cursor is in term of pixel centers,
//       that is why the lower bounds is inclusive, and the upper bound is exclusive
bool InBounds(Box2i bounds, Point2i p) {
	return bounds.x0 <= p.x && p.x < bounds.x1 && bounds.y0 <= p.y && p.y < bounds.y1;
}

Box2i GetNewBoxButKeepInBounds(int32 x, int32 y, int32 width, int32 height, Box2i bounds) {
	if (x < bounds.x0) x = bounds.x0;
	if (x + width >= bounds.x1) x = bounds.x1 - width;
	if (y < bounds.y0) y = bounds.y0;
	if (y + height >= bounds.y1) y = bounds.y1 - height;

	return {x, y, x + width, y + height};
}

bool SliderProcessEvent(OSEvent event, UISlider* status) {

	int32 width = status->hitBox.x1 - status->hitBox.x0;
	int32 height = status->hitBox.y1 - status->hitBox.y0;
	Point2i cursor = event.mouse.cursorPos;

	switch (event.type) {
	case Event_MouseMove: {

		if (status->isGrabbing) {
			if (cursor.x != status->grabPos.x || cursor.y != status->grabPos.y) {

				int32 x = status->grabPos.x + cursor.x;
				int32 y = status->grabPos.y + cursor.y;
				status->hitBox = GetNewBoxButKeepInBounds(x, y, width, height, status->boundaries);
			}

			return true;
		}
		else if (InBounds(status->hitBox, cursor)) {

			status->isActive = true;

			return true;
		}
		else {
			status->isActive = false;

			return false;
		}
	} break;

	case Event_MouseLeftButtonDown: {
		if (status->isActive){
			status->isGrabbing = true;
			status->grabPos = status->hitBox.p0 - cursor;

			return true;
		}
		else if (InBounds(status->boundaries, cursor)) {

			int32 x = cursor.x - width/2;
			int32 y = cursor.y - height/2;
			status->hitBox = GetNewBoxButKeepInBounds(x, y, width, height, status->boundaries);

			status->isActive = true;

			return true;
		}
		else {
			return false;
		}
	} break;

	case Event_MouseLeftButtonUp: {
		status->isGrabbing = false;

		return false;
	} break;

	case Event_KeyboardPress: {
		if (!status->isFocused) return false;

		switch (event.keyboard.vkCode) {
		case KEY_LEFT: {
			if (status->boundaries.x0 <= status->hitBox.x0 - 1) {
				status->hitBox.x0--;
				status->hitBox.x1--;
			}
		} break;
		case KEY_RIGHT: {
			if (status->hitBox.x1 + 1 < status->boundaries.x1) {
				status->hitBox.x0++;
				status->hitBox.x1++;
			}
		} break;
		case KEY_UP: {
			if (status->boundaries.y0 <= status->hitBox.y0 - 1) {
				status->hitBox.y0--;
				status->hitBox.y1--;
			}
		} break;
		case KEY_DOWN: {
			if (status->hitBox.y1 + 1 < status->boundaries.y1) {
				status->hitBox.y0++;
				status->hitBox.y1++;
			}
		} break;
		}

		return true;
	} break;
	}

	return false;
}