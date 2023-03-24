/*
 * TODO:
 * selectable text
 * text navigation
 * EDITABLE TEXT!!! 
 * drop-down, menu-bar, list-box, combo-box
 * tooltip, context-menu
 * theme/palette
 */

#include "uigraphics.cpp"
#include "font.cpp"
#include "icons.cpp"

#define UI_X_MOVABLE		 (1ull << 0)
#define UI_Y_MOVABLE		 (1ull << 1)
#define UI_MOVABLE			 (UI_X_MOVABLE | UI_Y_MOVABLE)
#define UI_RESIZABLE		 (1ull << 2)
#define UI_SHUFFLABLE		 (1ull << 3)
#define UI_CLICKABLE		 (1ull << 4)
#define UI_HIDE_OVERFLOW	 (1ull << 5)
#define UI_SCROLLABLE		((1ull << 6) | UI_HIDE_OVERFLOW)
#define UI_INFINITE_SCROLL	((1ull << 7) | UI_SCROLLABLE)
// TODO: maybe hidden shouldn't be a flag? 
// Same as isChecked/isSelected/isActive?
#define UI_HIDDEN			 (1ull << 8)

#define UI_CENTER			 (1ull << 10)
#define UI_MIDDLE			 (1ull << 11)
#define UI_HORIZONTAL_STACK	 (1ull << 12)
#define UI_VERTICAL_STACK	 (1ull << 13)
#define UI_FIT_CONTENT		 (1ull << 14)
#define UI_MIN_CONTENT		 (1ull << 15)
#define UI_X_THUMB			 (1ull << 16)
#define UI_Y_THUMB			 (1ull << 17)

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
	StringList editable;
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

	UIElement* parent;
	UIElement* first;
	UIElement* last;
	UIElement* prev;
	UIElement* next;
};

struct {
	FixedSize allocator;
	BakedFont iconsFont;

	UIElement* rootElement;

	UIElement* hovered;
	UIElement* focused;

	Point2 originalPos;
	Point2 grabPos;

	bool isGrabbing;
	bool isResizing;
	bool rootElementIsWindow;

	ssize start;
	ssize end;
} ui;

// Util
//-------------

Box2 GetScreenHitBox(UIElement* element) {
	UIElement* parent = element->parent;
	if (parent == NULL) 
		return {element->x, element->y, element->x + element->width, element->y + element->height};

	Box2 parentBox = GetScreenHitBox(parent);
	if (parent->flags & UI_HIDE_OVERFLOW) {
		float32 x0 = parentBox.x0 + MAX(element->x, 0);
		float32 y0 = parentBox.y0 + MAX(element->y, 0);
		float32 x1 = MIN(parentBox.x0 + element->x + element->width, parentBox.x1);
		float32 y1 = MIN(parentBox.y0 + element->y + element->height, parentBox.y1);
		return {x0, y0, x1, y1};
	}
	else {
		float32 x0 = parentBox.x0 + element->x;
		float32 y0 = parentBox.y0 + element->y;
		float32 x1 = parentBox.x0 + element->x + element->width;
		float32 y1 = parentBox.y0 + element->y + element->height;
		return {x0, y0, x1, y1};
	}
}

// NOTE: both for drawing and for intercepting hit-box
// TODO: probably should split
bool HasQuad(UIElement* e) {
	return (e->background.a) || (e->borderWidth && e->borderColor.a) 
		|| (e->flags & UI_CLICKABLE) || e->onClick 
		|| (e->text.flags & TEXT_SELECTABLE);
}

bool HasText(UIElement* e) {
	return e->text.string.data && e->text.string.length && e->text.color.a;
}

bool HasImage(UIElement* e) {
	return e->image.atlas != 0;
}

bool IsAncestorHidden(UIElement* e) {
	while (e) {
		if (e->flags & UI_HIDDEN) return true;
		e = e->parent;
	}
	return false;
}

