#define LOG(text)	MessageBoxA(NULL, text, NULL, 0)

#include "basic/basic.cpp"
#include "graphics/graphics.cpp"
#include "ui/ui.cpp"

int main() {
	Arena scratch = CreateArena(1024*1024*1024);
	Arena persist = CreateArena(1024*1024);

	UIInit(&persist, &scratch);
	UICreateWindow("OpenGL Window", {512, 512}, RGBA_DARKGREY);

	Font* font = (Font*)ArenaAlloc(&persist, sizeof(Font));
	*font = LoadFont(&scratch, "..\\AzeretMono-Regular.ttf", 24, 0xffffff);

	UIElement* list = UICreateElement(NULL);
	list->background = RGBA_LIGHTGREY;
	list->pos = {200, 200};
	list->dim = {128, 180};
	list->flags = UI_MOVABLE | UI_SCROLLABLE;

	const char* str = "item 1item 2item 3item 4item 5item 6item 7";
	for (int32 i=0; i<7; i++) {
		UIElement* item = UICreateElement(list);
		item->background = RGBA_LIGHTGREY;
		item->pos = {6, i*36+24};
		item->height = 30;
		item->flags = UI_FITTEXT;

		UIText* text = UICreateText(item);
		text->string = {(byte*)str+i*6, 6};
		text->font = font;
	}

	while(!OsWindowDestroyed() && !IsKeyDown(KEY_ESC)) {
		ArenaFreeAll(&scratch);
		OsProcessWindowEvents();

		GfxClearScreen();

		UIUpdateElements();
		UIRenderElements();

		//GfxDrawImage(font->texture, {0, 0, 1, 1}, {0, 0, 128, 128});

		GfxSwapBuffers();
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
void __stdcall WinMainCRTStartup() {
    ExitProcess(main());
}
#endif