enum UIAlignement {
	Align_Unaligned,
	Align_Left,
	Align_Top,
	Align_Right,
	Align_Bottom,
	Align_Center
};

enum UIPanelHover {
	UIPH_None,
	UIPH_Left,
	UIPH_Right,
	UIPH_Top,
	UIPH_Bottom,
	UIPH_BottomRight,
	UIPH_Splitter,
	UIPH_Header,
	UIPH_Closing
};

enum UIPanelState {
	UIPS_Rest,
	UIPS_Resize,
	UIPS_Drag,
};

// TODO: add min dim
struct UIPanel {
	union {
		struct {Point2 pos; Dimensions2 dim;};
		struct {float32 x, y, width, height;};
	};

	UIAlignement align;
	float32 splitPoint;

	void* data;

	union {
		struct {UIPanel *next, *prev;};
		struct {UIPanel *a, *b, *parent;};
	};

	struct {
		UIPanel* first;
		UIPanel* last;
		UIPanel* selected;
		int32 count;
	} tabs;
};

struct UIPanelManager;

typedef void GetDockProc(UIPanelManager* manager, Point2 cursor, UIPanel** dockPanel, UIAlignement* dock);

// TODO: my current solution leaves too much work for the usage side
//       need to find a better solution to specify hitboxes and to set active and hover
typedef void SetActivePanelProc(UIPanelManager* manager, Point2 cursor);

struct UIPanelManager {
	struct {
		UIPanel* first;
		UIPanel* last;
	} floating;

	UIPanel* root;

	UIPanel* active;
	UIPanelHover hover;
	UIPanelState state;

	Point2 grabPos;

	GetDockProc* GetDock;
	SetActivePanelProc* SetActivePanel;
	FixedSize allocator;
};

void UIPanelSplit(FixedSize* allocator, UIPanel* parent, UIAlignement align, float32 splitPoint) {
	parent->a = (UIPanel*)FixedSizeAlloc(allocator);
	parent->b = (UIPanel*)FixedSizeAlloc(allocator);
	parent->align = align;
	parent->splitPoint = splitPoint;
	parent->a->parent = parent;
	parent->b->parent = parent;

	parent->b->data = parent->data;
	parent->data = NULL;
	parent->b->tabs = parent->tabs;
	LINKEDLIST_FOREACH(&(parent->b->tabs), UIPanel, tab) tab->parent = parent->b;
}

void UIPanelSplit(FixedSize* allocator, UIPanel* parent, UIAlignement align, UIPanel* panel) {
	float32 dim = align == Align_Left || align == Align_Right ? panel->width : panel->height;
	float32 parentDim = align == Align_Left || align == Align_Right ? parent->width : parent->height;

	UIPanel* b = (UIPanel*)FixedSizeAlloc(allocator);	
	if (parent->align == Align_Unaligned) {
		b->data = parent->data;
		parent->data = NULL;
		b->tabs = parent->tabs;
		LINKEDLIST_FOREACH(&(b->tabs), UIPanel, tab) tab->parent = b;
	}
	else {
		b->align = parent->align;
		b->splitPoint = parent->splitPoint;
		b->data = parent->data;
		b->tabs = parent->tabs;
		LINKEDLIST_FOREACH(&(b->tabs), UIPanel, tab) tab->parent = b;
		b->a = parent->a;
		b->b = parent->b;
		b->a->parent = b;
		b->b->parent = b;
	}

	parent->a = panel;
	parent->b = b;
	parent->splitPoint = MIN(dim, parentDim/2);
	parent->a->parent = parent;
	parent->b->parent = parent;
	parent->align = align;
}

void UIPanelUndock(FixedSize* allocator, UIPanel* panel) {
	UIPanel* parent = panel->parent;

	if (parent->align != Align_Unaligned) {
		UIPanel* sibling = parent->a == panel ? parent->b : parent->a;
		if (sibling->align == Align_Unaligned) {
			parent->align = Align_Unaligned;
			parent->data = sibling->data;
			parent->tabs = sibling->tabs;
			LINKEDLIST_FOREACH(&(parent->tabs), UIPanel, tab) tab->parent = parent;
			sibling->data = NULL;
			parent->a = NULL;
			parent->b = NULL;	
		}
		else {
			parent->align = sibling->align;
			parent->splitPoint = sibling->splitPoint;
			parent->a = sibling->a;
			parent->b = sibling->b;
			parent->a->parent = parent;
			parent->b->parent = parent;
		}

		panel->parent = NULL;
		FixedSizeFree(allocator, sibling);
	}
	else {
		LINKEDLIST_REMOVE(&(parent->tabs), panel);
		panel->prev = NULL;
		panel->next = NULL;
		panel->parent = NULL;
		parent->tabs.count--;
		panel->pos = parent->pos;
		panel->dim = parent->dim;
		if (parent->tabs.selected == panel) parent->tabs.selected = parent->tabs.last;

		if (parent->tabs.count == 1) {
			UIPanel* first = parent->tabs.first;
			parent->data = first->data;
			LINKEDLIST_REMOVE(&(parent->tabs), first);
			if (parent->tabs.selected == first) parent->tabs.selected = parent->tabs.last;
			FixedSizeFree(allocator, first);
		}
	}
}

void UIPanelAddTab(FixedSize* allocator, UIPanel* panel, UIPanel* tab) {
	if (!panel->tabs.first) {
		UIPanel* first = (UIPanel*)FixedSizeAlloc(allocator);
		LINKEDLIST_ADD(&(panel->tabs), first);
		first->data = panel->data;
		first->tabs = panel->tabs;
		first->parent = panel;
		panel->data = NULL;
		panel->tabs.count = 1;
	}

	LINKEDLIST_ADD(&(panel->tabs), tab);
	tab->parent = panel;
	panel->tabs.selected = tab;
	panel->tabs.count++;
}

