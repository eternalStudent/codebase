// TODO: allow for cached font

// TODO: wrap/no-wrap is a separate concern than what input is allowed
enum UITextFlags: uint32 {
	UIText_SingleLine = (1 << 1),
	UIText_Wrap       = (1 << 2),
	UIText_DigitsOnly = (1 << 3),
};

struct UITextBox {
	union {
		struct {float32 x, y, width, height;};
		struct {Point2 pos; Dimensions2 dim;};
	};
	Point2 scroll;

	StringList data;
	UITextFlags flags;
};

struct UIText {
	UITextBox* boxes;
	int32 count;

	BakedFont* font;
	Point2 pad;

	UITextBox* active;
	UITextBox* hover;

	bool isSelecting;
	StringListPos head;
	StringListPos tail;
	StringListPos tors;

	FixedSize* allocator;
	BigBuffer* buffer;
};

void CopySelectedTextToClipboard(UIText* text) {
	String str = StringListPosCompare(text->tail, text->head)
		? StringListToTempString(text->tail, text->head, text->buffer)
		: StringListToTempString(text->head, text->tail, text->buffer);

	OSCopyToClipboard(str);
}

void DeleteSelectedText(UIText* text) {
	UITextBox* textbox = text->active;
	StringList* list = &textbox->data;
	text->head = StringListDelete(list, text->tail, text->head, 
		text->allocator, text->buffer);
	text->tail = text->head; 
}

void InsertText(void* data, String newString) {
	UIText* text = (UIText*)data;
	StringList* list = &text->active->data;
	text->head = StringListInsert(list, newString, text->tail, text->head, text->allocator, text->buffer);
	text->tail = text->head; 
}

