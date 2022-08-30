#define UI_FLIPY(y)        (ui.windowElement->height-(y)-1)         

#define UI_MOVABLE			(1 << 0)
#define UI_RESIZABLE		(1 << 1)
#define UI_SHUFFLEABLE		(1 << 2)
#define UI_CLICKABLE		(1 << 3)
#define UI_HIDE_OVERFLOW 	(1 << 4)
#define UI_SCROLLABLE		((1 << 5) + UI_HIDE_OVERFLOW)

#define UI_HIDDEN			(1 << 6)
#define UI_CENTER			(1 << 7)
#define UI_RIGHT			(1 << 8)

#define UI_FIT_CONTENT		(1 << 9)

#define UI_ELEVATOR 		(1 << 10)
#define UI_BRING_PARENT_TO_FRONT	(1 << 11)

#include "font.cpp"

struct UIStyle {
	float32 radius;
	uint32 background;
	float32 borderWidth;
	uint32 borderColor;
};

union UIBox {
	struct {int32 x, y, width, height;};
	struct {Point2i pos; Dimensions2i dim;};
};

struct UIText {
	Font* font;
	String string;
	uint32 color;
};

struct UIImage {
	TextureId atlas;
	Box2 crop;
};

#define UI_LEFT_POINTING_TRIANGLE 		1
#define UI_RIGHT_POINTING_TRIANGLE		2
#define UI_UP_POINTING_TRIANGLE 		3
#define UI_DOWN_POINTING_TRIANGLE 		4
#define UI_SQUARE 						5
#define UI_BULLET 						6

struct UISymbol {
	union {
		struct {int32 x0, y0;};
		Point2i pos;
	};
	uint32 color;
	int32 type;
};

struct UIElement {
	union {
		struct {int32 x, y, width, height;};
		struct {Point2i pos; Dimensions2i dim;};
		UIBox box;
	};

	union {
		struct {
			float32 radius;
			uint32 background;
			float32 borderWidth;
			uint32 borderColor;
		};
		UIStyle style;
	};

	// TODO: add cursorIcon?
	String name; // NOTE: for debugging

	uint32 flags;

	void (*onClick)(UIElement*);
	void (*onHover)(UIElement*);
	void (*onResize)(UIElement*);
	void (*onMove)(UIElement*);

	UIElement* parent;
	UIElement* first;
	UIElement* last;
	UIElement* next;
	UIElement* prev;

	UIText text;
	UIImage image;
	UISymbol symbol;
	opaque64 context;

	Point2i scrollPos;
};

struct {
	FixedSize allocator;
	Arena* scratch;

	UIStyle originalStyle;

	UIElement* windowElement;

	UIElement* active;
	Point2i originalPos;
	Point2i grabPos;

	bool isGrabbing;
	bool isResizing;
	bool isSelecting;

	UIElement* selected;
	ssize start;
	ssize end;
} ui;

Box2i GetAbsolutePosition(UIElement* element) {
	if (element == NULL) return {0, 0, ui.windowElement->width, ui.windowElement->height};
	UIElement* parent = element->parent;
	Box2i parentPos = GetAbsolutePosition(parent);
	if (parent == NULL) parent = ui.windowElement;

	return (element->flags & UI_RIGHT)
		? Box2i{parentPos.x1 - element->x - parent->scrollPos.x - element->width, 
				parentPos.y0 + element->y - parent->scrollPos.y, 
				parentPos.x1 - element->x - parent->scrollPos.x,
				parentPos.y0 + element->y - parent->scrollPos.y + element->height}
		: Box2i{parentPos.x0 + element->x - parent->scrollPos.x,
				parentPos.y0 + element->y - parent->scrollPos.y,
				parentPos.x0 + element->x - parent->scrollPos.x + element->width,
				parentPos.y0 + element->y - parent->scrollPos.y + element->height};
}

bool IsInBottomRight(Box2i box, Point2i pos) {
	return box.x1-4 <= pos.x && box.y1-4 <= pos.y;
}

ssize IsInTextBound(UIElement* textElement, Box2i box, Point2i pos) {
	UIText text = textElement->text;
	if (!text.string.length) return -1;
	float32 x_end = (float32)(pos.x - box.x0 + textElement->scrollPos.x);
	float32 y_end = (float32)(pos.y - (box.y0 + text.font->height) + textElement->scrollPos.y);
	StringNode node;
	return GetCharIndex(text.font, CreateStringList(text.string, &node), x_end, y_end);
}

