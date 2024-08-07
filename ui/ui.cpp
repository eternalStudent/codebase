/*
 * NOTE:
 * A lot of work has been done here to support 
 * all kinds of UI widget and behaviors.
 * But this design turned out to not be as flexible as I wanted it to be,
 * so now I mostly view this files as a reference, rather than
 * including it as-is in my projects.
 *
 * I am still experimenting on how to write a ui layer that is:
 *    a) not a framework, but a library, and
 *    b) help me be more productive, by removing the need to rewrite boilerplate code.
 */


/*
 * TODO:
 * ctrl+left/right
 * multi/single line text
 * drop-down, menu-bar, list-box, combo-box
 * tooltip, context-menu
 * theme
 */

#include "graphicsui.cpp"
#include "font.cpp"
#include "icons.cpp"

#define UI_X_MOVABLE		 (1ull << 0)
#define UI_Y_MOVABLE		 (1ull << 1)
#define UI_MOVABLE			 (UI_X_MOVABLE | UI_Y_MOVABLE)
#define UI_RESIZABLE		 (1ull << 2)
#define UI_SHUFFLEABLE		 (1ull << 3)
#define UI_CLICKABLE		 (1ull << 4)
#define UI_HIDE_OVERFLOW	 (1ull << 5)
#define UI_XSCROLLABLE		((1ull << 6) | UI_HIDE_OVERFLOW)
#define UI_YSCROLLABLE		((1ull << 7) | UI_HIDE_OVERFLOW)
#define UI_XYSCROLLABLE		 (UI_XSCROLLABLE | UI_YSCROLLABLE)
#define UI_CANVAS			 (1ull << 8)

#define UI_CENTER			 (1ull << 10)
#define UI_MIDDLE			 (1ull << 11)
#define UI_HORIZONTAL_STACK	 (1ull << 12)
#define UI_VERTICAL_STACK	 (1ull << 13)
#define UI_FIT_CONTENT		 (1ull << 14)
#define UI_MIN_CONTENT		 (1ull << 15)
#define UI_X_THUMB			 (1ull << 16)
#define UI_Y_THUMB			 (1ull << 17)

// TODO: maybe hidden shouldn't be a flag? 
// Same as isChecked/isSelected/isActive?
#define UI_HIDDEN			 (1ull << 20)

#define UI_GRADIENT 		 (1ull << 32)
#define UI_HOVER_STYLE		 (1ull << 33)
#define UI_ACTIVE_STYLE 	 (1ull << 34)
#define UI_CURSOR 			 (1ull << 35)
#define UI_TRANSITION		 (1ull << 36)


#define TEXT_SELECTABLE 	 (1 << 0)
#define TEXT_EDITABLE		((1 << 1) | TEXT_SELECTABLE)
#define TEXT_MULTILINE		((1 << 2) | TEXT_EDITABLE)
#define TEXT_WRAPPING		 (1 << 3)

struct UIStyle {
	Color background;
	float32 radius;
	float32 borderWidth;
	Color borderColor;
	Color color2;
};

struct UIText {
	BakedFont* font;
	Color color;
	String string;
	uint32 flags;
	StringList list;
};

struct UIImage {
	TextureId atlas;
	Box2 crop;
};

struct UIElement {
	String name;

	union {
		union {
			struct {Point2 pos; Dimensions2 dim;};
			struct {float32 x, y, width, height;};
		};
	};

	Point2 scroll;

	float32 transition;
	Point2 prevPos;

	union {
		UIStyle style;
		struct {
			Color background;
			float32 radius;
			float32 borderWidth;
			Color borderColor;
			Color color2;
		};
	};

	uint64 flags;

	UIStyle hover;
	UIStyle active;
	int32 cursor;
	float32 margin;

	union {	
		bool isChecked;
		bool isSelected;
		bool isActive;
	};

	UIText text;
	byte icon;
	UIImage image;

	opaque64 context;
	void (*onClick)(UIElement*);
	void (*onMove)(UIElement*);

	UIElement* xscrollBar;
	UIElement* yscrollBar;

	UIElement* parent;
	UIElement* first;
	UIElement* last;
	UIElement* prev;
	UIElement* next;
};

struct {
	FixedSize allocator;
	FixedSize stringNodeAllocator;
	StringBuilder text;
	BakedFont iconsFont;

	UIElement* rootElement;

	UIElement* hovered;
	UIElement* focused;
	UIElement* selected;

	Point2 originalPos;
	Point2 grabPos;

	bool isGrabbing;
	bool isResizing;
	bool isSelecting;
	bool rootElementIsWindow;

	int32 selectionCount;
	int32 keyCount;

	StringListPos tail;
	StringListPos head;

	float32 zoomScale;
	Point2 zoomOffset;
} ui;

//---------------------------

bool HasQuad(UIElement* e) {
	return (e->background.a) || (e->borderWidth && e->borderColor.a) || (e == ui.focused);
}

bool HasText(UIElement* e) {
	return e->text.list.length || e->text.flags;
}

bool HasImage(UIElement* e) {
	return e->image.atlas != 0;
}

bool HasSelectableText(UIElement* e) {
	return HasText(e) && HAS_FLAGS(e->text.flags, TEXT_SELECTABLE);
}

bool HasEditableText(UIElement* e) {
	return HasText(e) && HAS_FLAGS(e->text.flags, TEXT_EDITABLE);
}

bool IsInteractable(UIElement* e) {
	return HAS_FLAGS(e->flags, UI_CLICKABLE | UI_MOVABLE) || e->onClick || HasSelectableText(e);
}

bool IsXScrollable(UIElement* e) {
	return HAS_FLAGS(e->flags, UI_XSCROLLABLE);
}

bool IsYScrollable(UIElement* e) {
	return HAS_FLAGS(e->flags, UI_YSCROLLABLE);
}

bool IsAncestorHidden(UIElement* e) {
	while (e) {
		if (e->flags & UI_HIDDEN) return true;
		e = e->parent;
	}
	return false;
}

bool DoesAncestorHidesOverflow(UIElement* e) {
	while (e) {
		if (e->flags & UI_HIDE_OVERFLOW) return true;
		e = e->parent;
	}
	return false;
}

// Screen Position/Hit-BOx
//---------------------------

Point2 GetInterpolatedPos(UIElement* element) {
	Point2 p0 = element->prevPos;
	Point2 p1 = element->pos;
	float32 t = element->transition;
	return (1 - t)*p0 + t*p1;
}

