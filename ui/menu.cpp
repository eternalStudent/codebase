struct UIMenuItem {
	Box2i hitBox;
	UIMenuItem* parent;
	UIMenuItem* children;
	UIMenuItem* opened;
	int32 count;
};

struct UIMenu {
	UIMenuItem root;
	UIMenuItem* hovered;
	UIMenuItem* focused;
	UIMenuItem* clicked;
};

bool InBounds(Box2i bounds, Point2i p) {
	return bounds.x0 <= p.x && p.x < bounds.x1 && bounds.y0 <= p.y && p.y < bounds.y1;
}

UIMenuItem* GetMenuItemByPosition(UIMenuItem* parent, Point2i pos) {
	for (int32 i = 0; i < parent->count; i++) {
		UIMenuItem* item = parent->children + i;
		if (InBounds(item->hitBox, pos))
			return item;
	}
	if (parent->opened) {
		return GetMenuItemByPosition(parent->opened, pos);
	}
	return NULL;
}

void CloseEverything(UIMenuItem* parent) {
	if (parent->opened) CloseEverything(parent->opened);
	parent->opened = NULL;
}

bool IsAncestorOf(UIMenuItem* a, UIMenuItem* b) {
	if (b == NULL) 
		return false;

	if (b == a) 
		return true;

	return IsAncestorOf(a, b->parent);
}

bool UIMenuProcessEvent(OSEvent event, UIMenu* menu) {
	switch (event.type) {

	case Event_MouseMove: {
		Point2i cursor = event.mouse.cursorPos;
		menu->hovered = GetMenuItemByPosition(&menu->root, cursor);
		if (menu->hovered) {

			menu->focused = NULL;

			if (menu->hovered->parent->opened) {
				if (menu->hovered->count)
					menu->hovered->parent->opened = menu->hovered; 
				else
					menu->hovered->parent->opened = NULL;
			}

			if (!IsAncestorOf(menu->root.opened, menu->hovered))
				CloseEverything(&menu->root);

			return true;
		}

		return false;
	} break;

	case Event_MouseLeftButtonDown: {
		if (menu->hovered) {
			if (menu->hovered->parent->opened == menu->hovered) {
				menu->hovered->parent->opened = NULL;
			}
			else if (menu->hovered->count) {
				menu->hovered->parent->opened = menu->hovered;
			}
			else {
				menu->clicked = menu->hovered;
				CloseEverything(&menu->root);
			}
			menu->focused = menu->hovered;
			return true;
		}
		else {
			CloseEverything(&menu->root);
			return false;
		}
	} break;

	case Event_KeyboardPress: {
		if (event.keyboard.vkCode == KEY_ALT) {
			if (menu->focused) {
				menu->focused = NULL;
				CloseEverything(&menu->root);
			}
			else
				menu->focused = menu->root.children;
			return true;
		}

		else if (menu->focused) {
			if (event.keyboard.vkCode == KEY_ENTER) {
				if (menu->focused->count) {
					menu->focused->parent->opened = menu->focused;
					menu->focused = menu->focused->children;
				}
				else {
					menu->clicked = menu->focused;
				}
				return true;
			}
			else if (event.keyboard.vkCode == KEY_LEFT) {
				if (menu->focused->parent == &menu->root) {
					if (menu->focused - 1 >= menu->root.children) {
						menu->focused->parent->opened = NULL;
						menu->focused = menu->focused - 1;
					}
					else {
						menu->focused = menu->root.children + menu->root.count - 1;
					}
				}
				else if (menu->focused->parent->parent == &menu->root) {
					if (menu->root.opened - 1 >= menu->root.children) {
						UIMenuItem* prev = menu->root.opened - 1;
						if (prev->count) {
							menu->root.opened = prev;
							menu->focused = prev->children;
						}
						else {
							menu->root.opened = NULL;
							menu->focused = prev;
						}
					}
				}
				else {
					menu->focused = menu->focused->parent;
					menu->focused->parent->opened = NULL;
				}
				return true;
			}
			else if (event.keyboard.vkCode == KEY_RIGHT) {
				if (menu->focused->parent == &menu->root) {
					if (menu->focused + 1 < menu->root.children + menu->root.count) {
						menu->focused->parent->opened = NULL;
						menu->focused = menu->focused + 1;
					}
					else {
						menu->focused = menu->root.children;
					}
				}
				else {
					if (menu->focused->count) {
						menu->focused->parent->opened = menu->focused;
						menu->focused = menu->focused->children;
					}
					else if (menu->root.opened + 1 < menu->root.children + menu->root.count) {
						UIMenuItem* next = menu->root.opened + 1;
						if (next->count) {
							menu->root.opened = next;
							menu->focused = next->children;
						}
						else {
							menu->root.opened = NULL;
							menu->focused = next;
						}
					}
				}
				return true;
			}
			else if (event.keyboard.vkCode == KEY_UP) {
				if (menu->focused->parent == &menu->root) {
					// nothing
				}
				else {
					if (menu->focused - 1 >= menu->focused->parent->children) {
						menu->focused = menu->focused - 1;
					}
					else {
						menu->focused = menu->focused->parent->children + menu->focused->parent->count - 1;
					}
				}
				return true;
			}
			else if (event.keyboard.vkCode == KEY_DOWN) {
				if (menu->focused->parent == &menu->root) {
					if (menu->focused->count) {
						menu->focused->parent->opened = menu->focused;
						menu->focused = menu->focused->children;
					}
				}
				else {
					if (menu->focused + 1 < menu->focused->parent->children + menu->focused->parent->count) {
						menu->focused = menu->focused + 1;
					}
					else {
						menu->focused = menu->focused->parent->children;
					}
				}
				return true;
			}
			else if (event.keyboard.vkCode == KEY_ESC) {
				if (menu->focused->parent == &menu->root) {
					menu->focused = NULL;	
				}
				else {
					menu->focused = menu->focused->parent;
					menu->focused->parent->opened = NULL;
				}
				return true;
			}
		}
	} break;
	}

	return false;
}