void SetPosition(UIElement* element, int32 x, int32 y) {
	if (!element->parent) {
		element->pos = {x,y};
	}
	else {
		UIElement* parent = element->parent;

		if (x < 0) element->x = 0;
		else if (x+element->width > parent->width) element->x = parent->width-element->width;
		else element->x = x;

		if (y < 0) element->y = 0;
		else if (y+element->height > parent->height) element->y = parent->height-element->height;
		else element->y = y;
	}
}

bool IsAncestorHidden(UIElement* e) {
	while (e) {
		if (e->flags & UI_HIDDEN) return true;
		e = e->parent;
	}
	return false;
}

UIElement* GetElementByPosition(Point2i p) {
	UIElement* element = ui.windowElement;
	while (element->last) element = element->last;

	while (element) {
		if (!IsAncestorHidden(element)) {
			Box2i b = GetAbsolutePosition(element);
			if (INSIDE2(b, p)) break;
		}

		if (element->prev) {
			element = element->prev;
			while (element->last) element = element->last;
		}
		else element = element->parent;
	}

	if (element == ui.windowElement) return NULL;
	return element;
}

Point2i GetRelativePosition(Point2i cursorPos, UIElement* element) {
	Box2i absolute = GetAbsolutePosition(element);
	return {cursorPos.x - absolute.x0, cursorPos.y - absolute.y0};
}

void BringToFront(UIElement* element) {
	if (element->parent->last == element) return;

	UIElement* parent = element->parent;
	UIElement* prev = element->prev;
	UIElement* next = element->next;

	if (parent->first == element) parent->first = next;

	if (prev) prev->next = next;
	next->prev = prev;
	element->prev = parent->last;
	element->next = NULL;
	parent->last->next = element;
	parent->last = element;
}

UIElement* GetScrollableAnscestor(UIElement* e) {
	while (e && !(e->flags & UI_SCROLLABLE))
		e = e->parent;
	return e;
}

Dimensions2i GetContentDim(UIElement* e) {
	int32 width = 0;
	int32 height = 0;
	if (e->text.string.length) {
		StringNode node = {e->text.string, NULL, NULL};
		StringList list = {&node, &node, e->text.string.length};
		TextMetrics metrics = GetTextMetrics(e->text.font, list);
		width = (int32)(metrics.maxx + 0.5f);
		height = (int32)(metrics.endy + 0.5f);
	}
	LINKEDLIST_FOREACH(e, UIElement, child) {
		width  = MAX(child->x + child->width, width);
		height = MAX(child->y + child->height, height);
	}
	return {width, height};
}

void RenderText(UIElement* textElement, Point2i pos) {
	UIText text = textElement->text;
	if (!text.string.length) return;
	String string = text.string;
	Font* font = text.font;

	float32 x = (float32)(pos.x - textElement->scrollPos.x);
	float32 y = (float32)(UI_FLIPY(pos.y + font->height - textElement->scrollPos.y));
	StringNode node = {string, NULL, NULL};
	StringList list = {&node, &node, string.length};
	RenderText(font, list, x, y, text.color);
	
	// Handle selected text
	if (ui.selected && ui.selected == textElement) {
		ssize start = MIN(ui.start, ui.end);
		ssize end = MAX(ui.start, ui.end);
		ASSERT(0 <= start && start <= end && end <= string.length);
		ssize i = 0;
		float32 x0 = x;
		float32 y1 = y + font->height - 3;

		while (true) {
			if (i == start)
				break;
			byte b = string.data[i];
			if (b == 10) {
        		x0 = x;
        		y1 -= font->height;
      		}
      		else if (32 <= b && b <= 127) {
        		BakedChar* bakedchar = font->chardata + (b - 32);
         		x0 += bakedchar->xadvance;
         	}
      		i++;
		}

		float32 x1 = x0;
		while(true) {
			byte b = string.data[i];
			if (b == 10 || i == end) {
				if (start != end)
					GfxDrawBox2({x0, y1 - font->height, x1, y1}, 0x44ffffff);
				if (i == end) 
					break;
				x0 = x;
				x1 = x;
				y1 -= font->height;
			}
			else if (32 <= b && b <= 127) {
        		BakedChar* bakedchar = font->chardata + (b - 32);
         		x1 += bakedchar->xadvance;
         	}
      		i++;
		}
		
		
		if (ui.start < ui.end)
			GfxDrawBox2({x1-1, y1 - font->height-2, x1+1, y1+2}, RGBA_ORANGE);
		else
			GfxDrawBox2({x0-1, y1 - font->height-2, x0+1, y1+2}, RGBA_ORANGE);
	}
}