UIPanel* UIPanelGetPanelByPosition(UIPanel* panel, Point2 cursor) {
	if (panel->align == Align_Unaligned) {
		return InBounds(panel->pos, panel->dim, cursor) ? panel : NULL;
	}

	UIPanel* a = UIPanelGetPanelByPosition(panel->a, cursor);
	if (a) return a;
	return UIPanelGetPanelByPosition(panel->b, cursor);
}

bool UIPanelsProcessEvent(UIPanelManager* manager, OSEvent event) {
	Point2i* cursori = &event.mouse.cursorPos;
	Point2 cursor = {(float32)cursori->x + 0.5f, (float32)cursori->y + 0.5f};

	const float32 minWidth = 100;
	const float32 minHeight = 100;

	switch (event.type) {
	case Event_MouseMove: {

		if (manager->active) {
			UIPanel* panel = manager->active;

			if (manager->state == UIPS_Drag) {

				if (panel->parent) {
					UIPanelUndock(&manager->allocator, panel);
					LINKEDLIST_ADD(&(manager->floating), panel);
				}
				panel->pos = manager->grabPos + cursor;

				return true;
			}

			else if (manager->state == UIPS_Resize) {

				if (manager->hover == UIPH_Splitter) {
					switch (panel->align) {
					case Align_Left: { 
						panel->splitPoint = MAX(minWidth, manager->grabPos.x + cursor.x);
					} break;
					case Align_Top: {
						panel->splitPoint = MAX(minHeight, manager->grabPos.x + cursor.y);
					} break;
					case Align_Right: { 
						panel->splitPoint = MAX(minWidth, manager->grabPos.x - cursor.x);
					} break;
					case Align_Bottom: { 
						panel->splitPoint = MAX(minHeight, manager->grabPos.x - cursor.y);
					} break;
					}
				}
				else {
					Point2 pos = manager->grabPos + cursor;

					if (manager->hover == UIPH_BottomRight) {
						panel->width = MAX(minWidth, pos.x - panel->x);
						panel->height = MAX(minHeight, pos.y - panel->y);
					}
					else if (manager->hover == UIPH_Right) {
						panel->width = MAX(minWidth, pos.x - panel->x);
					}
					else if (manager->hover == UIPH_Bottom) {
						panel->height = MAX(minHeight, pos.y - panel->y);
					}
					else if (manager->hover == UIPH_Top) {
						float32 y0 = MIN(pos.y, panel->y + panel->height - minHeight);
						panel->height = panel->y + panel->height - y0;
						panel->y = y0;
					}
					else if (manager->hover == UIPH_Left) {
						float32 x0 = MIN(pos.x, panel->x + panel->width - minWidth);
						panel->width = panel->x + panel->width - x0;
						panel->x = x0;
					}
				}

				return true;
			}
		}

		manager->SetActivePanel(manager, cursor);
		return manager->active != NULL;
	} break;

	case Event_MouseLeftButtonDown: {
		if (manager->active) {
			UIPanel* panel = manager->active;

			if (manager->hover == UIPH_Header) {
				manager->state = UIPS_Drag;

				if (!panel->parent) // floating
					LINKEDLIST_MOVE_TO_LAST(&(manager->floating), panel);
				else if (panel->parent->align == Align_Unaligned) {
					panel->parent->tabs.selected = panel;
					panel->pos = panel->parent->pos;
				}

				manager->grabPos = panel->pos - cursor;

				return true;
			}

			else if (manager->hover == UIPH_Splitter) {
				manager->state = UIPS_Resize;
				
				switch (panel->align) {
				case Align_Left: { 
					manager->grabPos.x = panel->splitPoint - cursor.x;
				} break;
				case Align_Top: {
					manager->grabPos.x = panel->splitPoint - cursor.y;
				} break;
				case Align_Right: { 
					manager->grabPos.x = panel->splitPoint + cursor.x;
				} break;
				case Align_Bottom: { 
					manager->grabPos.x = panel->splitPoint + cursor.y;
				} break;
				}

				return true;
			}

			else if (manager->hover == UIPH_Closing) {
				if (panel->parent) {
					UIPanelUndock(&manager->allocator, panel);
				}
				else {
					LINKEDLIST_REMOVE(&(manager->floating), panel);
				}
				manager->active = NULL;
				FixedSizeFree(&manager->allocator,panel);

				return true;
			}

			else {
				manager->state = UIPS_Resize;

				if (manager->hover == UIPH_Left || manager->hover == UIPH_Top)
					manager->grabPos = panel->pos - cursor;
				else
					manager->grabPos = panel->pos + panel->dim - cursor;
				LINKEDLIST_MOVE_TO_LAST(&(manager->floating), panel);

				return true;
			}			
		}

		return false;
	} break;

	case Event_MouseLeftButtonUp: {

		if (manager->active) {
			UIPanel* panel = manager->active;

			if (manager->state == UIPS_Drag) {
				manager->state = UIPS_Rest;

				if (!panel->parent) {
					UIPanel* dockPanel;
					UIAlignement dock;
					manager->GetDock(manager, cursor, &dockPanel, &dock);
					if (dockPanel != panel && dock != Align_Unaligned) {
						if (dock == Align_Center) {
							LINKEDLIST_REMOVE(&(manager->floating), panel);
							UIPanelAddTab(&manager->allocator, dockPanel, panel);
						}
						else {
							LINKEDLIST_REMOVE(&(manager->floating), panel);
							UIPanelSplit(&manager->allocator, dockPanel, dock, panel);
						}
					}
				}
			}

			else {
				manager->state = UIPS_Rest;
			}
		}

		return false;
	} break;

	}

	return false;
}