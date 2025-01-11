struct UITable {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};

	int32 rowCount;
	int32 columnCount;
	int32 selectedRow;
	int32 selectedColumn;
	int32 activeSplitter;

	float32 grabX;
	bool isResizing;

	float32* rowHeight;

	// TODO: absolute x? Or 0..1?
	float32* columnSplitterX;
};

bool UITableProcessEvent(UITable* table, OSEvent event) {
	switch (event.type) {
	case Event_MouseMove: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;

		if (!InBounds(table->pos, table->dim, cursor)) {
			table->activeSplitter = 0;
			table->selectedColumn = 0;
			table->selectedRow = 0;
			return false;
		}

		if (table->isResizing) {
			float32 x = table->grabX + cursor.x;
			float32 proportion = (x - table->x)/table->width;
			if (proportion < 0.05f) proportion = 0.05f;
			if (proportion > 0.95f) proportion = 0.95f;
			if (table->activeSplitter > 1 
					&& 
				table->columnSplitterX[table->activeSplitter - 2] + 0.05f > proportion) {

				proportion = table->columnSplitterX[table->activeSplitter - 2] + 0.05f;
			}

			if (table->activeSplitter < table->columnCount - 1
					&& 
				table->columnSplitterX[table->activeSplitter] - 0.05f < proportion) {

				proportion = table->columnSplitterX[table->activeSplitter] - 0.05f;
			}

			table->columnSplitterX[table->activeSplitter - 1] = proportion;
			return true;
		}

		table->activeSplitter = 0;
		float32 x0 = table->x;
		for (int32 splitter = 0; splitter < table->columnCount - 1; splitter++) {
			float32 proportion = table->columnSplitterX[splitter];
			float32 x1 = table->x + proportion*table->width;
			if (InBounds({x1 - 2, table->y}, {4, table->height}, cursor)) {
				table->activeSplitter = splitter + 1;

				OSSetCursorIcon(CUR_MOVESIDE);
				return true;
			}

			x0 = x1;
		}

		table->selectedColumn = 0;
		x0 = table->x;
		for (int32 column = 0; column < table->columnCount; column++) {
			float32 proportion = column == table->columnCount - 1 
				? 1 
				: table->columnSplitterX[column];
			float32 x1 = table->x + proportion*table->width;

			if (InBounds({x0, table->y}, {x1 - x0, table->height}, cursor)) {
				table->selectedColumn = column + 1;
				break;
			}

			x0 = x1;
		}

		float32 y0 = table->y;
		for (int32 row = 0; row < table->rowCount; row++) {
			float32 height = table->rowHeight[row];
			if (InBounds({table->x, y0}, {table->width, height}, cursor)) {
				table->selectedRow = row + 1;
				break;
			}

			y0 += height;
		}

		OSSetCursorIcon(CUR_ARROW);
		return true;
	} break;

	case Event_MouseLeftButtonDown: {
		if (table->activeSplitter) {
			table->isResizing = true;
			float32 proportion = table->columnSplitterX[table->activeSplitter - 1];
			float32 x = table->x + proportion*table->width;
			table->grabX = x - event.mouse.cursorPos.x - 0.5f;

			return true;
		}

		return false;
	} break;

	case Event_MouseLeftButtonUp: {
		table->isResizing = false;

		return false;
	} break;
	}

	return false;
}