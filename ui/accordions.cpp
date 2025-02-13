// NOTE: for 0 or 1 expanded items

struct UIAccordion {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};
	float32 rowHeight;

	int32 count;
	int32 expanded;
	int32 hovered;
};

bool UIAccordionProcessEvent(UIAccordion* accordion, OSEvent event) {
	switch (event.type) {
	case Event_MouseMove: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;

		if (!InBounds(accordion->pos, accordion->dim, cursor)) {
			accordion->hovered = 0;
			return false;
		}

		float32 y = cursor.y - accordion->y;
		int32 index = (int32)(y / accordion->rowHeight);
		if (index + 1 <= accordion->expanded) {
			accordion->hovered = index + 1;
		}
		else {
			y -= accordion->height + (accordion->expanded - accordion->count)*accordion->rowHeight;
			if (y < 0)
				return false;
			
			accordion->hovered = accordion->expanded + (int32)(y / accordion->rowHeight) + 1;
		}

		OSSetCursorIcon(CUR_HAND);
		return true;
	} break;
	case Event_MouseLeftButtonDown: {
		if (accordion->hovered) {
			if (accordion->expanded == accordion->hovered) {
				accordion->expanded = 0;
			}
			else {
				accordion->expanded = accordion->hovered;
			}

			return true;
		}

		return false;
	} break;
	}

	return false;
}