Point2 GetCanvasPosition(UIElement* element) {
	UIElement* parent = element->parent;
	if (parent == NULL) return element->pos;

	Point2 parentPos = GetCanvasPosition(parent);
	Point2 pos = element->flags & UI_TRANSITION
		? GetInterpolatedPos(element)
		: element->pos;
	Point2 scroll = parent->scroll;

	return parentPos + pos - scroll;
}

Point2 GetScreenPosition(UIElement* element) {
	Point2 canvasPos = GetCanvasPosition(element);
	return ui.zoomScale*(canvasPos - ui.zoomOffset);
}

Box2 GetCanvasHitBox(UIElement* element) {
	UIElement* parent = element->parent;
	if (parent == NULL)
		return {element->x, element->y, element->x + element->width, element->y + element->height};

	Point2 parentPos = GetCanvasPosition(parent);
	Point2 relative = element->flags & UI_TRANSITION
		? GetInterpolatedPos(element)
		: element->pos;
	Point2 scroll = parent->scroll;
	Point2 pos = parentPos + relative - scroll;

	if (DoesAncestorHidesOverflow(parent)) {
		Box2 parentBox = GetCanvasHitBox(parent);
		float32 x0 = MAX(parentBox.x0, pos.x);
		float32 y0 = MAX(parentBox.y0, pos.y);
		float32 x1 = MIN(parentBox.x1, pos.x + element->width);
		float32 y1 = MIN(parentBox.y1, pos.y + element->height);
		return {x0, y0, x1, y1};
	}
	else return {
		pos.x,
		pos.y,
		pos.x + element->width,
		pos.y + element->height
	};
}

UIElement* GetElementByCanvasPosition(Point2 canvasPos) {
	UIElement* element = ui.rootElement;
	while (element->last) element = element->last;

	while (element) {
		// NOTE: canvas has no dimensions
		if (element->flags & UI_CANVAS)
			break;

		if ((IsInteractable(element) || IsXScrollable(element) || IsYScrollable(element)) 
				&& !IsAncestorHidden(element)) {

			Box2 box = GetCanvasHitBox(element);
			if (box.x0 <= canvasPos.x && canvasPos.x <= box.x1 && box.y0 <= canvasPos.y && canvasPos.y <= box.y1)
				break;
		}

		if (element->prev) {
			element = element->prev;
			while (element->last) element = element->last;
		}
		else element = element->parent;
	}

	return element;
}

bool IsInBottomRight(UIElement* element, Point2 canvasPos, Point2 cursorPos) {
	float32 width = element->width;
	float32 height = element->height;
	float32 margin = MAX(4, element->radius);
	Point2 p1 = {canvasPos.x + width, canvasPos.y + height};
	return p1.x - margin <= cursorPos.x && p1.y - margin <= cursorPos.y;
}

Point2 ScreenPositionToCanvasPosition(Point2i p) {
	Point2 pos = {(float32)p.x, (float32)p.y};
	return (1.f/ui.zoomScale)*pos + ui.zoomOffset;
}

//-------------

void SetPosition(UIElement* element, float32 x, float32 y) {
	if (!element->parent || (element->parent->flags & UI_CANVAS) ) {
		element->pos = {x,y};
	}
	else {
		UIElement* parent = element->parent;

		if (element->flags & UI_X_MOVABLE) {
			if (x < 0) element->x = 0;
			else if (x + element->width > parent->width) element->x = parent->width-element->width;
			else element->x = x;
		}

		if (element->flags & UI_Y_MOVABLE) {
			if (y < 0) element->y = 0;
			else if (y + element->height > parent->height) element->y = parent->height-element->height;
			else element->y = y;
		}
	}
}

Dimensions2 GetContentDim(UIElement* e) {
	float32 width = 0;
	float32 height = 0;
	if (HasText(e)) {
		UIText text = e->text;
		bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
		TextMetrics metrics = GetTextMetrics(text.font, text.list, shouldWrap, e->width);
		width = metrics.maxx;
		height = metrics.y + 0.5f*text.font->height;
	}
	LINKEDLIST_FOREACH(e, UIElement, child) {
		if (child->flags & UI_HIDDEN) continue;
		width  = MAX(child->x + child->width, width);
		height = MAX(child->y + child->height, height);
	}
	return {width, height};
}

UIElement* GetNext(UIElement* e) {
	if (e->first && !(e->flags & UI_HIDDEN)) 
		return e->first;
	if (e->next) return e->next;
	do {e = e->parent;} while(e && !e->next);
	return e ? e->next : NULL;
}

UIElement* FindFirstInteractable(UIElement* e) {
	if (e->flags & UI_HIDDEN)
		return NULL;
	for (UIElement* current = GetNext(e); 
			current != NULL; 
				current = GetNext(current)) {
		if (current->flags & UI_HIDDEN)
			continue;
		if (IsInteractable(current))
			return current;
	}
	return NULL;
}

// Text
//------------

StringNode* CreateStringNode() {
	StringNode* result = (StringNode*)FixedSizeAlloc(&ui.stringNodeAllocator);
	ASSERT(result);
	return result;
}

StringList CreateStringList(String string) {
	return CreateStringList(string, CreateStringNode());
}

void DestroyStringNode(StringNode* node) {
	FixedSizeFree(&ui.stringNodeAllocator, node);
}

void UpdateSelectedElementScrollPosition() {
	// TODO: is it necessarilly the parent?
	UIElement* scrollable = ui.selected->parent;
	if (!IsXScrollable(scrollable) && !IsYScrollable(scrollable)) return;
	Point2 pos = ui.selected->pos;
	UIText text = ui.selected->text;
	bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
	TextMetrics metrics = GetTextMetrics(text.font, text.list, ui.head, shouldWrap, ui.selected->width);
	bool isStartOfLine = IsStartOfLine(metrics, text.font, 
									   ui.head, shouldWrap, ui.selected->width);
	float32 margin = text.font->height;
	Box2 box = {metrics.x - margin, metrics.y - margin, 
				metrics.x + margin, metrics.y};

	if (IsXScrollable(scrollable)) {
		if (isStartOfLine) {
			scrollable->scroll.x = 0;
		}
		else if (pos.x + box.x0 < scrollable->scroll.x) {
			scrollable->scroll.x = MAX(pos.x + box.x0, 0); 
		}
		else if (scrollable->scroll.x + scrollable->width < pos.x + box.x1) {
			scrollable->scroll.x = pos.x + box.x1 - scrollable->width; 
		}
	}

	if (IsYScrollable(scrollable)) {
		// up
		if (box.y0 <= scrollable->scroll.y) {
			scrollable->scroll.y = MAX(pos.y + box.y0 - text.font->height, 0); 
		}

		// down
		if (scrollable->scroll.y + scrollable->height < box.y1) {
			scrollable->scroll.y = pos.y + box.y1 - scrollable->height + 0.5f*text.font->height; 
		}
	}
}

