#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	Arena scratch = CreateArena(1024*1024*1024);
	Arena persist = CreateArena(1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("OpenGL Window", {512, 512}, RGBA_DARKGREY);

	Font* font = (Font*)ArenaAlloc(&persist, sizeof(Font));
	*font = LoadFont(&scratch, "..\\AzeretMono-Regular.ttf", 24);

	UIElement* e1 = UICreateElement(NULL);
	e1->pos = {12, 12};
	e1->dim = {200, 200};
	e1->flags = UI_SCROLLABLE;
	e1->borderWidth = 1;
	e1->borderColor = RGBA_WHITE;
	e1->text.font = font;
	e1->text.color = RGBA_WHITE;
	e1->text.string = STR(R"STRING(Lorem ipsum dolor sit amet, 
consectetur adipiscing elit, 
sed do eiusmod tempor incididunt 
ut labore et dolore magna aliqua. 
Ut enim ad minim veniam, quis nostrud 
exercitation ullamco laboris nisi ut 
aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit
in voluptate velit esse cillum dolore
eu fugiat nulla pariatur. Excepteur
sint occaecat cupidatat non proident, 
sunt in culpa qui officia deserunt
mollit anim id est laborum.)STRING");

	byte itemsbuf[] = "item 1item 2item 3item 4item 5item 6item 7";
	UIElement* list = UICreateElement(NULL);
	list->dim = {120, 100};
	list->pos = {12, 224};
	list->flags = UI_SCROLLABLE;
	list->borderWidth = 1;
	list->borderColor = RGBA_WHITE;
	for(int32 i = 0; i < 7; i++){
		UIElement* item = UICreateElement(list);
		item->pos = {12, 24*i + 12};
		item->dim = {120, 42};
		item->text.font = font;
		item->text.string = {itemsbuf+ 6*i, 6};
		item->text.color = RGBA_WHITE;
	}


	UIElement* debug1 = UICreateElement(NULL);
	debug1->pos = {12, 430};
	debug1->flags = UI_MOVABLE;
	debug1->background = 0x33ffffff;
	debug1->text.font = font;
	debug1->text.color = RGBA_WHITE;

	UIElement* debug2 = UICreateElement(NULL);
	debug2->pos = {12, 454};
	debug2->flags = UI_MOVABLE;
	debug2->background = 0x33ffffff;
	debug2->text.font = font;
	debug2->text.color = RGBA_WHITE;

	byte buffer[10] = {};

	while(!OSWindowDestroyed() && !OSIsKeyDown(KEY_ESC)) {
		ArenaFreeAll(&scratch);
		OSProcessWindowEvents();

		int32 length1 = Int32ToDecimal(ui.start, buffer);
		debug1->text.string = {(byte*)buffer, length1};
		int32 length2 = Int32ToDecimal(ui.end, buffer + length1);
		debug2->text.string = {(byte*)buffer+length1, length2};

		GfxClearScreen();

		UIUpdateActiveElement();
		UIRenderElements();

		GfxSwapBuffers();
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup() {
    ExitProcess(main());
}
#endif