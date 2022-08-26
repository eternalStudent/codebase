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

	UIElement* e1 = UICreateScrollingPane(NULL, {200, 200}, {12, 12});
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
	UIElement* list = UICreateScrollingPane(NULL, {100, 100}, {12, 224});
	list->borderWidth = 1;
	list->borderColor = RGBA_WHITE;
	for(int32 i = 0; i < 7; i++){
		UIElement* item = UICreateElement(list);
		item->pos = {6, 24*i + 6};
		item->dim = {120, 42};
		item->text.font = font;
		item->text.string = {itemsbuf+ 6*i, 6};
		item->text.color = RGBA_WHITE;
	}

	while(!OSWindowDestroyed() && !OSIsKeyDown(KEY_ESC)) {
		ArenaFreeAll(&scratch);
		OSProcessWindowEvents();

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