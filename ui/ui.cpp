#define UI_FLIPY(y)        (ui.windowElement->height-(y)-1)         

#define UI_MOVABLE			(1 << 0)
#define UI_RESIZABLE		(1 << 1)
#define UI_CLICKABLE		(1 << 2)
#define UI_HIDDEN			(1 << 3)
#define UI_CENTER			(1 << 4)
#define UI_RIGHT			(1 << 5)
#define UI_HIDE_OVERFLOW 	(1 << 6)
#define UI_SCROLLABLE		((1 << 7) + UI_HIDE_OVERFLOW)
#define UI_FITTEXT			(1 << 8)

#include "font.cpp"
#include "clipboard.cpp"

struct UIText {
	union {
		Point2i pos;
		struct {int32 x, y;};
	};

	uint32 color;
	Font* font;
	String string;
};

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

struct UIImage {
	union {
		struct {int32 x, y, width, height;};
		struct {Point2i pos; Dimensions2i dim;};
		UIBox box;
	};

	TextureId atlas;
	Box2 crop;
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

	// TODO: add cursorIcon
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

	UIText* text;
	UIImage* image;
	opaque64 context;

	Point2i scrollPos;
};

struct {
	FixedSize allocator;
	int32 capacity;
	int32 elementCount;

	UIStyle originalStyle;

	UIElement* windowElement;

	UIElement* active;
	Point2i originalPos;
	Point2i grabPos;

	bool isGrabbing;
	bool isResizing;
	bool isSelecting;

	UIText* selected;
	int32 start;
	int32 end;

	Arena* arena;
} ui;