// TODO: scroll!!!!!!!
bool UITextProcessEvent(UIText* text, OSEvent event) {
	switch (event.type) {

	case Event_MouseMove: {
		Point2 cursor = event.mouse.cursorPos + 0.5f;

		if (text->isSelecting) {

			UITextBox* textbox = text->active;

			if (InBounds(textbox->pos, textbox->dim, cursor)) {
				bool shouldWrap = (textbox->flags & UIText_Wrap) == UIText_Wrap;
				text->head = FontGetCharPos(cursor - textbox->pos, text->font, textbox->data, shouldWrap, textbox->width - text->pad.x);
			}

			OSSetCursorIcon(CUR_TEXT);
			return true;
		}
		else {
			for (int32 i = 0; i < text->count; i++) {
				UITextBox* textbox = text->boxes + i;
				if (InBounds(textbox->pos, textbox->dim, event.mouse.cursorPos + 0.5f)) {

					bool shouldWrap = (textbox->flags & UIText_Wrap) == UIText_Wrap;
					float32 relx = cursor.x - (textbox->x + text->pad.x);
					float32 rely = cursor.y - (textbox->y + text->pad.y);
					
					text->hover = textbox;
					text->tors = FontGetCharPos({relx, rely}, text->font, textbox->data, shouldWrap, textbox->width - text->pad.x);
					OSSetCursorIcon(CUR_TEXT);
					return true;
				}
			}

			text->hover = NULL;
			return false;
		}
	} break;

	case Event_MouseLeftButtonDown : {	
		if (text->hover) {
			text->active = text->hover;
			text->tail = text->tors;
			text->head = text->tors;
			text->isSelecting = true;

			return true;
		}
		else {
			text->active = NULL;

			return false;
		}
	} break;

	case Event_MouseLeftButtonUp : {
		text->isSelecting = false;

		return false;
	} break;

	case Event_MouseDoubleClick: {
		if (text->hover) {
			text->active = text->hover;
			StringListFindWord(text->active->data, text->tors, &text->tail, &text->head);

			return true;
		}
		else {
			text->active = NULL;

			return false;
		}

	} break;

	case Event_MouseTripleClick: {
		if (text->hover) {
			text->active = text->hover;
			text->tail = StringListPosInc(StringListGetLastPosOf(SL_START(text->active->data), text->head, 10));
			text->head = StringListGetFirstPosOf(text->head, SL_END(text->active->data), 10);

			return true;
		}
		else {
			text->active = NULL;

			return false;
		}

	} break;

	case Event_KeyboardPress: {
		if (!text->active) return false;
		UITextBox* textbox = text->active;

		switch (event.keyboard.vkCode) {
		case KEY_LEFT: {
			if (StringListIsStart(text->head))
				return false;

			if (event.keyboard.ctrlIsDown) {
				StringListFindWord(text->active->data, text->head, &text->head, NULL);
				text->tail = text->head;
			}
			else {
				StringListPosDec(&text->head);

				if (!event.keyboard.shiftIsDown)
					text->tail = text->head;
			}

			return true;
		} break;
		case KEY_RIGHT: {
			if (StringListIsEnd(text->head))
				return false;

			if (event.keyboard.ctrlIsDown) {
				StringListFindWord(text->active->data, text->head, NULL, &text->head);
				text->tail = text->head;
			}
			else {
				StringListPosInc(&text->head);

				if (!event.keyboard.shiftIsDown)
					text->tail = text->head;
			}

			return true;
		} break;
		case KEY_UP: {
			if (StringListIsStart(text->head)) {
				return false;
			}

			bool shouldWrap = (textbox->flags & UIText_Wrap) == UIText_Wrap;
			TextMetrics metrics = GetTextMetrics(text->font, textbox->data, text->head, shouldWrap, textbox->width - text->pad.x);
			text->head = FontGetCharPos({metrics.x, metrics.y - 2*text->font->height},
									text->font, textbox->data, 
								  	shouldWrap, textbox->width - text->pad.x);

			if (!event.keyboard.shiftIsDown)
				text->tail = text->head;

			return true;
		} break;
		case KEY_DOWN: {
			if (StringListIsEnd(text->head)) {
				return false;
			}

			bool shouldWrap = (textbox->flags & UIText_Wrap) == UIText_Wrap;
			TextMetrics metrics = GetTextMetrics(text->font, textbox->data, text->head, shouldWrap, textbox->width - text->pad.x);
			text->head = FontGetCharPos({metrics.x, metrics.y},
									text->font, textbox->data, 
								  	shouldWrap, textbox->width - text->pad.x);

			if (!event.keyboard.shiftIsDown)
				text->tail = text->head;

			return true;
		} break;
		case KEY_BACKSPACE: {
			if (StringListIsStart(text->head) && StringListIsStart(text->tail)) {
				return false;
			}
			
			DeleteSelectedText(text);

			return true;
		} break;
		case KEY_DELETE: {
			if (StringListIsEnd(text->head) && StringListIsEnd(text->tail)) {		
				return false;
			}

			if (StringListPosEquals(text->head, text->tail)) {
				StringListPosInc(&text->head);
				text->tail = text->head;
			}

			DeleteSelectedText(text);

			return true;
		} break;
		case KEY_HOME: {
			text->head = event.keyboard.ctrlIsDown
				? SL_START(textbox->data)
				: StringListPosInc(StringListGetLastPosOf(SL_START(textbox->data), text->head, 10));

			if (!event.keyboard.shiftIsDown)
				text->tail = text->head;

			return true;
		} break;
		case KEY_END: {
			text->head = event.keyboard.ctrlIsDown
				? SL_END(textbox->data)
				: StringListGetFirstPosOf(text->head, SL_END(textbox->data), 10);

			if (!event.keyboard.shiftIsDown)
				text->tail = text->head;

			return true;
		} break;
		case KEY_X: {
			if (event.keyboard.ctrlIsDown) {
				CopySelectedTextToClipboard(text);
				DeleteSelectedText(text);

				return true;
			}
		} break;
		case KEY_C: {
			if (event.keyboard.ctrlIsDown) {
				CopySelectedTextToClipboard(text);

				return true;
			}
		} break;
		case KEY_V: {
			if (event.keyboard.ctrlIsDown) {
				OSRequestClipboardData(text, InsertText);

				return true;
			}
		} break;
		}
	} break;

	case Event_KeyboardChar: {
		if (!text->active) return false;
		UITextBox* textbox = text->active;

		byte b = event.keyboard.character;
		if (!IsDigit(b) && (textbox->flags & UIText_DigitsOnly)) {
			// nothing
		}
		else if (b == 10 && (textbox->flags & UIText_SingleLine)) {
			// nothing
		}
		else {
			text->head = StringListInsert(&textbox->data, {&b, 1}, text->tail, text->head, 
				text->allocator, text->buffer);
			text->tail = text->head;
			return true;
		}
	} break;
	}

	return false;
}

void UITextReplaceData(UIText* text, UITextBox* textbox, String data) {
	StringListReplace(&textbox->data, data, text->allocator);
}

uint64 UITextGetUnsigned(UIText* text, UITextBox* textbox) {
	if (textbox->data.length == 0)
		return 0;

	String data = StringListToTempString(textbox->data, text->buffer);
	return ParseUInt64(data);
}