void DeleteSelectedText() {
	StringList* list = &ui.selected->text.list;
	ui.head = StringListDelete(list, ui.tail, ui.head, 
		CreateStringNode, DestroyStringNode, &ui.text.ptr);
	ui.tail = ui.head; 
	UpdateSelectedElementScrollPosition();
}

void InsertText(String newString) {
	StringList* list = &ui.selected->text.list;
	ui.head = StringListInsert(list, newString, ui.tail, ui.head, 
		CreateStringNode, DestroyStringNode, &ui.text.ptr);
	ui.tail = ui.head; 
	UpdateSelectedElementScrollPosition();
}

void CopySelectedTextToClipboard() {
	// TODO: obviously problematic
	static byte buffer[256];
	ssize length;
	if (StringListPosCompare(ui.tail, ui.head)) {
		length = StringListCopy(ui.tail, ui.head, buffer);
	}
	else {
		length = StringListCopy(ui.head, ui.tail, buffer);
	}
	OSCopyToClipboard({buffer, length});
}

// Render
//------------

UIStyle GetCurrentStyle(UIElement* element) {
	UIStyle style = element->style;
	if ((element->flags & UI_HOVER_STYLE) && ui.hovered == element)
		style = element->hover;

	if ((element->flags & UI_ACTIVE_STYLE) && element->isActive)
		style = element->active;

	if (element == ui.focused) {
		style.borderWidth++;
		style.borderColor = COLOR_RED;
	}

	style.borderWidth *= ui.zoomScale;
	style.radius *= ui.zoomScale;

	return style;
}

void RenderElement(UIElement* element) {
	UIElement* parent = element->parent;

	// update width and height
	//------------------------
	Dimensions2 contentDim = GetContentDim(element);
	if (element->flags & UI_FIT_CONTENT) 
		element->dim = contentDim;
	if (element->flags & UI_MIN_CONTENT) 
		element->dim = {MAX(element->width, contentDim.width), MAX(element->height, contentDim.height)};
	if (element->flags & UI_X_THUMB) {
		UIElement* scrollBar = parent;
		UIElement* pane = scrollBar->parent->first;
		Dimensions2 paneDim = GetContentDim(pane);

		if (round(paneDim.width - 0.5f) <= pane->width) scrollBar->flags |= UI_HIDDEN;
		else {
			scrollBar->flags &= ~UI_HIDDEN;
			element->width = (pane->width*scrollBar->width)/paneDim.width;
		}
	}
	if (element->flags & UI_Y_THUMB) {
		UIElement* scrollBar = parent;
		UIElement* pane = scrollBar->parent->first;
		Dimensions2 paneDim = GetContentDim(pane);

		if (paneDim.height <= pane->height) scrollBar->flags |= UI_HIDDEN;
		else {
			scrollBar->flags &= ~UI_HIDDEN;
			element->height = (pane->height*scrollBar->height)/paneDim.height;
		}
	}

	// update x and y
	//----------------
	float32 x = element->x;
	float32 y = element->y;
	if (element->flags & UI_CENTER) 
		x = MAX((parent->width - element->width)/2.0f, 0);
	if (element->flags & UI_MIDDLE) 
		y = MAX((parent->height - element->height)/2.0f, 0);
	if ((element->flags & UI_HORIZONTAL_STACK) && element->prev) {
		x = element->prev->x 
			+ ( (element->prev->flags & UI_HIDDEN) ? 0 : element->prev->width) 
			+ element->margin;
	}
	if ((element->flags & UI_VERTICAL_STACK) && element->prev) {
		y = element->prev->y 
			+ ( (element->prev->flags & UI_HIDDEN) ? 0 : element->prev->height) 
			+ element->margin;
	}

	if (element->flags & UI_TRANSITION) {
		if (x != element->x || y != element->y) {
			element->prevPos = GetInterpolatedPos(element);
			element->transition = 0;
		}

		if (element->transition < 1) {
			element->transition += 0.125f;
		}
		else {
			element->transition = 0;
			element->prevPos = element->pos;
		}
	}
	element->pos = {x, y};

	// draw
	//-------
	if (element->flags & UI_HIDDEN) return;

	Point2 pos = GetScreenPosition(element);
	if (HasQuad(element)) {
		UIStyle style = GetCurrentStyle(element);
		GfxDrawQuad(pos, ui.zoomScale*element->dim, style.background, style.radius, style.borderWidth, style.borderColor,
			element->flags & UI_GRADIENT ? style.color2 : style.background);
	}
	if (HasText(element)) {
		UIText text = element->text;
		bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
		RenderText(pos, text.font, text.color, text.list, shouldWrap, pos.x + element->width, ui.zoomScale);
		
		if (ui.selected == element) {
			RenderTextSelection(pos, text.font, {0, 0, 1, 0.5f}, text.list,
						 		ui.tail, ui.head, shouldWrap, element->width, ui.zoomScale,
						 		text.color);	
		}
	}
	if (element->icon) {
		RenderGlyph(pos, &ui.iconsFont, COLOR_BLACK, element->icon, ui.zoomScale);
	}
	if (HasImage(element)) {
		GfxDrawImage(pos, ui.zoomScale*element->dim, element->image.atlas, element->image.crop);
	}

	// children
	//----------
	if (element->flags & UI_HIDE_OVERFLOW)
		GfxCropScreen((int32)pos.x, (int32)pos.y, (int32)element->width, (int32)element->height);
	LINKEDLIST_FOREACH(element, UIElement, child) 
		RenderElement(child);
	if (element->flags & UI_HIDE_OVERFLOW)
		GfxClearCrop();
}

// API
//------------

void UIInit(Arena* persist, Arena* scratch, AtlasBitmap* atlas) {
	ui = {};
	ui.allocator = CreateFixedSize(persist, 200, sizeof(UIElement));
	ui.stringNodeAllocator = CreateFixedSize(persist, 200, sizeof(StringNode));
	ui.text.buffer = ArenaAlloc(persist, 512);
	ui.text.ptr = ui.text.buffer;
	ui.iconsFont = LoadAndBakeIconsFont(scratch, atlas, 24);
	ui.zoomScale = 1;
}

UIElement* UICreateWindowElement(Color background) {
	FixedSizeReset(&ui.allocator);
	ui.rootElementIsWindow = true;
	ui.rootElement = (UIElement*)FixedSizeAlloc(&ui.allocator);
	ui.rootElement->pos = {0, 0};
	Dimensions2i windowDim = OSGetWindowDimensions();
	ui.rootElement->dim = {(float32)windowDim.width, (float32)windowDim.height};
	ui.rootElement->background = background;
	GfxSetBackgroundColor(background);
	return ui.rootElement;
}