void RenderImage(UIElement* imageElement, Point2i pos) {
	UIImage image = imageElement->image;
	if (!image.atlas) return;

	Box2 renderBox = Box2{
		(float32)pos.x, 
		(float32)UI_FLIPY(pos.y+imageElement->height), 
		(float32)(pos.x+imageElement->width), 
		(float32)UI_FLIPY(pos.y)};
	GfxDrawImage(image.atlas, image.crop, renderBox);
}

void RenderSymbol(UISymbol symbol, Point2i pos) {
	if (symbol.type == 0) return;

	float32 x0 = (float32)(symbol.pos.x + pos.x);
	float32 y0 = (float32)(symbol.pos.y + pos.y);
	switch (symbol.type) {
		case UI_LEFT_POINTING_TRIANGLE: {
			GfxDrawTriangle(
				{
					{x0, UI_FLIPY(y0 + 5.0f)},
					{x0 + 5.0f, UI_FLIPY(y0)},
					{x0 + 9.0f, UI_FLIPY(y0 + 9.0f)}
				}, symbol.color);
		} break;
		case UI_RIGHT_POINTING_TRIANGLE: {
			GfxDrawTriangle(
				{
					{x0, UI_FLIPY(y0)},
					{x0 + 9.0f, UI_FLIPY(y0 + 5.0f)},
					{x0, UI_FLIPY(y0 + 9.0f)}
				}, symbol.color);
		} break;
		case UI_UP_POINTING_TRIANGLE: {
			GfxDrawTriangle(
				{
					{x0 + 5.0f, UI_FLIPY(y0)},
					{x0, UI_FLIPY(y0 + 9.0f)},
					{x0 + 9.0f, UI_FLIPY(y0 + 9.0f)}
				}, symbol.color);
		} break;
		case UI_DOWN_POINTING_TRIANGLE: {
			GfxDrawTriangle(
				{
					{x0, UI_FLIPY(y0)},
					{x0 + 9.0f, UI_FLIPY(y0)},
					{x0 + 5.0f, UI_FLIPY(y0 + 9.0f)}
				}, symbol.color);
		} break;
		case UI_SQUARE: {
			GfxDrawBox2({x0, UI_FLIPY(y0 + 9.0f), x0 + 9.0f, UI_FLIPY(y0)}, symbol.color);
		} break;
		case UI_BULLET: {
			GfxDrawDisc({{x0, UI_FLIPY(y0 + 5.0f)}, 5.0f}, symbol.color, 0, 360);
		} break;
	}
}

int32 __center(UIElement* element) {
	return (element->parent->width - element->width)/2;
}

void RenderElement(UIElement* element) {
	if (element->flags & UI_HIDDEN) return;
	if (element->flags & UI_CENTER) element->x = __center(element);
	if (element->flags & UI_FIT_CONTENT) element->dim = GetContentDim(element);
	if (element->flags & UI_ELEVATOR) {
		UIElement* scrollBar = element->parent;
		UIElement* pane = scrollBar->prev;
		Dimensions2i contentDim = GetContentDim(pane);

		float32 length = (float32)(pane->height*scrollBar->height)/(float32)contentDim.height;
		float32 pos = (float32)((scrollBar->height - length)*pane->scrollPos.y)/(float32)(contentDim.height - pane->height); 

		element->height = (int32)length;
		element->y = (int32)pos;
	}
	Box2i box = GetAbsolutePosition(element);

	RenderImage(element, box.p0);

	Box2 renderBox = Box2{(float32)box.x0, (float32)UI_FLIPY(box.y1), (float32)box.x1, (float32)UI_FLIPY(box.y0)};
	if (element->radius) {
		GfxDrawBox2Rounded(renderBox, element->radius, element->background);
		if (element->borderWidth && element->borderColor) 
			GfxDrawBox2RoundedLines(renderBox, element->radius, element->borderWidth, element->borderColor);
	}
	else {
		GfxDrawBox2(renderBox, element->background);
		if (element->borderWidth && element->borderColor) 
			GfxDrawBox2Lines(renderBox, element->borderWidth, element->borderColor);
	}

	if (element->flags & UI_HIDE_OVERFLOW) GfxCropScreen(box.x0, UI_FLIPY(box.y1), element->width, element->height);
	LINKEDLIST_FOREACH(element, UIElement, child) RenderElement(child);
	RenderText(element, box.p0);
	RenderSymbol(element->symbol, box.p0);
	if (element->flags & UI_HIDE_OVERFLOW) GfxClearCrop();
}

// API
//-----------

void UIInit(Arena* persist, Arena* scratch) {
	int32 capacity = 200;
	ui.allocator = CreateFixedSize(persist, capacity, sizeof(UIElement));
	ui.scratch = scratch;
}

