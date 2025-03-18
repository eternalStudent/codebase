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

bool UIAccordionInExpanded(UIAccordion* accordion, Point2 cursor) {
	if (accordion->expanded == 0)
		return false;

	if (cursor.x < accordion->x || cursor.x >= accordion->x + accordion->width)
		return false;

	float32 expandedHeight = accordion->height - accordion->count*accordion->rowHeight;
	float32 expandedY = accordion->y + accordion->expanded*accordion->rowHeight;

	return expandedY <= cursor.y && cursor.y < expandedY + expandedHeight;
}

Box2 UIAccordionGetExpandedBox(UIAccordion* accordion) {
	if (accordion->expanded == 0)
		return {};

	float32 expandedHeight = accordion->height - accordion->count*accordion->rowHeight;
	float32 expandedY = accordion->y + accordion->expanded*accordion->rowHeight;

	return {accordion->x, expandedY, accordion->x + accordion->width, expandedY + expandedHeight};
}

Box2 UIAccordionGetHoveredBox(UIAccordion* accordion) {
	if (accordion->hovered == 0)
		return {};

	float32 x0 = accordion->x;
	float32 x1 = accordion->x + accordion->width;
	float32 y0 = accordion->hovered <= accordion->expanded || accordion->expanded == 0
		? accordion->y + (accordion->hovered - 1)*accordion->rowHeight
		: accordion->y + accordion->height + (accordion->hovered - 1 - accordion->count)*accordion->rowHeight;
	float32 y1 = y0 + accordion->rowHeight;

	return {x0, y0, x1, y1};
}

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