UIElement* GetElementByPosition(Point2i cursorPos) {
	UIElement* element = ui.rootElement;
	while (element->last) element = element->last;

	while (element) {
		if (HasQuad(element) || HasImage(element) && !IsAncestorHidden(element)) {
			Box2 box = GetScreenHitBox(element);
			if (box.x0 <= cursorPos.x && cursorPos.x <= box.x1 && box.y0 <= cursorPos.y && cursorPos.y <= box.y1)
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

void SetPosition(UIElement* element, float32 x, float32 y) {
	if (!element->parent) {
		element->pos = {x,y};
	}
	else {
		UIElement* parent = element->parent;

		if (element->flags & UI_X_MOVABLE) {
			if (x < 0) element->x = 0;
			else if (x+element->width > parent->width) element->x = parent->width-element->width;
			else element->x = x;
		}

		if (element->flags & UI_Y_MOVABLE) {
			if (y < 0) element->y = 0;
			else if (y+element->height > parent->height) element->y = parent->height-element->height;
			else element->y = y;
		}
	}
}

Dimensions2 GetContentDim(UIElement* e) {
	float32 width = 0;
	float32 height = 0;
	if (HasText(e)) {
		UIText text = e->text;
		TextMetrics metrics = text.flags & TEXT_WRAPPING
			? GetTextMetrics(text.font, text.string, e->width)
			: GetTextMetrics(text.font, text.string);
		width = metrics.maxx;
		height = metrics.endy;
	}
	LINKEDLIST_FOREACH(e, UIElement, child) {
		if (child->flags & UI_HIDDEN) continue;
		width  = MAX(child->x + child->width, width);
		height = MAX(child->y + child->height, height);
	}
	return {width, height};
}

bool IsInteractable(UIElement* e) {
	return (e->flags & (UI_CLICKABLE | UI_MOVABLE)) || e->onClick;
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
//--------

ssize GetTextIndex(UIElement* textElement, Point2 pos, Point2i cursorPos) {
	if (!HasText(textElement)) return -1;

	UIText text = textElement->text;
	float32 x_end = cursorPos.x - pos.x + textElement->scroll.x;
	float32 y_end = cursorPos.y - (pos.y + text.font->height) + textElement->scroll.y;
	return text.flags & TEXT_WRAPPING
		? GetCharIndex(text.font, text.string, x_end, y_end, textElement->width) 
		: GetCharIndex(text.font, text.string, x_end, y_end);
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
		style.borderColor = {1, 0, 0, 1};
	}

	return style;
}

Point2 GetInterpolatedPos(UIElement* e) {
	return (1 - e->transition)*e->prevPos + e->transition*e->pos;
}

Point2 GetScreenPosition(UIElement* element) {
	UIElement* parent = element->parent;
	if (parent == NULL) return {element->x, element->y};

	Point2 parentPos = GetScreenPosition(parent);
	Point2 pos = element->flags & UI_TRANSITION
		? GetInterpolatedPos(element)
		: element->pos;
	Point2 scrollPos = parent->scroll;

	return {parentPos.x + pos.x - scrollPos.x, parentPos.y + pos.y - scrollPos.y};
}

void RenderElement(UIElement* element) {
	if (element->flags & UI_HIDDEN) return;

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

		if (paneDim.width <= pane->width) scrollBar->flags |= UI_HIDDEN;
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
	if ((element->flags & UI_HORIZONTAL_STACK) && element->prev)
		x = element->prev->x + element->prev->width + element->margin;
	if ((element->flags & UI_VERTICAL_STACK) && element->prev)
		y = element->prev->y + element->prev->height + element->margin;

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
	Point2 pos = GetScreenPosition(element);
	if (HasQuad(element)) {
		UIStyle style = GetCurrentStyle(element);
		GfxDrawQuad(pos, element->dim, style.background, style.radius, style.borderWidth, style.borderColor,
			element->flags & UI_GRADIENT ? style.color2 : style.background);
	}
	if (HasText(element)) {
		UIText text = element->text;
		if (text.flags & TEXT_WRAPPING)
			RenderText(pos, text.font, text.color, text.string, pos.x + element->width);
		else
			RenderText(pos, text.font, text.color, text.string);
	}
	if (element->icon) {
		RenderGlyph(pos, &ui.iconsFont, COLOR_BLACK, element->icon);
	}
	if (HasImage(element)) {
		GfxDrawImage(pos, element->dim, element->image.atlas, element->image.crop);
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
	ui.iconsFont = LoadAndBakeIconsFont(scratch, atlas, 24);
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

UIElement* UIAddText(UIElement* parent, UIText text) {
	UIElement* textElement = UICreateElement(parent);
	textElement->text = text;
	textElement->flags = UI_FIT_CONTENT | UI_MIDDLE | UI_CENTER;
	return textElement;
}

UIElement* UIAddText(UIElement* parent, UIText text, float32 xmargin) {
	UIElement* textElement = UICreateElement(parent);
	textElement->x = xmargin;
	textElement->text = text;
	textElement->flags = UI_FIT_CONTENT;

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

void UIUpdate() {
	Point2i cursorPos = OSGetCursorPosition();
	if (ui.rootElementIsWindow) {
		Dimensions2i dim = OSGetWindowDimensions();
		ui.rootElement->dim = {(float32)dim.width, (float32)dim.height};
	}
	
	if (!ui.isResizing && !ui.isGrabbing) {
		ui.hovered = GetElementByPosition(cursorPos);
	}
	UIElement* element = ui.hovered;

	if (!element) {
		if (1 < cursorPos.x && cursorPos.x < ui.rootElement->width-1 && 1 < cursorPos.y && cursorPos.y < ui.rootElement->height-1)
			OSSetCursorIcon(CUR_ARROW);

		return;
	}

	Point2 absPos = GetScreenPosition(element);
	Point2 p1 = {absPos.x + element->width, absPos.y + element->height};
	bool isInBottomRight = p1.x - MIN(4, element->radius) <= cursorPos.x && p1.y - MIN(4, element->radius) <= cursorPos.y;

	// Handle mouse hover
	if (element->flags & UI_CURSOR) OSSetCursorIcon(element->cursor);
	else if (isInBottomRight && (element->flags & UI_RESIZABLE)) OSSetCursorIcon(CUR_RESIZE);
	else if ((element->flags & UI_MOVABLE) == UI_MOVABLE) OSSetCursorIcon(CUR_MOVE);
	else if (element->flags & UI_X_MOVABLE) OSSetCursorIcon(CUR_MOVESIDE);
	else if (element->flags & UI_Y_MOVABLE) OSSetCursorIcon(CUR_MOVEUPDN);
	else if (element->flags & UI_CLICKABLE) OSSetCursorIcon(CUR_HAND);
	else if (element->text.flags & TEXT_SELECTABLE) OSSetCursorIcon(CUR_TEXT);
	else OSSetCursorIcon(CUR_ARROW);

	// Handle left click
	if (OSIsMouseLeftClicked()) {
		if (element->flags & UI_SHUFFLABLE) {
			LINKEDLIST_MOVE_TO_LAST(element->parent, element);
		}

		if (element->onClick) {
			element->onClick(element);
		}

		if (isInBottomRight && (element->flags & UI_RESIZABLE)) {
			ui.isResizing = true;
			ui.originalPos = element->pos;
		}
		else if (element->flags & UI_MOVABLE) {
			ui.isGrabbing = true;
			ui.grabPos = {(float32)cursorPos.x, (float32)cursorPos.y};
			ui.originalPos = element->pos;
		}
		//ui.focused = NULL;
	}

	// Handle mouse released
	if (!OSIsMouseLeftButtonDown()) {
		ui.isGrabbing = false;
		ui.isResizing = false;
	}

	// Handle grabbing
	if (ui.isGrabbing && 
			(cursorPos.x != ui.grabPos.x || cursorPos.y != ui.grabPos.y)) {
		float32 newx  = ui.originalPos.x + cursorPos.x - ui.grabPos.x;
		float32 newy = ui.originalPos.y + cursorPos.y - ui.grabPos.y;
		SetPosition(element, newx, newy);
		if (element->onMove) element->onMove(element);
	}

	// Handle resizing
	if (ui.isResizing) {
		Point2 relativeCursorPos = {cursorPos.x - element->parent->x, cursorPos.y - element->parent->y};
		float32 x0 = MIN(ui.originalPos.x, relativeCursorPos.x);
		float32 y0 = MIN(ui.originalPos.y, relativeCursorPos.y);
		float32 x1 = MAX(ui.originalPos.x, relativeCursorPos.x);
		float32 y1 = MAX(ui.originalPos.y, relativeCursorPos.y);

		element->x = MAX(x0, 0);
		element->y = MAX(y0, 0);
		element->width = MIN(x1 - element->x, element->parent->width - element->x);
		element->height = MIN(y1 - element->y, element->parent->height - element->y);
	}

	// Handle scrolling
	int32 mouseWheelDelta = OSGetMouseWheelDelta();
	int32 mouseHWheelDelta = OSGetMouseHWheelDelta();
	if (mouseWheelDelta || mouseHWheelDelta) {
		UIElement* scrollable = element;
		while (scrollable && ((scrollable->flags & UI_SCROLLABLE) != UI_SCROLLABLE))
			scrollable = scrollable->parent;
		if (scrollable) {
			Dimensions2 contentDim = GetContentDim(scrollable);

			if (mouseWheelDelta) {
				scrollable->scroll.y -= mouseWheelDelta;
				if ((scrollable->flags & UI_INFINITE_SCROLL) != UI_INFINITE_SCROLL) {
					if (scrollable->scroll.y < 0) scrollable->scroll.y = 0;
					if (scrollable->scroll.y > contentDim.height - scrollable->height)
						scrollable->scroll.y = MAX(contentDim.height - scrollable->height, 0);
				}
			}

			if (mouseHWheelDelta) {
				scrollable->scroll.x += mouseHWheelDelta;
				if ((scrollable->flags & UI_INFINITE_SCROLL) != UI_INFINITE_SCROLL) {
					if (scrollable->scroll.x < 0) scrollable->scroll.x = 0;
					if (scrollable->scroll.x > contentDim.width - scrollable->width)
						scrollable->scroll.x = MAX(contentDim.width - scrollable->width, 0);
				}
			}

			// update scrollbar
			// NOTE: hmm... problematic...
			UIElement* yscrollBar = scrollable->next;
			if (yscrollBar) {
				UIElement* ythumb = yscrollBar->first;
				ythumb->y = ((yscrollBar->height - ythumb->height)*scrollable->scroll.y)/(contentDim.height - scrollable->height); 

				UIElement* xscrollBar = yscrollBar->next;
				if (xscrollBar) {
					UIElement* xthumb = xscrollBar->first;
					xthumb->x = ((xscrollBar->width - xthumb->width)*scrollable->scroll.x)/(contentDim.width - scrollable->width); 
				}
			}
		}
	}

	// Handle tab
	if (OSIsKeyPressed(KEY_TAB)) {
		if (ui.focused) {
			ui.focused = FindFirstInteractable(ui.focused);
		}
		if (!ui.focused) {
			ui.focused = FindFirstInteractable(ui.rootElement);
		}
	}

	// Handle space
	if (OSIsKeyPressed(KEY_SPACE) && ui.focused) {
		if (ui.focused->onClick) ui.focused->onClick(ui.focused);
		if (ui.focused->flags & UI_SHUFFLABLE)
			LINKEDLIST_MOVE_TO_LAST(ui.focused->parent, ui.focused);
	}

	// Handle Arrow Keys
	if (OSIsKeyDown(KEY_UP) && ui.focused) {
		if (ui.focused->flags & UI_Y_MOVABLE) {
			SetPosition(ui.focused, ui.focused->x, ui.focused->y - 1);
			if (ui.focused->onMove) ui.focused->onMove(ui.focused);
		}
	}
	if (OSIsKeyDown(KEY_DOWN) && ui.focused) {
		if (ui.focused->flags & UI_Y_MOVABLE) {
			SetPosition(ui.focused, ui.focused->x, ui.focused->y + 1);
			if (ui.focused->onMove) ui.focused->onMove(ui.focused);
		}
	}
	if (OSIsKeyDown(KEY_LEFT) && ui.focused) {
		if (ui.focused->flags & UI_X_MOVABLE) {
			SetPosition(ui.focused, ui.focused->x - 1, ui.focused->y);
			if (ui.focused->onMove) ui.focused->onMove(ui.focused);
		}
	}
	if (OSIsKeyDown(KEY_RIGHT) && ui.focused) {
		if (ui.focused->flags & UI_X_MOVABLE) {
			SetPosition(ui.focused, ui.focused->x + 1, ui.focused->y);
			if (ui.focused->onMove) ui.focused->onMove(ui.focused);
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
		UI_CLICKABLE | UI_SHUFFLABLE);
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
	UIElement* scrollPane = UICreateElement(container, {}, dim, style, UI_SCROLLABLE);

	UIElement* yscrollBar = UICreateElement(container, {dim.width + 4, 6}, {7, dim.height - 12}, {{0, 0, 0, 0.25f}, 3});
	UIElement* ythumb = UICreateElement(yscrollBar, {}, {7, 14}, {{1, 1, 1, 0.33f}, 3}, UI_Y_MOVABLE | UI_Y_THUMB);
	UIEnableHoverStyle(ythumb, {{1, 1, 1, 0.5f}, 3});
	UISetCursor(ythumb, CUR_ARROW);
	ythumb->onMove = __scroll_y;

	UIElement* xscrollBar = UICreateElement(container, {6, dim.height + 4}, {dim.width - 12, 7}, {{0, 0, 0, 0.25f}, 3});
	UIElement* xthumb = UICreateElement(xscrollBar, {}, {14, 7}, {{1, 1, 1, 0.33f}, 3}, UI_X_MOVABLE | UI_X_THUMB);
	UIEnableHoverStyle(xthumb, {{1, 1, 1, 0.5f}, 3});
	UISetCursor(xthumb, CUR_ARROW);
	xthumb->onMove = __scroll_x;

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