void UIRenderElements() {
	RenderImage(ui.windowElement, {0, 0});
	LINKEDLIST_FOREACH(ui.windowElement, UIElement, child) RenderElement(child);
	RenderText(ui.windowElement, {0, 0});	
}

void UISetWindowElement(uint32 background) {
	ui.windowElement = (UIElement*)FixedSizeAlloc(&ui.allocator);
	ui.windowElement->pos = {0, 0};
	ui.windowElement->background = background;
	GfxSetColor(background);
}

void UICreateWindow(const char* title, Dimensions2i dimensions, uint32 background) {
	OSCreateWindow(title, dimensions.width, dimensions.height);
	GfxInit(ui.scratch);
	UISetWindowElement(background);
}

void UICreateWindowFullScreen(const char* title, uint32 background) {
	OSCreateWindowFullScreen(title);
	GfxInit(ui.scratch);
	UISetWindowElement(background);
}

UIElement* UICreateElement(UIElement* parent) {
	UIElement* element = (UIElement*)FixedSizeAlloc(&ui.allocator);
	ASSERT(element != NULL);

	if (parent == NULL) parent = ui.windowElement;
	LINKEDLIST_ADD(parent, element);
	element->parent = parent;

	return element;
}

void UIDestroyElement(UIElement* element) {
	LINKEDLIST_REMOVE(element->parent, element);
	LINKEDLIST_FOREACH(element, UIElement, child) UIDestroyElement(child);

	FixedSizeFree(&ui.allocator, element);
}

void UpdateTextScrollPos() {
	UIText text = ui.selected->text;
	int32 elementWidth = ui.selected->width;
	int32 elementHeight = ui.selected->height;
	Font* font = text.font;
	String string = text.string;

	String sub = {string.data, ui.end};
	StringNode node;
	StringList list = CreateStringList(sub, &node);
	TextMetrics metrics = GetTextMetrics(font, list);
	float32 textLineWidth = metrics.endx;
	float32 nextLetter = 0.0f;
	if (ui.end < string.length) {
		byte b = string.data[ui.end + 1];
		if (b == 10) b = 32;
		nextLetter = GetCharWidth(font, string.data[ui.end + 1]);
	}

	int32 posRelativeToElement = (int32)(textLineWidth + nextLetter + 0.5f);
	if (elementWidth < posRelativeToElement - ui.selected->scrollPos.x) {
		ui.selected->scrollPos.x = posRelativeToElement - elementWidth; 
	}

	sub = {string.data, MAX(0, ui.end - 1)};
	list = CreateStringList(sub, &node);
	textLineWidth = GetTextMetrics(font, list).endx;
	posRelativeToElement = (int32)(textLineWidth + 0.5f);
	if (posRelativeToElement < ui.selected->scrollPos.x) {
		ui.selected->scrollPos.x = MAX(posRelativeToElement, 0);
	}

	if (elementHeight < metrics.endy - ui.selected->scrollPos.y) {
		ui.selected->scrollPos.y = (int32)(metrics.endy + 0.5f) - elementHeight;
	}

	if (metrics.endy - font->height < ui.selected->scrollPos.y) {
		ui.selected->scrollPos.y = MAX((int32)(metrics.endy - 1.5f*font->height + 0.5f), 0);
	}
}