UIElement* UICreateElement(
		UIElement* parent, 
		Point2 pos = {}, 
		Dimensions2 dim = {}, 
		UIStyle style = {}, 
		uint64 flags = 0) {

	ASSERT(parent || ui.rootElement);

	UIElement* element = (UIElement*)FixedSizeAlloc(&ui.allocator);
	ASSERT(element != NULL);

	if (parent == NULL) parent = ui.rootElement;
	LINKEDLIST_ADD(parent, element);
	element->parent = parent;

	element->pos = pos;
	element->dim = dim;
	element->style = style;
	element->flags = flags;

	return element;
}

void UIDestroyStringList(StringList* list) {
	LINKEDLIST_FOREACH(list, StringNode, child) DestroyStringNode(child);
}

void UIDestroyElement(UIElement* element) {
	if (ui.hovered == element) ui.hovered = NULL;
	if (ui.selected == element) ui.selected = NULL;
	if (ui.focused == element) ui.focused = NULL;

	LINKEDLIST_FOREACH(&element->text.list, StringNode, child) DestroyStringNode(child);
	LINKEDLIST_REMOVE(element->parent, element);
	LINKEDLIST_FOREACH(element, UIElement, child) UIDestroyElement(child);

	FixedSizeFree(&ui.allocator, element);
}

UIElement* UIAddText(UIElement* parent, UIText text) {
	UIElement* textElement = UICreateElement(parent);
	if (text.string.length) {
		// TODO: maybe a different flag?
		if ( (text.flags & TEXT_EDITABLE) == TEXT_EDITABLE) {
			StringCopy(text.string, ui.text.ptr);
			text.list = CreateStringList({ui.text.ptr, text.string.length});
			ui.text.ptr += text.string.length;
		}
		else {
			text.list = CreateStringList(text.string);
		}
	}
	textElement->text = text;
	textElement->flags = UI_MIN_CONTENT | UI_MIDDLE | UI_CENTER;
	return textElement;
}

UIElement* UIAddText(UIElement* parent, UIText text, float32 xmargin) {
	UIElement* textElement = UICreateElement(parent);
	if (text.string.length) {
		// TODO: maybe a different flag?
		if ( (text.flags & TEXT_EDITABLE) == TEXT_EDITABLE) {
			StringCopy(text.string, ui.text.ptr);
			text.list = CreateStringList({ui.text.ptr, text.string.length});
			ui.text.ptr += text.string.length;
		}
		else {
			text.list = CreateStringList(text.string);
		}
	}
	textElement->x = xmargin;
	textElement->text = text;
	textElement->flags = UI_MIN_CONTENT;

	if (text.flags & TEXT_WRAPPING) {
		textElement->width = parent->width - 2*xmargin;
		textElement->height = parent->height;
	}

	return textElement;
}

UIElement* UIAddImage(UIElement* parent, Point2 pos, Dimensions2 dim, TextureId atlas, Box2 crop) {
	UIElement* imageElement = UICreateElement(parent, pos, dim);
	imageElement->image = {atlas, crop};
	return imageElement;
}

void UIEnableGradient(UIElement* element, Color color2) {
	element->flags |= UI_GRADIENT;
	element->color2 = color2;
}

void UIEnableHoverStyle(UIElement* element, UIStyle hover) {
	element->flags |= UI_HOVER_STYLE;
	element->hover = hover;
}

void UIEnableActiveStyle(UIElement* element, UIStyle active) {
	element->flags |= UI_ACTIVE_STYLE;
	element->active = active;
}

// NOTE: right now I'm using this to override the cursor with arrow
// so it might be an overkill to specify the cursor.
void UISetCursor(UIElement* element, int32 cursor) {
	element->flags |= UI_CURSOR;
	element->cursor = cursor;
}

void UIEnableTransition(UIElement* element) {
	element->flags |= UI_TRANSITION;
	element->prevPos = element->pos;
}

