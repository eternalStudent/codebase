/*
 *  NOTE: for contextmenu, autocomplete, combobox and dropdown
 */

struct UIList {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};
	float32 rowHeight;
	float32 scrollOffset;

	int32 count;
	int32 selected;

	// NOTE: this is the selected when using dropdown
	// TODO: maybe come up with better names
	int32 former;

	float32 grabPos;
	bool isDragging;
	bool isVisible;
};

struct UIDropDown {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};
	UIList options;
};

bool UIListProcessEvent(UIList* list, OSEvent event) {
	if (!list->isVisible) return false;

	switch (event.type) {
	case Event_MouseMove : {
		Point2 cursor = event.mouse.cursorPos + 0.5f;

		if (list->isDragging) {
			float32 thumbOffset = GetThumbOffset(list->count*list->rowHeight, list->height, list->scrollOffset);
			float32 newThumbOffset = list->grabPos + cursor.y;

			if (newThumbOffset != thumbOffset) {
				float32 maxScrollOffset = GetMaxScrollOffset(list->count*list->rowHeight, list->height);
				float32 scrollOffset = GetScrollOffset(list->count*list->rowHeight, list->height, newThumbOffset);
				if (scrollOffset < 0) 
					scrollOffset = 0;
				if (scrollOffset > maxScrollOffset)
					scrollOffset = maxScrollOffset;
				list->scrollOffset = scrollOffset;
			}

			OSSetCursorIcon(CUR_ARROW);
			return true;
		}

		else if (InBounds(list->pos, list->dim, cursor)) {
			// TODO: don't need a for loop
			for (int32 i = 0; i < list->count; i++) {
				float32 y1 = list->y - list->scrollOffset + list->rowHeight*(i + 1);
				if (cursor.y < y1) {
					list->selected = i;
					break;
				}
			}

			OSSetCursorIcon(CUR_ARROW);
			return true;
		}

		return false;
	} break;
	case Event_MouseLeftButtonDown: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;

		// TODO: both the thumbWidth and the thumbxOffset should not be hard coded
		const float32 thumbWidth = 7;
		const float32 thumbxOffset = 4;
		
		float32 thumbHeight, thumbyOffset;
		GetThumbDimOffset(list->count*list->rowHeight, list->height, list->scrollOffset, &thumbHeight, &thumbyOffset);
		if (InBounds({list->x + list->width + thumbxOffset, list->y + thumbyOffset}, {thumbWidth, thumbHeight}, cursor)) {
			list->isDragging = true;
			list->grabPos = GetThumbOffset(list->count*list->rowHeight, list->height, list->scrollOffset) - cursor.y;

			return true;
		}

		if (InBounds(list->pos, list->dim, cursor)) {
			list->former = list->selected;
			list->isVisible = false;

			return false;
		}
	} break;
	case Event_MouseLeftButtonUp: {
		list->isDragging = false;

		return false;
	} break;
	case Event_MouseVerticalWheel: {
		if (InBounds(list->pos, list->dim, event.mouse.cursorPos + 0.5f)) {
			float32 maxScrollOffset = GetMaxScrollOffset(list->count*list->rowHeight, list->height);
			float32 scrollOffset = list->scrollOffset - event.mouse.wheelDelta;
			if (scrollOffset < 0)
				scrollOffset = 0;
			if (scrollOffset > maxScrollOffset)
				scrollOffset = maxScrollOffset;
			list->scrollOffset = scrollOffset;

			return true;
		}
	} break;
	case Event_KeyboardPress: {
		switch (event.keyboard.vkCode) {
		case KEY_UP: {
			if (list->selected) {
				list->selected--;

				float32 maxScroll = list->selected*list->rowHeight;
				if (maxScroll < list->scrollOffset)
					list->scrollOffset = maxScroll;

				float32 minScroll = (list->selected - (int32)(list->height/list->rowHeight) + 1)*list->rowHeight;
				if (0 <= minScroll && list->scrollOffset < minScroll)
					list->scrollOffset = minScroll;
			}

			return true;
		} break;
		case KEY_DOWN: {
			if (list->selected + 1 < list->count) {
				list->selected++;

				float32 maxScroll = list->selected*list->rowHeight;
				if (maxScroll < list->scrollOffset)
					list->scrollOffset = maxScroll;

				float32 minScroll = (list->selected - (int32)(list->height/list->rowHeight) + 1)*list->rowHeight;
				if (0 <= minScroll && list->scrollOffset < minScroll)
					list->scrollOffset = minScroll;
			}
			return true;
		} break;
		case KEY_ENTER: {
			list->former = list->selected;
			list->isVisible = false;

			return false;
		} break;
		}
	} break;
	}

	return false;
}

bool UIDropDownProcessEvent(UIDropDown* dropdown, OSEvent event) {
	if (event.type == Event_MouseLeftButtonDown) {

		Point2 cursor = event.mouse.cursorPos + 0.5f;
		if (InBounds(dropdown->pos, dropdown->dim, cursor)) {
			dropdown->options.isVisible = !dropdown->options.isVisible;

			return true;
		}
		
		if (dropdown->options.isVisible 
					&& 
			!InBounds(dropdown->options.pos, dropdown->options.dim, cursor)) {

			dropdown->options.isVisible = false;

			return false;
		}
	}

	return UIListProcessEvent(&dropdown->options, event);
}