UIElement* UIUpdateActiveElement() {
	Point2i cursorPos = OSGetCursorPosition();
	ui.windowElement->dim = OSGetWindowDimensions();
	UIElement* prev = ui.active;
	
	if (!ui.isResizing && !ui.isGrabbing) {
		ui.active = GetElementByPosition(cursorPos);
	}
	if (prev != ui.active) {
		if (prev)       prev->style = ui.originalStyle;
		if (ui.active)  ui.originalStyle = ui.active->style;
	}
	UIElement* element = ui.active;
	Box2i pos = GetAbsolutePosition(element);

	// Handle mouse hover
	bool isInBottomRight = false;
	ssize textIndex = -1;
	if (element) {
		textIndex = IsInTextBound(element, pos, cursorPos);
		if(IsInBottomRight(pos, cursorPos) && (element->flags & UI_RESIZABLE)) {
			OSSetCursorIcon(CUR_RESIZE);
			isInBottomRight = true;
		}
		else if (element->onHover) element->onHover(element);
		else if (element->flags & UI_CLICKABLE) OSSetCursorIcon(CUR_HAND);
		else if (element->flags & UI_MOVABLE) OSSetCursorIcon(CUR_MOVE);
		else if (textIndex != -1) {
			OSSetCursorIcon(CUR_TEXT);
		}
		else OSSetCursorIcon(CUR_ARROW);
	}
	else if (1 < cursorPos.x && cursorPos.x < ui.windowElement->width-1 && 1 < cursorPos.y && cursorPos.y < ui.windowElement->height-1)
		OSSetCursorIcon(CUR_ARROW);

	// Handle mouse pressed
	if (OSIsMousePressed(MOUSE_L)) {
		ui.selected = NULL;
		if (element) {
			if (element->flags & UI_SHUFFLEABLE) BringToFront(element);
			if (element->flags & UI_BRING_PARENT_TO_FRONT) BringToFront(element->parent);
			if (element->flags & UI_CLICKABLE) {
				if (element->onClick)
					element->onClick(element);
			}
			else if (textIndex != -1) {
				ui.end = textIndex;
				ui.start = ui.end;
				ui.selected = element;
				ui.isSelecting = true;
			}
			else if (isInBottomRight && (element->flags & UI_RESIZABLE)) {
				ui.isResizing = true;
				ui.originalPos = element->pos;
			}
			else if (element->flags & UI_MOVABLE) {
				ui.isGrabbing = true;
				ui.grabPos = cursorPos;
				ui.originalPos = element->pos;
			}
		}
	}

	// Handle double click
	if (element && OSIsMouseDoubleClicked() && textIndex != -1) {
		ui.selected = element;
		ui.end = element->text.string.length;
		ui.start = 0;
	}
	
	// Handle mouse released
	if (!OSIsMouseDown(MOUSE_L)) {
		ui.isGrabbing = false;
		ui.isResizing = false;
		ui.isSelecting = false;
	}

	// Handle scrolling
	int32 mouseWheelDelta = OSGetMouseWheelDelta();
	int32 mouseHWheelDelta = OSGetMouseHWheelDelta();
	if (mouseWheelDelta || mouseHWheelDelta) {
		UIElement* scrollable = GetScrollableAnscestor(element);
		if (scrollable) {
			Dimensions2i contentDim = GetContentDim(scrollable);

			if (mouseWheelDelta) {
				scrollable->scrollPos.y -= mouseWheelDelta;
				if (scrollable->scrollPos.y < 0) scrollable->scrollPos.y = 0;
				if (scrollable->scrollPos.y > contentDim.height - scrollable->height)
					scrollable->scrollPos.y = contentDim.height - scrollable->height;
			}

			if (mouseHWheelDelta) {
				scrollable->scrollPos.x += mouseHWheelDelta;
				if (scrollable->scrollPos.x < 0) scrollable->scrollPos.x = 0;
				if (scrollable->scrollPos.x > contentDim.width - scrollable->width)
					scrollable->scrollPos.x = contentDim.width - scrollable->width;
			}
		}
	}

	// Handle grabbing
	if (ui.isGrabbing && 
			(cursorPos.x != ui.grabPos.x || cursorPos.y != ui.grabPos.y)) {
		ASSERT(element);
		int32 newx  = ui.originalPos.x + cursorPos.x - ui.grabPos.x;
		int32 newy = ui.originalPos.y + cursorPos.y - ui.grabPos.y;
		SetPosition(element, newx, newy);
		if (element->onMove) element->onMove(element);
	}

	// Handle resizing
	if (ui.isResizing) {
		ASSERT(element);
		Point2i relativeCursorPos = GetRelativePosition(cursorPos, element->parent);
		int32 x0 = MIN(ui.originalPos.x, relativeCursorPos.x);
		int32 y0 = MIN(ui.originalPos.y, relativeCursorPos.y);
		int32 x1 = MAX(ui.originalPos.x, relativeCursorPos.x);
		int32 y1 = MAX(ui.originalPos.y, relativeCursorPos.y);

		element->x = MAX(x0, 0);
		element->y = MAX(y0, 0);
		element->width = MIN(x1 - element->x, element->parent->width - element->x);
		element->height = MIN(y1 - element->y, element->parent->height - element->y);

		if (element->onResize) element->onResize(element);
	}

	// Handle text selection
	if (ui.isSelecting) {
		static int32 selectionCount = 0;
		if (element && element->text.string.length) {
			selectionCount = 0;
			ASSERT(element == ui.selected && textIndex != -1);
			ui.end = textIndex;
		}
		else {
			if (selectionCount == 0) {
				selectionCount = 6;
				Box2i selectedPos = GetAbsolutePosition(ui.selected);
				if (cursorPos.x < selectedPos.x1) {
					ui.end = MAX(ui.end - 1, 0);
					UpdateTextScrollPos();
				}
				if (selectedPos.x1 < cursorPos.x) {
					if (ui.end < ui.selected->text.string.length && ui.selected->text.string.data[ui.end + 1] != 10) {
						ui.end++;
						UpdateTextScrollPos();
					}
				}
				// TODO: handle ups and downs
			}
			else selectionCount--;
		}
	}

	// Handle arrow keys
	static int32 leftKeyCount = 0;
	if (OSIsKeyPressed(KEY_LEFT) || OSIsKeyDown(KEY_LEFT)) {
		if (leftKeyCount == 0) {
			leftKeyCount = 6;
			if (ui.selected) {
				ui.end = MAX(0, ui.end - 1);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.start = ui.end;
				UpdateTextScrollPos();
			}
		}
		else leftKeyCount--;
	}
	static int32 rightKeyCount = 0;
	if (OSIsKeyDown(KEY_RIGHT)) {
		if (OSIsKeyPressed(KEY_RIGHT) || rightKeyCount == 0) {
			rightKeyCount = 6;
			if (ui.selected) {
				String string = ui.selected->text.string;
				ui.end = MIN(ui.end + 1, string.length);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.start = ui.end;
				UpdateTextScrollPos();
			}
		}
		else rightKeyCount--;
	}
	static int32 upKeyCount = 0;
	if (OSIsKeyDown(KEY_UP)) {
		if (OSIsKeyPressed(KEY_UP) || upKeyCount == 0) {
			upKeyCount = 6;
			if (ui.selected) {
				String string = ui.selected->text.string;
				Font* font = ui.selected->text.font;
				StringNode node;
				StringList list = CreateStringList({string.data, ui.end}, &node);
				TextMetrics metrics = GetTextMetrics(font, list);
				ssize end = GetCharIndex(font, list, metrics.endx, metrics.endy - 2.5f*font->height);
				if (end != -1) ui.end = end;

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.start = ui.end;
				UpdateTextScrollPos();
			}
		}
		else upKeyCount--;
	}
	static int32 downKeyCount = 0;
	if (OSIsKeyDown(KEY_DOWN)) {
		if (OSIsKeyPressed(KEY_DOWN) || downKeyCount == 0) {
			downKeyCount = 6;
			if (ui.selected) {
				String string = ui.selected->text.string;
				Font* font = ui.selected->text.font;
				StringNode node;
				StringList list = CreateStringList({string.data, ui.end}, &node);
				TextMetrics metrics = GetTextMetrics(font, list);
				ssize end = GetCharIndex(font, list, metrics.endx, metrics.endy - 0.5f*font->height);
				if (end != -1) ui.end = end;

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.start = ui.end;
				UpdateTextScrollPos();
			}
		}
		else downKeyCount--;
	}

	// Handle copy
	if (OSIsKeyDown(KEY_CTRL) && OSIsKeyPressed(KEY_C) && ui.selected) {
		String string = ui.selected->text.string;
		ssize start = MIN(ui.start, ui.end);
		ssize end = MAX(ui.start, ui.end);
		String sub = {string.data + start, end - start};
		OSCopyToClipboard(sub);
	}

	return element;
}