void UIProcessEvent(OSEvent event) {
	switch (event.type) {
	case Event_WindowResize: {
		if (ui.rootElementIsWindow) {
			Dimensions2i dim = event.window.dim;
			ui.rootElement->dim = {(float32)dim.width, (float32)dim.height};
		}
	} break;
	case Event_MouseMove: {
		Point2 cursorPos = ScreenPositionToCanvasPosition(event.mouse.cursorPos);
		UIElement* element = ui.hovered;

		if (ui.isResizing) {
			Point2 relativeCursorPos = cursorPos - element->parent->pos;
			float32 x0 = MIN(ui.originalPos.x, relativeCursorPos.x);
			float32 y0 = MIN(ui.originalPos.y, relativeCursorPos.y);
			float32 x1 = MAX(ui.originalPos.x, relativeCursorPos.x);
			float32 y1 = MAX(ui.originalPos.y, relativeCursorPos.y);

			element->x = MAX(x0, 0);
			element->y = MAX(y0, 0);
			element->width = MIN(x1 - element->x, element->parent->width - element->x);
			element->height = MIN(y1 - element->y, element->parent->height - element->y);
			return;
		}

		if (ui.isGrabbing) {
			if (cursorPos.x != ui.grabPos.x || cursorPos.y != ui.grabPos.y) {
				float32 newx = ui.originalPos.x + cursorPos.x - ui.grabPos.x;
				float32 newy = ui.originalPos.y + cursorPos.y - ui.grabPos.y;
				SetPosition(element, newx, newy);
				if (element->onMove) element->onMove(element);
			}
			return;
		}

		ui.hovered = GetElementByCanvasPosition(cursorPos);
		element = ui.hovered;
		if (element == NULL) {
			OSSetCursorIcon(CUR_ARROW);
			return;
		}
		Point2 canvasPos = GetCanvasPosition(element);

		if (ui.isSelecting) {
			// TODO: handle zoom
			if (element == ui.selected) {
				ui.selectionCount = 0;
				UIText text = ui.selected->text;
				bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
				float32 relx = cursorPos.x - canvasPos.x + element->scroll.x;
				float32 rely = cursorPos.y - canvasPos.y + element->scroll.y - text.font->height;
				
				ui.head = GetCharPos(text.font, text.list, relx, rely, shouldWrap, element->width);
			}
			
			return;
		}

		if (element->flags & UI_CURSOR) OSSetCursorIcon(element->cursor);
		else if (IsInBottomRight(element, canvasPos, cursorPos) && (element->flags & UI_RESIZABLE)) OSSetCursorIcon(CUR_RESIZE);
		else if ((element->flags & UI_MOVABLE) == UI_MOVABLE) OSSetCursorIcon(CUR_MOVE);
		else if (element->flags & UI_X_MOVABLE) OSSetCursorIcon(CUR_MOVESIDE);
		else if (element->flags & UI_Y_MOVABLE) OSSetCursorIcon(CUR_MOVEUPDN);
		else if (element->flags & UI_CLICKABLE) OSSetCursorIcon(CUR_HAND);
		else if (element->text.flags & TEXT_SELECTABLE) OSSetCursorIcon(CUR_TEXT);
		else OSSetCursorIcon(CUR_ARROW);
	} break;

	case Event_MouseLeftButtonDown: {
		Point2 cursorPos = ScreenPositionToCanvasPosition(event.mouse.cursorPos);
		UIElement* element = ui.hovered;
		if (!element) return;

		Point2 canvasPos = GetCanvasPosition(element);

		if (element->flags & UI_SHUFFLEABLE) {
			LINKEDLIST_MOVE_TO_LAST(element->parent, element);
		}

		if (element->onClick) {
			element->onClick(element);
		}

		if (HasEditableText(element)) {
			// TODO: handle zoom
			UIText text = element->text;
			float32 relx = cursorPos.x - canvasPos.x + element->scroll.x;
			float32 rely = cursorPos.y - canvasPos.y + element->scroll.y - text.font->height;
			bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
			ui.head = GetCharPos(text.font, text.list, relx, rely, shouldWrap, element->width);
			ui.tail = ui.head;
			ui.selected = element;
			if (ui.focused == element) ui.focused = NULL;
			ui.isSelecting = true;
		}
		else {
			ui.selected = NULL;
			if (IsInBottomRight(element, canvasPos, cursorPos) 
				&& (element->flags & UI_RESIZABLE)) {

				ui.isResizing = true;
				ui.originalPos = element->pos;
			}
			else if (element->flags & UI_MOVABLE) {
				ui.isGrabbing = true;
				ui.grabPos = cursorPos;
				ui.originalPos = element->pos;
			}
		}

		ui.focused = NULL;
	} break;

	case Event_MouseLeftButtonUp: {
		ui.isGrabbing = false;
		ui.isResizing = false;
		ui.isSelecting = false;
	} break;

	case Event_MouseVerticalWheel: {
		UIElement* scrollable = ui.hovered;
		while (scrollable && !IsYScrollable(scrollable))
			scrollable = scrollable->parent;
		if (scrollable) {
			Dimensions2 contentDim = GetContentDim(scrollable);
			scrollable->scroll.y -= event.mouse.wheelDelta;
			if (scrollable->scroll.y < 0) scrollable->scroll.y = 0;
			if (scrollable->scroll.y > contentDim.height - scrollable->height)
				scrollable->scroll.y = MAX(contentDim.height - scrollable->height, 0);

			UIElement* yscrollBar = scrollable->yscrollBar;
			if (yscrollBar) {
				UIElement* ythumb = yscrollBar->first;
				ythumb->y = ((yscrollBar->height - ythumb->height)*scrollable->scroll.y)/(contentDim.height - scrollable->height); 
			}
		}

		else {
			// zoom in/out
			if (event.mouse.ctrlIsDown) {
				Point2 canvasPos = ScreenPositionToCanvasPosition(event.mouse.cursorPos);
				Point2 cursor = {(float32)event.mouse.cursorPos.x, (float32)event.mouse.cursorPos.y};
				float32 oldScale = ui.zoomScale;
				ui.zoomScale *= 1 + event.mouse.wheelDelta/480.0f;
				ui.zoomOffset = (1/oldScale - 1/ui.zoomScale)*cursor + ui.zoomOffset;
				canvasPos = ScreenPositionToCanvasPosition(event.mouse.cursorPos);
			}
			else {
				ui.zoomOffset.y -= event.mouse.wheelDelta;
			}
		}
	} break;

	case Event_MouseHorizontalWheel: {
		UIElement* scrollable = ui.hovered;
		while (scrollable && !IsXScrollable(scrollable))
			scrollable = scrollable->parent;
		if (scrollable) {
			Dimensions2 contentDim = GetContentDim(scrollable);
			scrollable->scroll.x += event.mouse.wheelDelta;
			if (scrollable->scroll.x < 0) scrollable->scroll.x = 0;
			if (scrollable->scroll.x > contentDim.width - scrollable->width)
				scrollable->scroll.x = MAX(contentDim.width - scrollable->width, 0);

			UIElement* xscrollBar = scrollable->xscrollBar;
			if (xscrollBar) {
				UIElement* xthumb = xscrollBar->first;
				xthumb->x = ((xscrollBar->width - xthumb->width)*scrollable->scroll.x)/(contentDim.width - scrollable->width); 
			}
		}

		else {
			ui.zoomOffset.x += event.mouse.wheelDelta;
		}
	} break;

	case Event_MouseDoubleClick: {
		UIElement* element = ui.hovered;
		if (!HasText(element)) return;
		ui.selected = element;
		if (ui.focused == element) ui.focused = NULL;
		StringListFindWord(element->text.list, ui.head, &ui.tail, &ui.head);
		ui.isSelecting = false;
	} break;

	case Event_MouseTripleClick: {
		UIElement* element = ui.hovered;
		if (!HasText(element)) return;
		ui.selected = element;
		if (ui.focused == element) ui.focused = NULL;
		StringList list = ui.selected->text.list;
		ui.tail = StringListPosInc(StringListGetLastPosOf(SL_START(list), ui.head, 10));
		ui.head = StringListGetFirstPosOf(ui.head, SL_END(list), 10);
		ui.isSelecting = false;
	} break;

	case Event_KeyboardPress: {
		switch (event.keyboard.vkCode) {
		case KEY_TAB: {
			if (ui.selected) {
				// do nothing
			}
			else {
				if (ui.focused) {
					ui.focused = FindFirstInteractable(ui.focused);
				}
				if (!ui.focused) {
					ui.focused = FindFirstInteractable(ui.rootElement);
				}
			}
		} break;
		case KEY_SPACE: {
			if (ui.selected) {
				// do nothing
			}
			else if (ui.focused) {
				if (ui.focused->onClick) ui.focused->onClick(ui.focused);
				if (ui.focused->flags & UI_SHUFFLEABLE)
					LINKEDLIST_MOVE_TO_LAST(ui.focused->parent, ui.focused);
			}
		} break;
		case KEY_ENTER: {
			if (ui.selected) {
				// do nothing
			}
			
			if (ui.focused) {
				if (ui.focused->onClick) ui.focused->onClick(ui.focused);
				if (ui.focused->flags & UI_SHUFFLEABLE)
					LINKEDLIST_MOVE_TO_LAST(ui.focused->parent, ui.focused);
				if (HasEditableText(ui.focused)) {
					ui.selected = ui.focused;
					//ui.focused = NULL;
					ui.head = SL_START(ui.selected->text.list);
					ui.tail = ui.head; 
				}
			}
		} break;
		case KEY_LEFT: {
			if (ui.selected && !StringListIsStart(ui.head)) {
				StringListPosDec(&ui.head);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.tail = ui.head;
					
				UpdateSelectedElementScrollPosition();
			}
			else if (ui.focused) {
				if (ui.focused->flags & UI_X_MOVABLE) {
					SetPosition(ui.focused, ui.focused->x - 1, ui.focused->y);
					if (ui.focused->onMove) ui.focused->onMove(ui.focused);
				}
				else if (HasSelectableText(ui.focused)) {
					ui.selected = ui.focused;
					ui.focused = NULL;
					ui.head = SL_END(ui.selected->text.list);
					ui.tail = ui.head;
				}
			}
		} break;
		case KEY_RIGHT: {
			if (ui.selected && !StringListIsEnd(ui.head)) {
				StringListPosInc(&ui.head);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.tail = ui.head;
					
				UpdateSelectedElementScrollPosition();
			}
			else if (ui.focused) {
				if (ui.focused->flags & UI_X_MOVABLE) {
					SetPosition(ui.focused, ui.focused->x + 1, ui.focused->y);
					if (ui.focused->onMove) ui.focused->onMove(ui.focused);
				}
				else if (HasSelectableText(ui.focused)) {
					ui.selected = ui.focused;
					ui.focused = NULL;
					ui.head = SL_START(ui.selected->text.list);
					ui.tail = ui.head;
				}
			}
		} break;
		case KEY_UP: {
			if (ui.selected && !StringListIsStart(ui.head)) {
				UIText text = ui.selected->text;
				bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
				TextMetrics metrics = GetTextMetrics(text.font, text.list, ui.head, shouldWrap, ui.selected->width);
				ui.head = GetCharPos(text.font, text.list, 
									  metrics.x, metrics.y - 2*text.font->height,
									  shouldWrap, ui.selected->width);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.tail = ui.head;
					
				UpdateSelectedElementScrollPosition();
			}
			else if (ui.focused && ui.focused->flags & UI_Y_MOVABLE) {
				SetPosition(ui.focused, ui.focused->x, ui.focused->y - 1);
				if (ui.focused->onMove) ui.focused->onMove(ui.focused);
			}
		} break;
		case KEY_DOWN: {
			if (ui.selected && !StringListIsEnd(ui.head)) {
				UIText text = ui.selected->text;
				bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
				TextMetrics metrics = GetTextMetrics(text.font, text.list, ui.head, shouldWrap, ui.selected->width);
				ui.head = GetCharPos(text.font, text.list, 
									  metrics.x, metrics.y,
									  shouldWrap, ui.selected->width);

				if (!OSIsKeyDown(KEY_SHIFT))
					ui.tail = ui.head;
					
				UpdateSelectedElementScrollPosition();
			}
			else if (ui.focused && ui.focused->flags & UI_Y_MOVABLE) {
				SetPosition(ui.focused, ui.focused->x, ui.focused->y + 1);
				if (ui.focused->onMove) ui.focused->onMove(ui.focused);
			}
		} break;
		case KEY_HOME: {
			if (ui.selected) {
				UIText text = ui.selected->text;
				ui.head = event.keyboard.ctrlIsDown
					? SL_START(text.list)
					: StringListGetLastPosOf(SL_START(text.list), ui.head, 10);

				if (!event.keyboard.shiftIsDown)
					ui.tail = ui.head;

				UpdateSelectedElementScrollPosition();
			}
		} break;
		case KEY_END: {
			if (ui.selected) {
				UIText text = ui.selected->text;
				ui.head = event.keyboard.ctrlIsDown
					? SL_END(text.list)
					: StringListGetFirstPosOf(ui.head, SL_END(text.list), 10);

				if (!event.keyboard.shiftIsDown)
					ui.tail = ui.head;

				UpdateSelectedElementScrollPosition();
			}
		} break;
		case KEY_C: {
			if (ui.selected && event.keyboard.ctrlIsDown) {
				CopySelectedTextToClipboard();
			}
		} break;
		case KEY_X: {
			if (ui.selected && HasEditableText(ui.selected) && event.keyboard.ctrlIsDown) {
				CopySelectedTextToClipboard();
				if (HasEditableText(ui.selected))
					DeleteSelectedText();
			}
		} break;
		case KEY_V: {
			if (ui.selected && HasEditableText(ui.selected) && event.keyboard.ctrlIsDown) {
				OSRequestClipboardData(InsertText);
			}
		} break;
		case KEY_BACKSPACE: {
			if (ui.selected && HasEditableText(ui.selected)) {
				DeleteSelectedText();
			}
		} break;
		case KEY_DELETE: {
			if (ui.selected && HasEditableText(ui.selected)) {
				if (ui.tail.node == ui.head.node && ui.tail.index == ui.head.index) {
					if (StringListIsEnd(ui.head)) break;
					StringListPosInc(&ui.head);
					ui.tail = ui.head;
				}
				DeleteSelectedText();
			}
		} break;
		}
	} break;
	case Event_KeyboardChar: {
		if (ui.selected && (HasEditableText(ui.selected))) {
			byte b = event.keyboard.character;
			if (b == 10 && ui.focused == ui.selected) {
				ui.focused = NULL;
			}
			else {
				InsertText({&b, 1});
			}
		}
		else if (ui.focused && HasEditableText(ui.focused)) {
			byte b = event.keyboard.character;
			if (b == 9 || b == 10) {
				// do nothing
			}
			else {
				ui.selected = ui.focused;
				ui.focused = NULL;
				ui.head = SL_START(ui.selected->text.list);
				ui.tail = ui.head;
				InsertText({&b, 1});
			}
		}
	} break;
	} 
}

