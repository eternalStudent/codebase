struct UIList {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};
	float32 rowHeight;
	float32 scrollOffset;

	int32 count;
	int32 selected;

	float32 grabPos;
	bool isDragging;
	bool isVisible;
};

bool UIListProcessEvent(UIList* list, OSEvent event) {
	if (!list->isVisible) return false;

	Point2 cursor = {event.mouse.cursorPos.x + 0.5f, event.mouse.cursorPos.y + 0.5f};

	switch (event.type) {
	case Event_MouseMove : {
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

			return true;
		}

		return false;
	} break;
	case Event_MouseLeftButtonDown: {
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
	} break;
	case Event_MouseLeftButtonUp: {
		list->isDragging = false;

		return false;
	} break;
	case Event_MouseVerticalWheel: {
		if (InBounds(list->pos, list->dim, cursor)) {
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
		}
	} break;
	}

	return false;
}