void UIDrawLine(Point2i p0, Point2i p1, uint32 rgba, float32 lineWidth) {
	Point2 points[2] = {{(float32)p0.x, (float32)UI_FLIPY(p0.y)}, {(float32)p1.x, (float32)UI_FLIPY(p1.y)}};
	Line2 line = {points, 2};
	GfxDrawLine(line, lineWidth, rgba);
}

Box2i UIGetAbsolutePosition(UIElement* element) {
	return GetAbsolutePosition(element);
}

// specific elements
//-------------------

UIBox __pad(UIElement* parent, int32 l, int32 r, int32 t, int32 b) {
	return UIBox{l, t, parent->width - (l+r), parent->height - (t+b)};
}

UIBox __pad(UIElement* parent, int32 pad) {
	return __pad(parent, pad, pad, pad, pad);
}

void __toggle(UIElement* e) {
	byte* context = (byte*)e->context.p;
	*context = !(*context);
	e->background = *context ? RGBA_DARKGREY : 0;
	ui.originalStyle = e->style;
}

UIElement* UICreateCheckbox(UIElement* parent, UIText text, byte* context) {
	UIElement* container = UICreateElement(parent);
	container->flags = UI_FIT_CONTENT;
	container->height = 24;
	container->name = STR("container");

	UIElement* checkbox = UICreateElement(container);
	checkbox->dim = {24, 24};
	checkbox->background = RGBA_LIGHTGREY;
	checkbox->radius = 6;
	checkbox->name = STR("checkbox");

	UIElement* check = UICreateElement(checkbox);
	check->box = __pad(checkbox, 4);
	check->flags = UI_CLICKABLE;
	check->radius = 3;
	check->onClick = __toggle;
	check->context.p = context;
	check->name = STR("check");

	UIElement* textElement = UICreateElement(container);
	textElement->pos = {37, -4};
	textElement->text = text;

	return container;
}