void UIUpdate() {
	OSEvent event;
	while (OSPollEvent(&event)) {
		UIProcessEvent(event);
	}

	// NOTE: this is not a response to any os event
	// TODO: maybe I should trigger an event to handle this
	if (ui.isSelecting && ui.hovered != ui.selected) {
		if (ui.selectionCount) {
			ui.selectionCount--;
		}
		else {
			ui.selectionCount = 6;
			UIElement* parent = ui.selected->parent;
			Point2 selectedPos = GetCanvasPosition(parent);												

			UIText text = ui.selected->text;
			bool shouldWrap = (text.flags & TEXT_WRAPPING) == TEXT_WRAPPING;
			
			TextMetrics prev = StringListIsStart(ui.head)
				? TextMetrics{}
				: GetTextMetrics(text.font, text.list, StringListPosDec(ui.head), shouldWrap, ui.selected->width);
			TextMetrics metrics = NextTextMetrics(prev, text.font, StringListPosDec(ui.head), shouldWrap, ui.selected->width);
			TextMetrics next = StringListIsEnd(ui.head)
				? metrics
				: NextTextMetrics(metrics, text.font, ui.head, shouldWrap, ui.selected->width);

			bool isStartOfLine = IsStartOfLine(metrics, text.font,
											   ui.head, shouldWrap, ui.selected->width);

			Point2 cursorPos = ScreenPositionToCanvasPosition(OSGetCursorPosition());

			// left
			if (cursorPos.x < selectedPos.x && !StringListIsStart(ui.head)) {
				bool prevIsStartOfLine = IsStartOfLine(prev, text.font, 
													   StringListPosDec(ui.head), shouldWrap, ui.selected->width);

				if ( (prev.y == metrics.y && !isStartOfLine) ||
					 (prevIsStartOfLine) )

					StringListPosDec(&ui.head);
			}

			// right
			if (selectedPos.x + parent->width <= cursorPos.x && !StringListIsEnd(ui.head)) {
				if (next.y == metrics.y) 
					StringListPosInc(&ui.head);
			}

			// up
			if (cursorPos.y < selectedPos.y) {
				ui.head = GetCharPos(text.font, text.list, 
									  metrics.x, metrics.y - 2*text.font->height,
									  shouldWrap, ui.selected->width);					
			}

			// down
			if (selectedPos.y + parent->height < cursorPos.y) {
				ui.head = GetCharPos(text.font, text.list, 
									  metrics.x, metrics.y,
									  shouldWrap, ui.selected->width);
			}

			UpdateSelectedElementScrollPosition();
		}
	}
}