Box2i GetAbsolutePosition(UIElement* element) {
	if (element == NULL) return {0, 0, ui.windowElement->width, ui.windowElement->height};
	UIElement* parent = element->parent;
	if (parent == NULL) return {0, 0, element->width, element->height};
	Box2i parentPos = GetAbsolutePosition(parent);

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

bool IsInTextBound(UIText* text, Box2i box, Point2i pos) {
	if (!text) return false;
	int32 lineCount;
	Box2i textBox = {
		box.x0 + text->x, 
		box.y0 + text->y, 
		box.x0 + text->x + (int32)(GetTextWidth(text->font, text->string, &lineCount) + 0.5f), 
		box.y0 + text->y + (int32)(lineCount*text->font->height + 0.5f)
	};
	return INSIDE2(textBox, pos);
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

UIElement* GetElementByPosition(Point2i p) {
	UIElement* element = ui.windowElement;
	while (element->last) element = element->last;

	while (element) {
		if (!(element->flags & UI_HIDDEN)) {
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

void MoveToFront(UIElement* element) {
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
	LINKEDLIST_FOREACH(e, UIElement, child) {
		width  = MAX(child->x + child->width, width);
		height = MAX(child->y + child->height, height);
	}
	return {width, height};
}

void RenderText(UIText* text, Point2i parentPos) {
	if (!text) return;
	String string = text->string;
	Font* font = text->font;

	float32 x = (float32)(text->pos.x + parentPos.x);
	float32 y = UI_FLIPY((float32)(text->pos.y + parentPos.y)) - font->height;
	RenderText(font, x, y, string, text->color);
	
	// Handle selected text
	if (ui.selected == text) {
		int32 start = MIN(ui.start, ui.end);
		int32 end = MAX(ui.start, ui.end);
		ASSERT(0 <= start && start <= end && end <= string.length);
		int32 i = 0;
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
      		else {
      			ASSERT(32 <= b && b <= 127);
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
					GfxDrawBox2(0x44ffffff, {x0, y1 - font->height, x1, y1});
				if (i == end) 
					break;
				x0 = x;
				x1 = x;
				y1 -= font->height;
			}
			else {
				ASSERT(32 <= b && b <= 127);
        		BakedChar* bakedchar = font->chardata + (b - 32);
         		x1 += bakedchar->xadvance;
         	}
      		i++;
		}
		
		
		if (ui.start < ui.end)
			GfxDrawBox2(RGBA_ORANGE, {x1-1, y1 - font->height-2, x1+1, y1+2});
		else
			GfxDrawBox2(RGBA_ORANGE, {x0-1, y1 - font->height-2, x0+1, y1+2});
	}
}

void RenderImage(UIImage* image, Point2i parentPos) {
	if (!image) return;

	Point2i p0 = MOVE2(image->pos, parentPos);
	Box2 renderBox = Box2{(float32)p0.x, (float32)UI_FLIPY(p0.y+image->height), (float32)(p0.x+image->width), (float32)UI_FLIPY(p0.y)};
	GfxDrawImage(image->atlas, image->crop, renderBox);
}

int32 __center(UIElement* element) {
	return (element->parent->width - element->width)/2;
}

int32 __fit_text(UIElement* element) {
	int32 lineCount;
	return element->text->x + (int32)(GetTextWidth(element->text->font, element->text->string, &lineCount) + 0.5f);
}

void RenderElement(UIElement* element) {
	if (element->flags & UI_HIDDEN) return;
	if (element->flags & UI_CENTER) element->x = __center(element);
	if (element->flags & UI_FITTEXT) element->width = __fit_text(element);
	Box2i box = GetAbsolutePosition(element);
	Box2 renderBox = Box2{(float32)box.x0, (float32)UI_FLIPY(box.y1), (float32)box.x1, (float32)UI_FLIPY(box.y0)};
	if (element->radius) {
		GfxDrawBox2Rounded(element->background, renderBox, element->radius);
		if (element->borderWidth && element->borderColor) 
			GfxDrawBox2RoundedLines(element->borderColor, element->borderWidth, renderBox, element->radius);
	}
	else {
		GfxDrawBox2(element->background, renderBox);
		if (element->borderWidth && element->borderColor) 
			GfxDrawBox2Lines(element->borderColor, element->borderWidth, renderBox);
	}
	// TODO: this should become its own element
	if (element->flags & UI_SCROLLABLE) {
		Dimensions2i contentDim = GetContentDim(element);
		if (element->height < contentDim.height) {
			int32 length = (element->height*element->height)/contentDim.height;
			int32 pos = ((element->height - length)*element->scrollPos.y)/(contentDim.height - element->height); 
			Box2 elevator = {(float32)box.x1 - 9, 
				(float32)UI_FLIPY(box.y0 + pos + length), 
				(float32)box.x1 - 2, 
				(float32)UI_FLIPY(box.y0 + pos)};
			GfxDrawBox2Rounded(0x44000000, elevator, 3);
		}
	}
	if (element->flags & UI_HIDE_OVERFLOW) GfxCropScreen(box.x0, UI_FLIPY(box.y1), element->width, element->height);
	LINKEDLIST_FOREACH(element, UIElement, child) RenderElement(child);
	RenderText(element->text, box.p0);
	RenderImage(element->image, box.p0);
	if (element->flags & UI_HIDE_OVERFLOW) GfxClearCrop();
}

// API
//-----------

void UIInit(Arena* persist, Arena* scratch) {
	int32 capacity = 200;
	ui.allocator = CreateFixedSize(persist, capacity, sizeof(UIElement));
	ui.capacity = capacity;
	ui.arena = scratch;
}

void UIRenderElements() {
	LINKEDLIST_FOREACH(ui.windowElement, UIElement, child) RenderElement(child);
	RenderText(ui.windowElement->text, {0, 0});
	RenderImage(ui.windowElement->image, {0, 0});	
}

void UISetWindowElement(uint32 background) {
	ui.elementCount = 1;
	ui.windowElement = (UIElement*)FixedSizeAlloc(&ui.allocator);
	ui.windowElement->pos = {0, 0};
	ui.windowElement->background = background;
	GfxSetColor(background);
}

void UICreateWindow(const char* title, Dimensions2i dimensions, uint32 background) {
	OSCreateWindow(title, dimensions.width, dimensions.height);
	GfxInit(ui.arena);
	UISetWindowElement(background);
}

void UICreateWindowFullScreen(const char* title, uint32 background) {
	OSCreateWindowFullScreen(title);
	GfxInit(ui.arena);
	UISetWindowElement(background);
}

UIElement* UICreateElement(UIElement* parent) {
	if (ui.elementCount == ui.capacity) return NULL;
	UIElement* element = (UIElement*)FixedSizeAlloc(&ui.allocator);

	ui.elementCount++;

	if (parent == NULL) parent = ui.windowElement;
	LINKEDLIST_ADD(parent, element);
	element->parent = parent;
	return element;
}

void UIDestroyElement(UIElement* element) {
	ui.elementCount--;

	LINKEDLIST_REMOVE(element->parent, element);
	LINKEDLIST_FOREACH(element, UIElement, child) UIDestroyElement(child);

	FixedSizeFree(&ui.allocator, element->text);
	FixedSizeFree(&ui.allocator, element->image);
	FixedSizeFree(&ui.allocator, element);
}

UIText* UICreateText(UIElement* parent) {
	if (ui.elementCount == ui.capacity) return NULL;
	UIText* text = (UIText*)FixedSizeAlloc(&ui.allocator);
	ui.elementCount++;

	if (parent) parent->text = text;
	else ui.windowElement->text = text;
	return text;
}

UIImage* UICreateImage(UIElement* parent) {
	if (ui.elementCount == ui.capacity) return NULL;
	UIImage* image = (UIImage*)FixedSizeAlloc(&ui.allocator);
	ui.elementCount++;

	if (parent) {
		parent->image = image;
		image->dim = parent->dim;
	}
	else ui.windowElement->image = image;
	return image;
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
	bool isInTextBound = false;
	if (element) {
		if(IsInBottomRight(pos, cursorPos) && (element->flags & UI_RESIZABLE)) {
			OSSetCursorIcon(CUR_RESIZE);
			isInBottomRight = true;
		}
		else if (IsInTextBound(element->text, pos, cursorPos)) {
			OSSetCursorIcon(CUR_TEXT);
			isInTextBound = true;
		}
		else if (element->onHover) element->onHover(element);
		else if (element->flags & UI_CLICKABLE) OSSetCursorIcon(CUR_HAND);
		else if (element->flags & UI_MOVABLE) OSSetCursorIcon(CUR_MOVE);
		else OSSetCursorIcon(CUR_ARROW);
	}
	else if (1 < cursorPos.x && cursorPos.x < ui.windowElement->width-1 && 1 < cursorPos.y && cursorPos.y < ui.windowElement->height-1)
		OSSetCursorIcon(CUR_ARROW);

	// Handle mouse pressed
	if (OSIsMousePressed(MOUSE_L)) {
		ui.selected = NULL;
		if (element) {
			MoveToFront(element);
			if (element->flags & UI_CLICKABLE) {
				if (element->onClick)
					element->onClick(element);
			}
			else if (isInTextBound) {
				float32 x_end = (float32)(cursorPos.x - (pos.x0 + element->text->x));
				float32 y_end = (float32)(cursorPos.y - (pos.y0 + element->text->y + element->text->font->height));
				ui.end = GetCharIndex(element->text->font, element->text->string, x_end, y_end);
				ui.start = ui.end;
				ui.selected = element->text;
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
	if (element && OSIsMouseDoubleClicked() && isInTextBound) {
		ui.selected = element->text;
		ui.end = (int32)element->text->string.length;
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
	if (mouseWheelDelta) {
		UIElement* scrollable = GetScrollableAnscestor(element);
		if (scrollable) {
			Dimensions2i contentDim = GetContentDim(scrollable);
			scrollable->scrollPos.y += mouseWheelDelta;
			if (scrollable->scrollPos.y < 0) scrollable->scrollPos.y = 0;
			if (scrollable->scrollPos.y > contentDim.height - scrollable->height)
				scrollable->scrollPos.y = contentDim.height - scrollable->height;
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
		ASSERT(element && element->text);
		float32 x_end = (float32)(cursorPos.x - (pos.x0 + element->text->x));
		float32 y_end = (float32)(cursorPos.y - (pos.y0 + element->text->y + element->text->font->height));
		ui.end = GetCharIndex(element->text->font, element->text->string, x_end, y_end);
	}

	// Handle arrow keys
	if (OSIsKeyPressed(KEY_LEFT)) {
		if (ui.selected) {
			ui.end = MAX(0, ui.end - 1);
			if (!OSIsKeyDown(KEY_SHIFT))
				ui.start = ui.end;
		}
	}
	if (OSIsKeyPressed(KEY_RIGHT)) {
		if (ui.selected) {
			ui.end = MIN(ui.end + 1, (int32)ui.selected->string.length);
			if (!OSIsKeyDown(KEY_SHIFT))
				ui.start = ui.end;
		}
	}

	// Handle copy
	if (OSIsKeyDown(KEY_CTRL) && OSIsKeyPressed(KEY_C) && ui.selected) {
		String string = ui.selected->string;
		int32 start = MIN(ui.start, ui.end);
		int32 end = MAX(ui.start, ui.end);
		String sub = {string.data + start, end - start};
		OSCopyToClipboard(sub);
	}

	return element;
}

void UIDrawLine(Point2i p0, Point2i p1, uint32 rgba, float32 lineWidth) {
	Point2 points[2] = {{(float32)p0.x, (float32)UI_FLIPY(p0.y)}, {(float32)p1.x, (float32)UI_FLIPY(p1.y)}};
	Line2 line = {points, 2};
	GfxDrawLine(rgba, lineWidth, line);
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

UIElement* UICreateCheckbox(UIElement* parent, Font* font, String str, byte* context) {
	UIElement* wrapper = UICreateElement(parent);
	int32 lineCount;
	wrapper->width = 37+(int32)GetTextWidth(font, str, &lineCount);
	// TODO: handle line count
	wrapper->height = 24;
	wrapper->name = STR("wrapper");

	UIElement* checkbox = UICreateElement(wrapper);
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

	UIText* text = UICreateText(wrapper);
	text->pos = {37, -4};
	text->font = font;
	text->string = str;

	return wrapper;
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

void __hover(UIElement* e) {
	e->background = RGBA_GREY;
	OSSetCursorIcon(CUR_HAND);
}

UIElement* UICreateButton(UIElement* parent, Dimensions2i dim) {
	UIElement* button = UICreateElement(parent);
	button->dim = dim;
	button->background = RGBA_LIGHTGREY;
	button->radius = 12.0f;
	button->borderWidth = 1;
	button->borderColor = RGBA_WHITE;
	button->onHover = __hover;
	button->flags = UI_CLICKABLE;
	button->name = STR("button");
	return button;
}

void __choose(UIElement* e) {
	UIElement* parent = e->parent;
	LINKEDLIST_FOREACH(parent, UIElement, child) {
		child->background = RGBA_LIGHTGREY;
		child->onHover = __hover;
	}
	e->background = RGBA_DARKGREY;
	e->onHover = NULL;
	ui.originalStyle = e->style;
}

void __fit(UIElement* e) {
	LINKEDLIST_FOREACH(e, UIElement, child) {
		Dimensions2i dim = child->dim;
		UIElement* body = child->first;
		body->height = e->height - (dim.height+1);
    	body->width = e->width-2;
	}
}

UIElement* UICreateTabControl(UIElement* parent, Dimensions2i dim) {
	UIElement* control = UICreateElement(parent);
	control->dim = dim;
	control->background = RGBA_LIGHTGREY;
	control->flags = UI_MOVABLE | UI_RESIZABLE;
	control->borderColor = RGBA_WHITE;
	control->borderWidth = 1;
	control->onResize = __fit;
	control->name = STR("tab control");

	return control;
}

UIElement* UICreateTab(UIElement* parent, Dimensions2i dim, String title, Font* font) {
	UIElement* last = parent->last;

	UIElement* header = UICreateElement(parent);
	header->x = last ? last->x+last->width+1 : 24;
	header->dim = dim;
	header->radius = 3;
	header->flags = UI_CLICKABLE;
	header->onHover = __hover;
	header->onClick = __choose;
	header->background = RGBA_LIGHTGREY;
	header->name = STR("tab header");

	UIText* text = UICreateText(header);
    text->string = title;
    text->x = 3;
    text->font = font;

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
	MoveToFront(active->parent);
	__choose(active->parent);
}