void __move_sideway(UIElement* e) {
	OSSetCursorIcon(CUR_MOVESIDE);
}

UIElement* UICreateSlider(UIElement* parent, int32 width) {
	UIElement* slider = UICreateElement(parent);
	slider->dim = {width, 16};
	slider->background = RGBA_LIGHTGREY;
	slider->name = STR("slider");

	UIElement* sled = UICreateElement(slider);
	sled->pos = {(width - 8)/2, 0};
	sled->dim = {8, 16};
	sled->radius = 4;
	sled->background = RGBA_WHITE;
	sled->flags = UI_MOVABLE;
	sled->onHover = __move_sideway;
	sled->name = STR("sled");

	return slider;
}

void __hover1(UIElement* e) {
	e->background = RGBA_GREY;
	OSSetCursorIcon(CUR_HAND);
}

void __hover2(UIElement* e) {
	e->background = RGBA_LIGHTGREY;
	OSSetCursorIcon(CUR_HAND);
}

UIElement* UICreateButton(UIElement* parent, Dimensions2i dim) {
	UIElement* button = UICreateElement(parent);
	button->dim = dim;
	button->background = RGBA_LIGHTGREY;
	button->radius = 12.0f;
	button->borderWidth = 1;
	button->borderColor = RGBA_WHITE;
	button->onHover = __hover1;
	button->flags = UI_CLICKABLE;
	button->name = STR("button");
	return button;
}

void __switch(UIElement* e) {
	UIElement* parent = e->parent;
	LINKEDLIST_FOREACH(parent, UIElement, child) {
		child->background = RGBA_LIGHTGREY;
		child->onHover = __hover1;
	}
	e->background = RGBA_DARKGREY;
	e->onHover = NULL;
	ui.originalStyle = e->style;
}

UIElement* UICreateTabControl(UIElement* parent, Dimensions2i dim) {
	UIElement* control = UICreateElement(parent);
	control->dim = dim;
	control->background = RGBA_LIGHTGREY;
	control->flags = UI_MOVABLE | UI_RESIZABLE | UI_FIT_CONTENT;
	control->borderColor = RGBA_WHITE;
	control->borderWidth = 1;
	control->name = STR("tab control");

	return control;
}

UIElement* UICreateTab(UIElement* parent, Dimensions2i dim, String title, Font* font) {
	UIElement* last = parent->last;

	UIElement* header = UICreateElement(parent);
	header->x = last ? last->x+last->width+1 : 24;
	header->dim = dim;
	header->radius = 3;
	header->flags = UI_CLICKABLE | UI_SHUFFLEABLE;
	header->onHover = __hover1;
	header->onClick = __switch;
	header->background = RGBA_LIGHTGREY;
	header->name = STR("tab header");

	UIElement* textElement = UICreateElement(header);
    textElement->text.string = title;
    textElement->x = 3;
    textElement->text.font = font;

    UIElement* body = UICreateElement(header);
    body->x = -header->x;
    body->y = dim.height;
    body->height = parent->height - (dim.height+1);
    body->width = parent->width-2;
    body->background = RGBA_DARKGREY;
    body->name = STR("tab body");

    return body;
}

void UISetActiveTab(UIElement* active) {
	BringToFront(active->parent);
	__switch(active->parent);
}

void __scroll(UIElement* e) {
	OSSetCursorIcon(CUR_ARROW);
	UIElement* scrollBar = e->parent;
	UIElement* pane = scrollBar->prev;
	Dimensions2i contentDim = GetContentDim(pane);
	float32 y = (float32)((contentDim.height - pane->height)*e->y)/(float32)(scrollBar->height - e->height); 
	pane->scrollPos.y = (int32)y;
}

void __arrow(UIElement* e) {
	OSSetCursorIcon(CUR_ARROW);
}