void UIRender() {
	if (ui.rootElementIsWindow) 
		LINKEDLIST_FOREACH(ui.rootElement, UIElement, child) RenderElement(child);
	else 
		RenderElement(ui.rootElement);
}

// Specific Widgets
//-------------------

UIElement* UICreateButton(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style) {
	UIElement* button = UICreateElement(parent, pos, dim, style, UI_CLICKABLE);
	UIStyle hover = style;
	hover.background = {0.5f*style.background.rgb, style.background.a};
	UIEnableHoverStyle(button, hover);

	button->name = STR("button");
	return button;
}

void __check_box(UIElement* e) {
	if (e->isChecked) {
		e->isChecked = false;
		e->icon = Icon_check_empty;
	}
	else {
		e->isChecked = true;
		e->icon = Icon_check;
	}
}

UIElement* UICreateCheckbox(UIElement* parent, Point2 pos = {}) {
	UIElement* checkbox = UICreateElement(parent, pos, {}, {}, UI_FIT_CONTENT | UI_CLICKABLE);
	checkbox->icon = Icon_check_empty;
	checkbox->onClick = __check_box;

	checkbox->name = STR("checkbox");
	return checkbox;
}

void __select_button(UIElement* e) {
	UIElement* radioGroup = e->parent;
	LINKEDLIST_FOREACH(radioGroup, UIElement, button) {
		button->isChecked = button == e;
		button->icon = button == e 
			? Icon_dot_circled
			: Icon_circle_empty;
	}
}

UIElement* UICreateRadioButton(UIElement* radioGroup, Point2 pos = {}) {
	UIElement* radioButton = UICreateElement(radioGroup, pos, {}, {}, UI_FIT_CONTENT | UI_CLICKABLE);
	radioButton->icon = Icon_circle_empty;
	radioButton->onClick = __select_button;

	radioButton->name = STR("radio");
	return radioButton;
}

UIElement* UICreateSlider(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style, float32 thumbWidth) {
	float32 half_height = dim.height/2.f;
	float32 quater_height = half_height/2.f;
	UIElement* slider = UICreateElement(parent, {pos.x, pos.y - quater_height}, {dim.width, half_height}, style);
	UICreateElement(slider, {(dim.width - thumbWidth)/2.f, -quater_height}, {thumbWidth, dim.height}, style, 
		UI_X_MOVABLE);

	slider->name = STR("slider");
	return slider;
}

UIElement* UICreateVSlider(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style, float32 thumbHeight) {
	float32 half_width = dim.width/2.f;
	float32 quater_width = half_width/2.f;
	UIElement* slider = UICreateElement(parent, {pos.x - quater_width, pos.y}, {half_width, dim.height}, style);
	UICreateElement(slider, {-quater_width, (dim.height - thumbHeight)/2.f}, {dim.width, thumbHeight}, style, 
		UI_Y_MOVABLE);

	slider->name = STR("slider");
	return slider;
}

void __activate_tab(UIElement* e) {
	UIElement* tabControl = e->parent;
	LINKEDLIST_FOREACH(tabControl, UIElement, header) {
		header->isActive = header == e;
		if (header == e)
			header->first->next->flags &= ~UI_HIDDEN;
		else
			header->first->next->flags |= UI_HIDDEN;
	}
}

UIElement* UICreateTab(UIElement* tabControl, Dimensions2 headerDim, UIStyle active, UIText header) {
	Color bg = active.background;
	UIStyle nonactive = {{0.5f*bg.rgb + 0.5f, bg.a},
		active.radius, active.borderWidth, active.borderColor};
	UIStyle hover = {{0.75f*bg.rgb + 0.25f, bg.a}, 
		active.radius, active.borderWidth, active.borderColor};

	float32 x = 0;
	LINKEDLIST_FOREACH(tabControl, UIElement, sibling) {
		if (x < sibling->x + sibling->width) x = sibling->x + sibling->width;
	}

	UIElement* tab = UICreateElement(tabControl, {x, 2}, {headerDim.width, headerDim.height + active.radius}, nonactive, 
		UI_CLICKABLE | UI_SHUFFLEABLE);
	UIAddText(tab, header);
	UIEnableHoverStyle(tab, hover);
	UIEnableActiveStyle(tab, active);
	tab->onClick = __activate_tab;

	UIElement* tabBody = UICreateElement(tab, {-x, headerDim.height}, {tabControl->width, tabControl->height - headerDim.height - 2}, {bg});
	__activate_tab(tab);

	tab->name = STR("tab");
	tabBody->name = STR("tab body");
	return tabBody;
}

