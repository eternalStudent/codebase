enum UIPanelState {
	UIPS_Rest,
	UIPS_Grab,
	UIPS_ResizeHover,
	UIPS_Resize,
};

enum UIPanelDock {
	UIPD_Undocked,
	UIPD_Left,
	UIPD_Right,
	UIPD_Top,
	UIPD_Bottom,
};

enum UIPanelHover {
	UIPH_None,
	UIPH_Left,
	UIPH_Right,
	UIPH_Top,
	UIPH_Bottom,
	UIPH_BottomRight,
	UIPH_Header,
	UIPH_Closing,
};

struct UIPanel {
	Box2i header;
	Box2i borders;
	Box2i closing;
	Box2i docks[4];

	UIPanelState state;
	UIPanelHover hover;
	UIPanelDock dock;

	Point2i grabPos;
	Dimensions2i min;
};

bool UIPanelProcessEvent(OSEvent event, UIPanel* panel) {
	Point2i cursor = event.mouse.cursorPos;

	switch (event.type) {
	case Event_MouseMove: {
		if (panel->state == UIPS_Grab) {
			if (cursor.x != panel->grabPos.x || cursor.y != panel->grabPos.y) {

				int32 x = panel->grabPos.x + cursor.x;
				int32 y = panel->grabPos.y + cursor.y;

				int32 width = panel->borders.x1 - panel->borders.x0;
				int32 height = panel->borders.y1 - panel->borders.y0;

				panel->borders = {x, y, x + width, y + height};
			}

			panel->dock = UIPD_Undocked;
			for (int32 i = 0; i < 4; i++) {
				if (InBounds(panel->docks[i], cursor)) {
					panel->dock = (UIPanelDock)(i + 1);
					break;
				}
			}

			return true;
		}
		else if (panel->state == UIPS_Resize) {

			int32 x = panel->grabPos.x + cursor.x;
			int32 y = panel->grabPos.y + cursor.y;

			if (panel->hover == UIPH_BottomRight) {
				if (x - panel->borders.x0 < panel->min.width) {
					panel->borders.x1 = panel->borders.x0 + panel->min.width;
				}
				else {
					panel->borders.x1 = x;
				}

				if (y - panel->borders.y0 < panel->min.height) {
					panel->borders.y1 = panel->borders.y0 + panel->min.height;
				}
				else {
					panel->borders.y1 = y;
				}
			}
			else if (panel->hover == UIPH_Right) {
				if (x - panel->borders.x0 < panel->min.width) {
					panel->borders.x1 = panel->borders.x0 + panel->min.width;
				}
				else {
					panel->borders.x1 = x;
				}
			}
			else if (panel->hover == UIPH_Bottom) {
				if (y - panel->borders.y0 < panel->min.height) {
					panel->borders.y1 = panel->borders.y0 + panel->min.height;
				}
				else {
					panel->borders.y1 = y;
				}
			}
			else if (panel->hover == UIPH_Top) {
				if (panel->borders.y1 - y < panel->min.height) {
					panel->borders.y0 = panel->borders.y1 - panel->min.height;
				}
				else {
					panel->borders.y0 = y;
				}
			}
			else if (panel->hover == UIPH_Left) {
				if (panel->borders.x1 - x < panel->min.width) {
					panel->borders.x0 = panel->borders.x1 - panel->min.width;
				}
				else {
					panel->borders.x0 = x;
				}
			}

			return true;

		}
		else if (InBounds({panel->borders.x1 - 1, panel->borders.y1 - 1, panel->borders.x1 + 4, panel->borders.y1 + 4}, cursor)) {
			panel->state = UIPS_ResizeHover;
			panel->hover = UIPH_BottomRight;

			return true;
		}
		else if (InBounds({panel->borders.x0, panel->borders.y1 - 1, panel->borders.x1, panel->borders.y1 + 4}, cursor)) {
			panel->state = UIPS_ResizeHover;
			panel->hover = UIPH_Bottom;

			return true;
		}
		else if (InBounds({panel->borders.x1 - 1, panel->borders.y0, panel->borders.x1 + 4, panel->borders.y1}, cursor)) {
			panel->state = UIPS_ResizeHover;
			panel->hover = UIPH_Right;

			return true;
		}
		else if (InBounds({panel->borders.x0, panel->borders.y0 - 4, panel->borders.x1, panel->borders.y0 + 1}, cursor)) {
			panel->state = UIPS_ResizeHover;
			panel->hover = UIPH_Top;

			return true;
		}
		else if (InBounds({panel->borders.x0 - 4, panel->borders.y0, panel->borders.x0 + 1, panel->borders.y1}, cursor)) {
			panel->state = UIPS_ResizeHover;
			panel->hover = UIPH_Left;

			return true;
		}
		else if (InBounds(panel->header, cursor)) {
			panel->hover = UIPH_Header;

			return true;
		}
		else if (InBounds(panel->closing, cursor)) {
			panel->hover = UIPH_Closing;

			return true;
		}
		else {
			panel->hover = UIPH_None;

			return false;
		}
	} break;
	case Event_MouseLeftButtonDown: {
		if (panel->hover == UIPH_Header) {
			panel->state = UIPS_Grab;
			panel->grabPos = panel->borders.p0 - cursor;
			return true;
		}
		else if (panel->state == UIPS_ResizeHover) {
			panel->state = UIPS_Resize;
			
			if (panel->hover == UIPH_Left || panel->hover == UIPH_Top)
				panel->grabPos = panel->borders.p0 - cursor;
			else
				panel->grabPos = panel->borders.p1 - cursor;

			return true;
		}
	} break;
	case Event_MouseLeftButtonUp: {
		panel->state = UIPS_Rest;

		return false;
	} break;
	}

	return false;
}