UIElement* UICreateScrollingPane(UIElement* parent, Dimensions2i dim, Point2i pos) {
	UIElement* container = UICreateElement(parent);
	container->dim = {dim.width + 15, dim.height};
	container->pos = pos;
	container->name = STR("scrolling pane container");

	UIElement* pane = UICreateElement(container);
	pane->dim = dim;
	pane->flags = UI_SCROLLABLE;
	pane->name = STR("scrolling pane");

	UIElement* scrollBar = UICreateElement(container);
	scrollBar->x = dim.width + 4;
	scrollBar->y = 6;
	scrollBar->dim = {7, dim.height - 12};
	scrollBar->background = 0x33ffffff;
	scrollBar->radius = 3.0f;
	scrollBar->name = STR("scroll bar");

	UIElement* elevator = UICreateElement(scrollBar);
	elevator->dim = {7, 14};
	elevator->flags = UI_MOVABLE | UI_ELEVATOR;
	elevator->onHover = __arrow;
	elevator->onMove = __scroll;
	elevator->background = 0x33ffffff;
	elevator->radius = 3.0f;
	elevator->name = STR("elevator");

	return pane;
}

void __display_or_hide(UIElement* e) {
	UIElement* element = e->next;
	if (element->flags & UI_HIDDEN) {
		element->flags &= ~UI_HIDDEN;
		e->symbol.type = 3;
	}
	else {
		element->flags |= UI_HIDDEN;
		e->symbol.type = 4;
	}
}

UIElement* UICreateDropdown(UIElement* parent, Dimensions2i dim, Point2i pos, UIText text) {
	UIElement* container = UICreateElement(parent);
	container->dim = dim;
	container->pos = pos;
	container->name = STR("dropdown container");

	UIElement* dropdown = UICreateElement(container);
	dropdown->dim = dim;
	dropdown->flags = UI_CLICKABLE | UI_BRING_PARENT_TO_FRONT;
	dropdown->onClick = __display_or_hide;
	dropdown->background = RGBA_GREY;
	dropdown->name = STR("dropdown");
	dropdown->symbol.type = 4;
	dropdown->symbol.pos = {dim.width + 3, 15};
	dropdown->symbol.color = RGBA_WHITE;

	UIElement* textElement = UICreateElement(dropdown);
	textElement->x = 5;
	textElement->text = text;

	UIElement* hidden = UICreateScrollingPane(container, {dim.width, 100}, {0, dim.height});
	hidden->parent->flags |= UI_HIDDEN;
	hidden->background = RGBA_GREY;

	return dropdown;
}

void __select(UIElement* e) {
	UIElement* list = e->parent;
	UIElement* listContainer = list->parent;
	UIElement* dropdown = listContainer->parent;
	UIElement* visible = dropdown->first;

	visible->first->text.string = e->first->text.string;

	listContainer->flags |= UI_HIDDEN;
	visible->symbol.type = 4;
}

UIElement* UIAddDropdownItem(UIElement* dropdown, UIText text) {
	UIElement* list = dropdown->next->first;
	UIElement* last = list->last;

	UIElement* item = UICreateElement(list);
	item->dim = dropdown->dim;
	if (last) item->y = last->y + last->height;
	item->flags = UI_CLICKABLE;
	item->onClick = __select;
	item->onHover = __hover2;

	UIElement* textElement = UICreateElement(item);
	textElement->x = 5;
	textElement->text = text;

	return item;
}

void __expnad_or_collapse(UIElement* e) {
	int32 height = 0;
	UIElement* children = e->first;
	if (e->parent != ui.windowElement) {
		LINKEDLIST_FOREACH(children, UIElement, child) height += child->height;
	}
	if (e->symbol.type == 2) {
		e->first->flags &= ~UI_HIDDEN;
		e->symbol.type = 4;
		if (e->parent != ui.windowElement) {
			e = e->next;
			while (e) {
				e->y += height;
				e = e-> next;
			}
		}
	}
	else if (e->symbol.type == 4) {
		e->first->flags |= UI_HIDDEN;
		e->symbol.type = 2;
		if (e->parent != ui.windowElement) {
			e = e->next;
			while (e) {
				e->y -= height;
				e = e-> next;
			}
		}
	}
}

UIElement* UICreateTreeItem(UIElement* parent, Dimensions2i dim) {
	UIElement* item;
	if (parent) {
		UIElement* last = parent->first->last;
		item = UICreateElement(parent->first);
		item->x = parent->x + 12;
		item->y = last ? last->y + last->height : parent->height;
		UIElement* e = parent->next;
		while (e) {
			e->y += dim.height;
			e = e->next;
		}
		parent->symbol.type = 4;
	}
	else {
		item = UICreateElement(NULL);
	}
	item->symbol = {{3, 9}, RGBA_WHITE, 0};
	item->dim = dim;
	item->flags = UI_CLICKABLE;
	item->onHover = __arrow;
	item->onClick = __expnad_or_collapse;

	UIElement* children = UICreateElement(item);
	children->name = STR("children");

	return item;
}