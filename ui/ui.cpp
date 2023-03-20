/*
 * TODO:
 * Previous version supported editable text
 * accordion, tree-view, splitter
 * drop-down, menu-bar, list-box, combo-box, spinner
 * tooltip, context-menu
 */

#define UI_X_MOVABLE		(1ull << 0)
#define UI_Y_MOVABLE		(1ull << 1)
#define UI_MOVABLE			(UI_X_MOVABLE | UI_Y_MOVABLE)
#define UI_RESIZABLE		(1ull << 2)
#define UI_SHUFFLABLE		(1ull << 3)
#define UI_CLICKABLE		(1ull << 4)
#define UI_HIDE_OVERFLOW	(1ull << 5)
#define UI_SCROLLABLE		((1ull << 6) | UI_HIDE_OVERFLOW)
#define UI_INFINITE_SCROLL  ((1ull << 7) | UI_SCROLLABLE)

#define UI_CENTER			(1ull << 10)
#define UI_MIDDLE			(1ull << 11)
#define UI_FIT_CONTENT		(1ull << 14)
#define UI_MIN_CONTENT		(1ull << 15)
#define UI_ELEVATOR 		(1ull << 16)

#define UI_GRADIENT 		(1ull << 32)
#define UI_HOVER_STYLE		(1ull << 33)
#define UI_ACTIVE_STYLE		(1ull << 34)

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
};

struct UIImage {
	TextureId atlas;
	Box2 crop;
};

struct UIElement {
	union {
		union {
			struct {Point2 pos; Dimensions2 dim;};
			struct {float32 x, y, width, height;};
		};
	};

	Point2 scroll;

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
	UIStyle focused;

	union {	
		bool isChecked;
		bool isSelected;
		bool isActive;
	};

	UIText text;
	byte icon;
	UIImage image;

	void (*onClick)(UIElement*);
	void (*onMove)(UIElement*);

	opaque64 context;

	UIElement* parent;
	UIElement* first;
	UIElement* last;
	UIElement* prev;
	UIElement* next;
};

struct {
	FixedSize allocator;
	BakedFont iconsFont;
	byte icons[Icon_count];

	UIElement* rootElement;

	UIElement* hovered;
	UIElement* focused;

	Point2 originalPos;
	Point2 grabPos;

	bool isGrabbing;
	bool isResizing;
	bool rootElementIsWindow;
} ui;

// Util
//-------------

Point2 GetScreenPosition(UIElement* element) {
	UIElement* parent = element->parent;
	if (parent == NULL) return {element->x, element->y};

	Point2 parentPos = GetScreenPosition(parent);
	return {parentPos.x + element->x - parent->scroll.x, parentPos.y + element->y - parent->scroll.y};
}

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
bool HasQuad(UIElement* e) {
	return (e->background.a) || (e->borderWidth && e->borderColor.a) || (e->flags & UI_CLICKABLE);
}

bool HasText(UIElement* e) {
	return e->text.string.data && e->text.string.length && e->text.color.a;
}

bool HasImage(UIElement* e) {
	return e->image.atlas != 0;
}