void UIActivateTab(UIElement* tabBody) {
	UIElement* tab = tabBody->parent;
	__activate_tab(tab);
	LINKEDLIST_MOVE_TO_LAST(tab->parent, tab);
}

void __scroll_x(UIElement* e) {
	UIElement* scrollBar = e->parent;
	UIElement* pane = scrollBar->parent->first;
	Dimensions2 contentDim = GetContentDim(pane);
	float32 x = ((contentDim.width - pane->width)*e->x)/(scrollBar->width - e->width); 
	pane->scroll.x = x;
}

void __scroll_y(UIElement* e) {
	UIElement* scrollBar = e->parent;
	UIElement* pane = scrollBar->parent->first;
	Dimensions2 contentDim = GetContentDim(pane);
	float32 y = ((contentDim.height - pane->height)*e->y)/(scrollBar->height - e->height); 
	pane->scroll.y = y;
}

UIElement* UICreateScrollPane(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style) {
	UIElement* container = UICreateElement(parent, pos, {dim.width + 15, dim.height + 15});
	UIElement* scrollPane = UICreateElement(container, {}, dim, style, UI_XYSCROLLABLE);

	UIElement* yscrollBar = UICreateElement(container, {dim.width + 4, 6}, {7, dim.height - 12}, {{0, 0, 0, 0.25f}, 3});
	UIElement* ythumb = UICreateElement(yscrollBar, {}, {7, 14}, {{1, 1, 1, 0.33f}, 3}, UI_Y_MOVABLE | UI_Y_THUMB);
	UIEnableHoverStyle(ythumb, {{1, 1, 1, 0.5f}, 3});
	UISetCursor(ythumb, CUR_ARROW);
	ythumb->onMove = __scroll_y;
	scrollPane->yscrollBar = yscrollBar;

	UIElement* xscrollBar = UICreateElement(container, {6, dim.height + 4}, {dim.width - 12, 7}, {{0, 0, 0, 0.25f}, 3});
	UIElement* xthumb = UICreateElement(xscrollBar, {}, {14, 7}, {{1, 1, 1, 0.33f}, 3}, UI_X_MOVABLE | UI_X_THUMB);
	UIEnableHoverStyle(xthumb, {{1, 1, 1, 0.5f}, 3});
	UISetCursor(xthumb, CUR_ARROW);
	xthumb->onMove = __scroll_x;
	scrollPane->xscrollBar = xscrollBar;

	return scrollPane;
}

void __move_vsplitter(UIElement* e) {
	UIElement* A = e->prev;
	UIElement* B = e->next;

	if (A) A->width = e->x - A->x;
	if (B) {
		float32 x1 = B->x + B->width;
		B->x = e->x + e->width;
		B->width = x1 - B->x;
	}
}

UIElement* UICreateVSplitter(UIElement* parent, float32 x, float32 width) {
	UIElement* splitter = UICreateElement(parent, {x - width/2, 0}, {width, parent->height}, {{0.5f, 0.5f, 0.5f, 1}, 0, 1, COLOR_BLACK}, UI_X_MOVABLE);
	splitter->onMove = __move_vsplitter;
	__move_vsplitter(splitter);
	return splitter;
}

void __move_hsplitter(UIElement* e) {
	UIElement* A = e->prev;
	UIElement* B = e->next;

	if (A) A->height = e->y - A->y;
	if (B) {
		float32 y1 = B->y + B->height;
		B->y = e->y + e->height;
		B->height = y1 - B->y;
	}
}

UIElement* UICreateHSplitter(UIElement* parent, float32 y, float32 height) {
	UIElement* splitter = UICreateElement(parent, {0, y - height/2}, {parent->width, height}, {{0.5f, 0.5f, 0.5f, 1}, 0, 1, COLOR_BLACK}, UI_Y_MOVABLE);
	splitter->onMove = __move_hsplitter;
	__move_hsplitter(splitter);
	return splitter;
}

void __expand_or_collapse(UIElement* e) {
	UIElement* children = e->first;
	if (e->icon == Icon_right_dir) {
		// TODO: need to find a good way to do this with transition
		// animation
		children->flags &= ~UI_HIDDEN;
		e->icon = Icon_down_dir;
	}
	else if (e->icon == Icon_down_dir) {
		children->flags |= UI_HIDDEN;
		e->icon = Icon_right_dir;
	}
}

UIElement* UICreateTreeRoot(UIElement* parent, Point2 pos, Point2 subTreeOffset) {
	UIElement* root = UICreateElement(parent, pos, {}, {}, UI_FIT_CONTENT);
	root->onClick = __expand_or_collapse;

	UICreateElement(root, subTreeOffset, {}, {}, UI_FIT_CONTENT); // children container

	return root;
}

UIElement* UICreateTreeItem(UIElement* parent) {
	ASSERT(parent && parent->first);
	UIElement*item = UICreateElement(parent->first, {}, {}, {}, 
		UI_FIT_CONTENT | UI_VERTICAL_STACK);
	UIEnableTransition(item);
	parent->icon = Icon_down_dir;
	parent->onClick = __expand_or_collapse;

	UICreateElement(item, parent->first->pos, {}, {}, UI_FIT_CONTENT); // children container

	return item;
}

UIElement* UICreateTextBox(UIElement* parent, Point2 pos, Dimensions2 dim, UIText text) {
	UIElement* textBox = UICreateScrollPane(parent, pos, dim, {{0.9f, 0.9f, 0.9f, 1}, 0, 1, COLOR_BLACK});
	UIAddText(textBox, text, 6);

	textBox->name = STR("text box");
	return textBox;
}

UIElement* UICreateTextBox(UIElement* parent, Point2 pos, Dimensions2 dim, UIText text, float32 innerWidth) {
	UIElement* textBox = UICreateScrollPane(parent, pos, dim, {{0.9f, 0.9f, 0.9f, 1}, 0, 1, COLOR_BLACK});
	UIElement* textElement = UIAddText(textBox, text, 6);
	textElement->width = innerWidth;

	return textBox;
}

UIElement* UIAddOption(UIElement* options, float32 x, float32 width, UIStyle style, 
		UIText text, float32 margin, void (*onClick)(UIElement*)) {

	UIElement* option = UICreateElement(options, {x}, {width}, style, UI_MIN_CONTENT | UI_VERTICAL_STACK);
	UIAddText(option, text, margin);
	option->onClick = onClick;
	return option;
}