UIElement* GetElementByPosition(Point2i cursorPos) {
	UIElement* element = ui.rootElement;
	while (element->last) element = element->last;

	while (element) {
		if (HasQuad(element) || HasImage(element)) {
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
		StringNode node = {e->text.string, NULL, NULL};
		StringList list = {&node, &node, e->text.string.length};
		TextMetrics metrics = GetTextMetrics(e->text.font, list);
		width = metrics.maxx;
		height = metrics.endy;
	}
	LINKEDLIST_FOREACH(e, UIElement, child) {
		//if (child->flags & UI_ADDENDUM) continue;
		width  = MAX(child->x + child->width, width);
		height = MAX(child->y + child->height, height);
	}
	return {width, height};
}

// Render
//------------

UIStyle GetCurrentStyle(UIElement* element) {
	if ((element->flags & UI_HOVER_STYLE) && ui.hovered == element)
		return element->hover;

	if ((element->flags & UI_ACTIVE_STYLE) && element->isActive)
		return element->active;

	return element->style;
}

void RenderElement(UIElement* element) {
	UIElement* parent = element->parent;
	Dimensions2 contentDim = GetContentDim(element);

	if (element->flags & UI_FIT_CONTENT) 
		element->dim = contentDim;
	if (element->flags & UI_MIN_CONTENT) 
		element->dim = {MAX(element->width, contentDim.width), MAX(element->height, contentDim.height)};
	if (element->flags & UI_CENTER) element->x = MAX((parent->width - element->width)/2.0f, 0);
	if (element->flags & UI_MIDDLE) element->y = MAX((parent->height - element->height)/2.0f, 0);
	if (element->flags & UI_ELEVATOR) {
		UIElement* scrollBar = element->parent;
		UIElement* pane = scrollBar->prev;
		Dimensions2 paneDim = GetContentDim(pane);

		element->height = (pane->height*scrollBar->height)/paneDim.height;
	}

	Point2 pos = GetScreenPosition(element);

	if (HasQuad(element)) {
		UIStyle style = GetCurrentStyle(element);
		GfxDrawQuad(pos, element->dim, style.background, style.radius, style.borderWidth, style.borderColor,
			element->flags & UI_GRADIENT ? style.color2 : style.background);
	}
	if (HasText(element))
		RenderText(pos, element->text.font, element->text.color, element->text.string);
	if (element->icon) {
		RenderGlyph(pos, &ui.iconsFont, {0, 0, 0, 1}, element->icon);
	}
	if (HasImage(element)) {
		GfxDrawImage(pos, element->dim, element->image.atlas, element->image.crop);
	}

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
	for (byte i = 0; i < Icon_count; i++) ui.icons[i] = i;
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
		uint32 flags = 0) {

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

UIElement* UIAddText(UIElement* parent, UIText text, float32 x) {
	UIElement* textElement = UICreateElement(parent);
	textElement->x = x;
	textElement->text = text;
	textElement->flags = UI_FIT_CONTENT | UI_MIDDLE;
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
	if (isInBottomRight && (element->flags & UI_RESIZABLE)) OSSetCursorIcon(CUR_RESIZE);
	else if ((element->flags & UI_MOVABLE) == UI_MOVABLE) OSSetCursorIcon(CUR_MOVE);
	else if (element->flags & UI_X_MOVABLE) OSSetCursorIcon(CUR_MOVESIDE);
	else if (element->flags & UI_CLICKABLE) OSSetCursorIcon(CUR_HAND);
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
		while (scrollable && !(scrollable->flags & UI_SCROLLABLE))
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
			// TODO: horizontal update as well
			UIElement* scrollBar = scrollable->next;
			UIElement* thumb = scrollBar->first;

			thumb->y = ((scrollBar->height - thumb->height)*scrollable->scroll.y)/(contentDim.height - scrollable->height); 
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
	UIEnableGradient(button, {
		style.background.r*0.5f + 0.5f, 
		style.background.g*0.5f + 0.5f, 
		style.background.b*0.5f + 0.5f,
		style.background.a});

	UIEnableHoverStyle(button, {button->color2, style.radius, style.borderWidth, style.borderColor, style.background});
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
	UIElement* checkbox = UICreateElement(parent, pos);
	checkbox->icon = Icon_check_empty;
	checkbox->isChecked = false;
	checkbox->flags = UI_FIT_CONTENT | UI_CLICKABLE;
	checkbox->onClick = __check_box;

	return checkbox;
}

void __select_button(UIElement* e) {
	UIElement* radioGroup = e->parent;
	LINKEDLIST_FOREACH(radioGroup, UIElement, button) {
		if (button == e) {
			button->isChecked = true;
			button->icon = Icon_dot_circled;
		}
		else {
			button->isChecked = false;
			button->icon = Icon_circle_empty;
		}
	}
}

UIElement* UICreateRadioButton(UIElement* radioGroup, Point2 pos = {}) {
	UIElement* radioButton = UICreateElement(radioGroup, pos);
	radioButton->icon = Icon_circle_empty;
	radioButton->isChecked = false;
	radioButton->flags = UI_FIT_CONTENT | UI_CLICKABLE;
	radioButton->onClick = __select_button;

	return radioButton;
}

UIElement* UICreateSlider(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style, float32 thumbWidth) {
	float32 half_height = dim.height/2.f;
	float32 quater_height = half_height/2.f;
	UIElement* slider = UICreateElement(parent, {pos.x, pos.y - quater_height}, {dim.width, half_height}, style);
	UICreateElement(slider, {(dim.width - thumbWidth)/2.f, -quater_height}, {thumbWidth, dim.height}, style, 
		UI_X_MOVABLE);

	return slider;
}

UIElement* UICreateVSlider(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style, float32 thumbHeight) {
	float32 half_width = dim.width/2.f;
	float32 quater_width = half_width/2.f;
	UIElement* slider = UICreateElement(parent, {pos.x - quater_width, pos.y}, {half_width, dim.height}, style);
	UICreateElement(slider, {-quater_width, (dim.height - thumbHeight)/2.f}, {dim.width, thumbHeight}, style, 
		UI_Y_MOVABLE);

	return slider;
}

void __activate_tab(UIElement* e) {
	UIElement* tabControl = e->parent;
	LINKEDLIST_FOREACH(tabControl, UIElement, header) {
		header->isActive = header == e;
	}
}

UIElement* UICreateTab(UIElement* tabControl, Dimensions2 dim, UIStyle style, UIText header) {
	Color bg = style.background;
	UIStyle nonactive = {{
		0.5f*bg.r + 0.5f,
		0.5f*bg.g + 0.5f,
		0.5f*bg.b + 0.5f,
		bg.a,
	}, style.radius, style.borderWidth, style.borderColor};
	UIStyle hover = {{
		0.75f*bg.r + 0.25f,
		0.75f*bg.g + 0.25f,
		0.75f*bg.b + 0.25f,
		bg.a,
	}, style.radius, style.borderWidth, style.borderColor};
	UIStyle active = style;

	UIElement* lastTab = tabControl->last;
	float32 x = lastTab ? lastTab->x + lastTab->width : 0;
	UIElement* tab = UICreateElement(tabControl, {x, 2}, {dim.width, dim.height + style.radius}, nonactive, 
		UI_CLICKABLE | UI_SHUFFLABLE);
	UIAddText(tab, header);
	UIEnableHoverStyle(tab, hover);
	UIEnableActiveStyle(tab, active);
	tab->onClick = __activate_tab;
	__activate_tab(tab);

	UIElement* tabBody = UICreateElement(tab, {-x, dim.height}, {tabControl->width, tabControl->height - dim.height - 2}, active);
	return tabBody;
}

void __scroll(UIElement* e) {
	UIElement* scrollBar = e->parent;
	UIElement* pane = scrollBar->prev;
	Dimensions2 contentDim = GetContentDim(pane);
	float32 y = ((contentDim.height - pane->height)*e->y)/(scrollBar->height - e->height); 
	pane->scroll.y = y;
}

UIElement* UICreateScrollPane(UIElement* parent, Point2 pos, Dimensions2 dim, UIStyle style) {
	// TODO: hide scrollbar by default, and only show it when content is bigger than pane.
	// TODO: also add horizontal scrollbar
	UIElement* container = UICreateElement(parent, pos, {dim.width + 15, dim.height});
	UIElement* scrollPane = UICreateElement(container, {}, dim, style, UI_SCROLLABLE);
	UIElement* scrollBar = UICreateElement(container, {dim.width + 4, 6}, {7, dim.height - 12}, {{0, 0, 0, 0.5f}, 3});
	UIElement* thumb = UICreateElement(scrollBar, {}, {7, 14}, {{1, 1, 1, 0.5f}, 3}, UI_Y_MOVABLE | UI_ELEVATOR);
	thumb->onMove = __scroll;

	return scrollPane;
}